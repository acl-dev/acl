#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_client_pipeline.hpp"
#endif

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl {

redis_pipeline_channel::redis_pipeline_channel(const char* addr,
	int conn_timeout, int rw_timeout, bool retry)
: addr_(addr)
, buf_(81920)
{
	conn_ = NEW redis_client(addr, conn_timeout, rw_timeout, retry);
}

redis_pipeline_channel::~redis_pipeline_channel(void)
{
	delete conn_;
}

bool redis_pipeline_channel::start_thread(void)
{
	if (!((connect_client*) conn_)->open()) {
		logger_error("open %s error %s", addr_.c_str(), last_serror());
		return false;
	}

	this->start();
	return true;
}

void redis_pipeline_channel::push(redis_pipeline_message* msg)
{
	msgs_.push_back(msg);
#ifdef USE_MBOX
	box_.push(msg);
#else
	box_.push(msg, false);
#endif
}

void redis_pipeline_channel::flush(void)
{
	if (msgs_.empty()) {
		return;
	}

	buf_.clear();
	for (std::vector<redis_pipeline_message*>::iterator it = msgs_.begin();
		it != msgs_.end(); ++it) {
		string* req = (*it)->get_cmd().get_request_buf();
		buf_.append(req->c_str(), req->size());
	}
	msgs_.clear();

#ifdef DEBUG_BOX
	printf(">>>%s<<<\r\n", buf_.c_str());
#endif

	socket_stream* conn = conn_->get_stream();
	if (conn == NULL) {
		printf("conn NULL\r\n");
		exit(1);
	}

	if (conn->write(buf_) == -1) {
		printf("write error, addr=%s, buf=%s\r\n",
			addr_.c_str(), buf_.c_str());
		exit(1);
	}
#ifdef DEBUG_BOX
	printf("write ok, nmsg=%ld\n", msgs.size());
#endif

}

void* redis_pipeline_channel::run(void)
{
	dbuf_pool* dbuf;
	const redis_result* result;
	size_t nchild;
	int* timeout;

	while (!conn_->eof()) {
		redis_pipeline_message* msg = box_.pop();
		if (msg == NULL) {
			break;
		}

#ifdef DEBUG_BOX
		printf("reader: get msg\r\n");
#endif
		socket_stream* conn = conn_->get_stream();
		if (conn == NULL) {
			printf("get_stream null\r\n");
			break;
		}

		dbuf = msg->get_cmd().get_dbuf();
		timeout = msg->get_timeout();
		if (timeout) {
			conn->set_rw_timeout(*timeout);
		}

		nchild = msg->get_nchild();
		if (nchild >= 1) {
			result = conn_->get_objects(*conn, dbuf, nchild);
		} else {
			result = conn_->get_object(*conn, dbuf);
		}
		msg->push(result);
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////////////

redis_client_pipeline::redis_client_pipeline(const char* addr)
: addr_(addr)
, conn_timeout_(10)
, rw_timeout_(10)
, retry_(true)
, nchannels_(1)
{
}

redis_client_pipeline::~redis_client_pipeline(void)
{
}

redis_client_pipeline& redis_client_pipeline::set_timeout(int conn_timeout,
	int rw_timeout)
{
	conn_timeout_ = conn_timeout;
	rw_timeout_   = rw_timeout;
	return *this;
}

redis_client_pipeline& redis_client_pipeline::set_retry(bool on)
{
	retry_ = on;
	return *this;
}

redis_client_pipeline& redis_client_pipeline::set_channels(size_t n)
{
	nchannels_ = n;
	return *this;
}

const redis_result* redis_client_pipeline::run(redis_pipeline_message& msg)
{
#ifdef USE_MBOX
	box_.push(&msg);
#else
	box_.push(&msg, false);
#endif

	return msg.wait();
}

void* redis_client_pipeline::run(void)
{
	for (size_t i = 0; i < 4; i++) {
		redis_pipeline_channel* channel = NEW redis_pipeline_channel(
			addr_, conn_timeout_, rw_timeout_, retry_);
		if (channel->start_thread()) {
			channels_.push_back(channel);
		} else {
			delete channel;
		}
	}

	if (channels_.empty()) {
		logger_error("no channel created!");
		return NULL;
	}

	size_t size = channels_.size(), count = 0;
	redis_pipeline_channel* channel;
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
			channel = channels_[count++ % size];
			channel->push(msg);
			timeout = 0;
#ifdef USE_MBOX
		} else if (!success) {
#else
		} else if (found) {
#endif
			break;
		} else {
			timeout = -1;
			flush_all();
		}
	}

	printf("Exiting ...\r\n");
	return NULL;
}

void redis_client_pipeline::flush_all(void)
{
	for (std::vector<redis_pipeline_channel*>::iterator
		it = channels_.begin(); it != channels_.end(); ++it) {
		(*it)->flush();
	}
}

} // namespace acl

#endif // ACL_CLIENT_ONLY
