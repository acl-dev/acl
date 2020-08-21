#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_client_pipeline.hpp"
#endif

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl {

redis_reader::redis_reader(redis_client& conn)
: conn_(conn)
{
}

redis_reader::~redis_reader(void) {}

void redis_reader::push(redis_pipeline_message* msg)
{
#ifdef USE_MBOX
	box_.push(msg);
#else
	box_.push(msg, false);
#endif
}

void* redis_reader::run(void)
{
	while (!conn_.eof()) {
		redis_pipeline_message* msg = box_.pop();
		if (msg == NULL) {
			break;
		}

#ifdef DEBUG_BOX
		printf("reader: get msg\r\n");
#endif
		socket_stream* conn = conn_.get_stream();
		if (conn == NULL) {
			printf("get_stream null\r\n");
			break;
		}

		dbuf_pool* dbuf = msg->cmd_->get_dbuf();
		msg->result_ = conn_.get_object(*conn, dbuf);
//#ifdef USE_MBOX
		msg->box_.push(msg);
//#else
//		msg->box_.push(msg, false);
//#endif
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////////////

redis_client_pipeline::redis_client_pipeline(const char* addr, int conn_timeout,
	int rw_timeout, bool retry)
: addr_(addr)
, conn_timeout_(conn_timeout)
, rw_timeout_(rw_timeout)
, retry_(retry)
{
	conn_ = NEW redis_client(addr, conn_timeout, rw_timeout, retry);
}

redis_client_pipeline::~redis_client_pipeline(void)
{
	delete conn_;
}

const redis_result* redis_client_pipeline::run(redis_command* cmd,
	size_t nchild, int* timeout)
{
#if 0
	BOX<redis_pipeline_message> box(false);
	dbuf_pool* dbuf = cmd->get_dbuf();
	redis_pipeline_message* msg = new(dbuf)
		redis_pipeline_message(cmd, nchild, timeout, box);

	box_.push(msg);

	msg = box.pop();
	//printf(">>>>box get result, msg=%p\r\n", msg);
	return msg->result_;
#else
	mbox<redis_pipeline_message> box(false);
	redis_pipeline_message msg(cmd, nchild, timeout, box);

#ifdef USE_MBOX
	box_.push(&msg);
#else
	box_.push(&msg, false);
#endif

	redis_pipeline_message* m = box.pop();
	if (m == NULL) {
		exit(1);
	}
#ifdef DEBUG_BOX
	printf(">>>>box get result, msg=%p\r\n", m);
#endif
	return m->result_;
#endif
}

void* redis_client_pipeline::run(void)
{
	if (!((connect_client*) conn_)->open()) {
		logger_error("open %s error %s", addr_.c_str(), last_serror());
		return NULL;
	}

	reader_ = NEW redis_reader(*conn_);
	reader_->start();

	std::vector<redis_pipeline_message*> msgs;
	int  timeout = -1;
#ifdef USE_MBOX
	bool success;
#else
	bool found;
#endif

	while (true) {
#ifdef USE_MBOX
		redis_pipeline_message* msg = box_.pop(timeout, &success);
#else
		redis_pipeline_message* msg = box_.pop(timeout, &found);
#endif

#ifdef DEBUG_BOX
		printf("peek one msg=%p, timeout=%d\r\n", msg, timeout);
#endif
		if (msg != NULL) {
			msgs.push_back(msg);
			reader_->push(msg);
			timeout = 0;
#ifdef USE_MBOX
		} else if (!success) {
#else
		} else if (found) {
#endif
			break;
		} else if (msgs.empty()) {
			timeout = -1;
		} else {
			timeout = -1;
			send(msgs);
			msgs.clear();
		}
	}

	printf("Exiting ...\r\n");
	return NULL;
}

void redis_client_pipeline::send(std::vector<redis_pipeline_message*>& msgs)
{
	acl::string buf(81920);
	for (std::vector<redis_pipeline_message*>::iterator it = msgs.begin();
		it != msgs.end(); ++it) {
		string* req = (*it)->cmd_->get_request_buf();
		buf.append(req->c_str(), req->size());
	}

#ifdef DEBUG_BOX
	printf(">>>%s<<<\r\n", buf.c_str());
#endif

	socket_stream* conn = conn_->get_stream();
	if (conn == NULL) {
		printf("conn NULL\r\n");
		exit(1);
	}

	if (conn->write(buf) == -1) {
		printf("write error, addr=%s, buf=%s\r\n",
			addr_.c_str(), buf.c_str());
		exit(1);
	}
#ifdef DEBUG_BOX
	printf("write ok, nmsg=%ld\n", msgs.size());
#endif
}

} // namespace acl

#endif // ACL_CLIENT_ONLY
