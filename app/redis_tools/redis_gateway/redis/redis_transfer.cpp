#include "stdafx.h"
#include "redis_object.h"
#include "redis_request.h"
#include "redis_transfer.h"

redis_transfer::redis_transfer(acl::dbuf_guard& dbuf, acl::socket_stream& conn,
	acl::redis_client_pipeline& pipeline)
: dbuf_(dbuf)
, conn_(conn)
, pipeline_(pipeline)
, buff_(256)
{
}

redis_transfer::~redis_transfer(void) {}

bool redis_transfer::run(const std::vector<const redis_object*>& reqs) {
	std::vector<redis_request*> requests;
	requests.reserve(200);

#define	EQ	!acl_strcasecmp

	for (std::vector<const redis_object*>::const_iterator cit = reqs.begin();
		cit != reqs.end(); ++cit) {

		//redis_request* request = new redis_request(&pipeline_);
		redis_request* request = dbuf_.create<redis_request>
			(&dbuf_, &pipeline_);
		request->build_request(**cit);
		const char** argv = request->get_argv();
		size_t argc = request->get_argc();
		if (argc >= 2) {
			if (EQ(argv[0], "CLUSTER") && EQ(argv[1], "SLOTS")) {
				if (!redirect2me()) {
					return false;
				}
				continue;
			}
		}

		requests.push_back(request);
		acl::redis_pipeline_message& msg = request->get_message();
		pipeline_.push(&msg);
	}

#if 0
	if (requests.size() > 100) {
		printf(">>>request size=%zd\n", requests.size());
	}
#endif

	buff_.clear();

	for (std::vector<redis_request*>::iterator it = requests.begin();
		it != requests.end(); ++it) {

		acl::redis_pipeline_message& msg = (*it)->get_message();
		const acl::redis_result* result = msg.wait();
		if (result != NULL) {
			build_reply(*result, buff_);
		}
	}

	//printf(">>>>reply=[%s]\r\n", buff_.c_str());

	if (conn_.write(buff_) == (int) buff_.size()) {
		return true;
	}

	logger("Reply client error!");
	return false;
}

bool redis_transfer::build_reply(const acl::redis_result& result,
	acl::string& buff) {
	return reply_add_object(result, buff);
}

bool redis_transfer::reply_add_object(const acl::redis_result& obj,
	acl::string& buff) {

	switch (obj.get_type()) {
	case acl::REDIS_RESULT_ARRAY:
		return reply_add_array(obj, buff);
	case acl::REDIS_RESULT_STRING:
		return reply_add_string(obj, buff);
	case acl::REDIS_RESULT_INTEGER:
		return reply_add_integer(obj, buff);
	case acl::REDIS_RESULT_STATUS:
		return reply_add_status(obj, buff);
	case acl::REDIS_RESULT_ERROR:
		return reply_add_error(obj, buff);
	default:
		logger_error("unknown type=%d", (int) obj.get_type());
		return false;
	}
}

bool redis_transfer::reply_add_array(const acl::redis_result& obj,
	acl::string& buff) {

	size_t size;
	const acl::redis_result** children = obj.get_children(&size);
	if (children == NULL || size == 0) {
		logger_error("No children!");
		return false;
	}

	buff.format_append("*%zd\r\n", size);

	for (size_t i = 0; i < size; i++) {
		const acl::redis_result* child = children[i];
		if (!reply_add_object(*child, buff)) {
			return false;
		}
	}

	return true;
}

bool redis_transfer::reply_add_string(const acl::redis_result& obj,
	acl::string& buff) {

	size_t len;
	const char* data = obj.get(0, &len);
	if (data == NULL) {
		buff.format_append("$-1\r\n");
		return true;
	}

	buff.format_append("$%zd\r\n%s\r\n", len, data);
	return true;
}

bool redis_transfer::reply_add_integer(const acl::redis_result& obj,
	acl::string& buff) {

	bool ok;
	long long n = obj.get_integer64(&ok);
	if (!ok) {
		logger_error("get_integer64 error");
		return false;
	}

	buff.format_append(":%lld\r\n", n);
	return true;
}

bool redis_transfer::reply_add_status(const acl::redis_result& obj,
	acl::string& buff) {

	const char* ptr = obj.get_status();
	buff.format_append("+%s\r\n", ptr);
	return true;
}

bool redis_transfer::reply_add_error(const acl::redis_result& obj,
	acl::string& buff) {

	const char* ptr = obj.get_error();
	buff.format_append("-%s\r\n", ptr);
	return true;
}

bool redis_transfer::redirect2me(void) {
	acl::string buff;

	buff += "*4\r\n";

	buff += "*3\r\n";
	buff += ":0\r\n";
	buff += ":4000\r\n";
	buff += "*3\r\n";
	buff += "$9\r\n";
	buff += "127.0.0.1\r\n";
	buff += ":16379\r\n";
	buff += "$40\r\n";
	buff += "5c17f9e161196446a9be4aa1c62c5e1518ece030\r\n";

	buff += "*3\r\n";
	buff += ":4001\r\n";
	buff += ":8000\r\n";
	buff += "*3\r\n";
	buff += "$9\r\n";
	buff += "127.0.0.1\r\n";
	buff += ":16379\r\n";
	buff += "$40\r\n";
	buff += "6c17f9e161196446a9be4aa1c62c5e1518ece030\r\n";

	buff += "*3\r\n";
	buff += ":8001\r\n";
	buff += ":12000\r\n";
	buff += "*3\r\n";
	buff += "$9\r\n";
	buff += "127.0.0.1\r\n";
	buff += ":16379\r\n";
	buff += "$40\r\n";
	buff += "7c17f9e161196446a9be4aa1c62c5e1518ece030\r\n";

	buff += "*3\r\n";
	buff += ":12001\r\n";
	buff += ":16383\r\n";
	buff += "*3\r\n";
	buff += "$9\r\n";
	buff += "127.0.0.1\r\n";
	buff += ":16379\r\n";
	buff += "$40\r\n";
	buff += "8c17f9e161196446a9be4aa1c62c5e1518ece030\r\n";

	if (conn_.write(buff) == (int) buff.size()) {
		return true;
	}

	logger_error("reply to client error");
	return false;
}
