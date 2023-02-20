#include "stdafx.h"
#include "redis_object.h"
#include "redis_transfer.h"

redis_transfer::redis_transfer(acl::socket_stream& conn, acl::redis_command& cmd,
	const redis_object& req)
: conn_(conn)
, cmd_(cmd)
, req_(req)
{
}

redis_transfer::~redis_transfer(void) {}

bool redis_transfer::run(void) {
	const char**  argv = req_.get_argv();
	const size_t* lens = req_.get_lens();
	size_t        argc = req_.get_size();

	if (argc >= 2) {
		cmd_.hash_slot(argv[1]);
	}
	cmd_.build_request(argc, argv, lens);

	//const acl::redis_result* result = cmd_.run();

	// Get the pipeline message being transfered between curr thread
	// and the pipeline thread.
	acl::redis_pipeline_message& msg = cmd_.get_pipeline_message();

	// Get the pipeline object for putting message to it.
	acl::redis_client_pipeline* pipeline = cmd_.get_pipeline();

	// Push one request message to the pipeline thread.
	pipeline->push(&msg);

	// Wait result message from the box in msg object.
	const acl::redis_result* result = msg.wait();

	if (result == NULL) {
		logger_error("get reply from redis error!");
		return false;
	}

	acl::string buff;

	int ret = result->argv_to_string(buff);
	if (ret == 0) {
		buff += "$-1";
	} else if (ret < 0) {
		logger_error("Result data is empty, error=%s, status=%s, type=%d, ret=%d",
			result->get_error(), result->get_status(),
			result->get_type(), ret);
		return false;
	}

	buff += "\r\n";

	if (conn_.write(buff) == (int) buff.size()) {
		return true;
	}

	logger("Reply client error!");
	return false;
}
