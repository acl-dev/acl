#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/token_tree.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_command.hpp"
#include "acl_cpp/redis/redis_cluster.hpp"
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
, max_slot_(16384)
, conn_timeout_(10)
, rw_timeout_(10)
, retry_(true)
, nchannels_(1)
{
	slot_addrs_ = (const char**) acl_mycalloc(max_slot_, sizeof(char*));
	channels_   = NEW token_tree;
}

redis_client_pipeline::~redis_client_pipeline(void)
{
	for (std::vector<char*>::iterator it = addrs_.begin();
		it != addrs_.end(); ++it) {
		acl_myfree(*it);
	}
	acl_myfree(slot_addrs_);
	delete channels_;
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

redis_client_pipeline & redis_client_pipeline::set_max_slot(size_t max_slot) {
	max_slot_ = max_slot;
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
	set_all_slot();
	start_channels();

	if (channels_->first_node() == NULL) {
		logger_error("no channel created!");
		return NULL;
	}

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
			int slot = msg->get_cmd().get_slot();
			channel = get_channel(slot);
			if (channel == NULL) {
				printf("channel null, slot=%d\r\n", slot);
				exit(1);
			}
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
	const token_node* iter = channels_->first_node();
	while (iter) {
		redis_pipeline_channel* channel = (redis_pipeline_channel*)
			iter->get_ctx();
		channel->flush();
		iter = channels_->next_node();
	}
}

void redis_client_pipeline::set_slot(size_t slot, const char* addr)
{
	if (slot >= max_slot_ || addr == NULL || *addr == 0) {
		return;
	}

	// 遍历缓存的所有地址，若该地址不存在则直接添加，然后使之与 slot 进行关联

	std::vector<char*>::const_iterator cit = addrs_.begin();
	for (; cit != addrs_.end(); ++cit) {
		if (strcmp((*cit), addr) == 0) {
			break;
		}
	}

	// 将 slot 与地址进行关联映射
	if (cit != addrs_.end()) {
		slot_addrs_[slot] = *cit;
	} else {
		// 只所以采用动态分配方式，是因为在往数组中添加对象时，无论
		// 数组如何做动态调整，该添加的动态内存地址都是固定的，所以
		// slot_addrs_ 的下标地址也是相对不变的
		char* buf = acl_mystrdup(addr);
		addrs_.push_back(buf);
		slot_addrs_[slot] = buf;
	}
}

void redis_client_pipeline::set_all_slot(void)
{
	redis_client client(addr_, 30, 60, false);

	if (!passwd_.empty()) {
		client.set_password(passwd_);
	}

	redis_cluster cluster(&client);

	const std::vector<redis_slot*>* slots = cluster.cluster_slots();
	if (slots == NULL) {
		return;
	}

	std::vector<redis_slot*>::const_iterator cit;
	for (cit = slots->begin(); cit != slots->end(); ++cit) {
		const redis_slot* slot = *cit;
		const char* ip = slot->get_ip();
		int port = slot->get_port();
		if (*ip == 0 || port <= 0 || port > 65535) {
			continue;
		}

		size_t slot_min = slot->get_slot_min();
		size_t slot_max = slot->get_slot_max();
		if (slot_max >= max_slot_ || slot_max < slot_min) {
			continue;
		}

		char buf[128];
		safe_snprintf(buf, sizeof(buf), "%s:%d", ip, port);
		for (size_t i = slot_min; i <= slot_max; i++) {
			set_slot(i, buf);
		}
	}
}

void redis_client_pipeline::start_channels(void) {
	for (std::vector<char*>::const_iterator cit = addrs_.begin();
		cit != addrs_.end(); ++cit) {
		redis_pipeline_channel* channel = NEW redis_pipeline_channel(
			*cit, conn_timeout_, rw_timeout_, retry_);
		if (channel->start_thread()) {
			channels_->insert(*cit, channel);
		} else {
			delete channel;
		}
	}

	if (channels_->first_node() == NULL) {
		return;
	}

	redis_pipeline_channel* channel = NEW redis_pipeline_channel(
		addr_, conn_timeout_, rw_timeout_, retry_);
	if (channel->start_thread()) {
		channels_->insert(addr_, channel);
	} else {
		delete channel;
	}
}

redis_pipeline_channel* redis_client_pipeline::get_channel(int slot) {
	const char* addr;
	if (slot >= 0 && slot < (int) max_slot_) {
		addr = slot_addrs_[slot];
		if (addr == NULL) {
			addr = addr_.c_str();
		}
	} else {
		addr = addr_.c_str();
	}

	const token_node* node = channels_->find(addr);
	return node == NULL ? NULL : (redis_pipeline_channel*) node->get_ctx();
}

} // namespace acl

#endif // ACL_CLIENT_ONLY
