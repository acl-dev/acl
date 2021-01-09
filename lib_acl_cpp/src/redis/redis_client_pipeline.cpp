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

redis_pipeline_channel::redis_pipeline_channel(redis_client_pipeline& pipeline,
	const char* addr, int conn_timeout, int rw_timeout, bool retry)
: pipeline_(pipeline)
, addr_(addr)
, buf_(81920)
{
	client_ = NEW redis_client(addr, conn_timeout, rw_timeout, retry);
}

redis_pipeline_channel::~redis_pipeline_channel(void)
{
	delete client_;
}

redis_pipeline_channel& redis_pipeline_channel::set_passwd(const char *passwd)
{
	if (passwd && *passwd) {
		client_->set_password(passwd);
	}
	return *this;
}

bool redis_pipeline_channel::start_thread(void)
{
	if (!((connect_client*) client_)->open()) {
		logger_error("open %s error %s", addr_.c_str(), last_serror());
		return false;
	}
	this->start();
	return true;
}

void redis_pipeline_channel::stop_thread(void)
{
	redis_pipeline_message message(NULL, redis_pipeline_t_stop);
	push(&message);
	this->wait();
}

void redis_pipeline_channel::push(redis_pipeline_message* msg)
{
	box_.push(msg, false);
}

bool redis_pipeline_channel::flush_all(void)
{
	if (msgs_.empty()) {
		return true;
	}

	buf_.clear();

	for (std::vector<redis_pipeline_message*>::iterator it = msgs_.begin();
		it != msgs_.end(); ++it) {
#if 0
		string* req = (*it)->get_cmd()->get_request_buf();
		buf_.append(req->c_str(), req->size());
#else
		redis_command::build_request((*it)->argc_, (*it)->argv_,
			(*it)->lens_, buf_);
#endif
	}

	bool retried = false;
	while (true) {
		socket_stream* conn = client_->get_stream(false);
		if (conn) {
			if (conn->write(buf_) == (int) buf_.size()) {
				return true;
			}

			logger_error("write error=%s, addr=%s, buf=%s",
				last_serror(), addr_.c_str(), buf_.c_str());
		}

		// return false if we have retried
		if (retried) {
			return false;
		}

		// close the old connection
		client_->close();

		// reopen the new connection
		if (!((connect_client*) client_)->open()) {
			logger_error("reopen connection failed!");
			return false;
		}
		retried = true;
	}
}

void redis_pipeline_channel::all_failed()
{
	std::vector<redis_pipeline_message*>::iterator it;
	for (it = msgs_.begin(); it != msgs_.end(); ++it) {
		(*it)->push(NULL);
	}
}

bool redis_pipeline_channel::wait_one(socket_stream& conn,
	redis_pipeline_message& msg)
{
	dbuf_pool* dbuf = msg.get_cmd()->get_dbuf();
	int* timeout = msg.get_timeout();
	if (timeout) {
		conn.set_rw_timeout(*timeout);
	}

	const redis_result* result;
	size_t nchild = msg.get_nchild();
	if (nchild >= 1) {
		result = client_->get_objects(conn, dbuf, nchild);
	} else {
		result = client_->get_object(conn, dbuf);
	}

	if (result == NULL) {
		logger_error("can't get result");
		return false;
	}

	int type = result->get_type();
	if (type == REDIS_RESULT_UNKOWN) {
		logger_warn("unknown type=%d", (int) type);
		msg.push(result);
		return true;
	} else if (type != REDIS_RESULT_ERROR) {
		msg.push(result);
		return true;
	}

#define	EQ(x, y) !strncasecmp((x), (y), sizeof(y) -1)

	const char* ptr = result->get_error();
	if (ptr == NULL || *ptr == 0) {
		logger_error("error info null");
		msg.push(result);
	} else if (EQ(ptr, "MOVED") || EQ(ptr, "ASK")) {
		const char* addr = msg.get_cmd()->get_addr(ptr);
		if (addr == NULL) {
			logger_error("no redirect addr got");
			msg.push(result);
		} if (msg.get_redirect_count() >= 5) {
			logger_error("redirect count(%d) exceed limit(5)",
				(int) msg.get_redirect_count());
			msg.push(result);
		} else {
			msg.set_addr(addr);
			msg.set_type(redis_pipeline_t_redirect);
			// transfer to pipeline processs again
			pipeline_.push(&msg);
		}
	} else if (EQ(ptr, "CLUSTERDOWN")) {
		msg.set_addr(this->get_addr());
		msg.set_type(redis_pipeline_t_clusterdonw);
		// transfer to pipeline processs again
		pipeline_.push(&msg);
	} else {
		logger_error("unknown error: %s", ptr);
		msg.push(result);
	}
	return true;
}

bool redis_pipeline_channel::wait_results(void)
{
	if (msgs_.empty()) {
		return true;
	}

	socket_stream* conn = client_->get_stream(false);
	if (conn == NULL) {
		logger_error("get_stream null");
		return false;
	}

	std::vector<redis_pipeline_message*>::iterator it;
	for (it = msgs_.begin(); it != msgs_.end(); ++it) {
		if (!wait_one(*conn, **it)) {
			break;
		}
	}

	// if we can't get the first result, the socket maybe be disconnected,
	// so we should return false and retry again.
	if (it == msgs_.begin()) {
		logger_error("get the first result failed");
		return false;
	}

	// return NULL for the left failed results
	for (; it != msgs_.end(); ++it) {
		(*it)->push(NULL);
	}

	return true;
}

bool redis_pipeline_channel::handle_messages(void)
{
	bool retried = false;

	while (true) {
		if (!flush_all()) {
			logger_error("all failed ...");
			break;
		}

		if (wait_results()) {
			msgs_.clear();
			return true;
		}

		if (retried) {
			logger_error("retried failed");
			break;
		}
		retried = true;

		client_->close();
		if (!((connect_client*) client_)->open()) {
			logger_error("reopen failed");
			break;
		}
	}

	all_failed();
	msgs_.clear();
	return false;
}

void* redis_pipeline_channel::run(void)
{
	bool success;
	int timeout = -1;

	while (!client_->eof()) {
		redis_pipeline_message* msg = box_.pop(timeout, &success);
		if (msg != NULL) {
			timeout = 0;

			switch (msg->get_type()) {
			case redis_pipeline_t_cmd:
				msgs_.push_back(msg);
				break;
			case redis_pipeline_t_stop:
				// handle left messages and stop the channel.
				handle_messages();
				return NULL;
			default:
				break;
			}
		} else if (!success) {
			break;
		} else {
			timeout = -1;
			handle_messages();
		}
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
, preconn_(true)
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

redis_client_pipeline& redis_client_pipeline::set_password(const char* passwd)
{
	if (passwd && *passwd) {
		passwd_ = passwd;
	}
	return *this;
}

redis_client_pipeline & redis_client_pipeline::set_max_slot(size_t max_slot)
{
	max_slot_ = max_slot;
	return *this;
}

redis_client_pipeline & redis_client_pipeline::set_preconnect(bool yes)
{
	preconn_ = yes;
	return *this;
}

void redis_client_pipeline::start_thread(void)
{
	this->start();
}

void redis_client_pipeline::stop_thread(void)
{
	redis_pipeline_message message(NULL, redis_pipeline_t_stop);
	push(&message);
	this->wait();  // wait for the thread to exit
}

const redis_result* redis_client_pipeline::run(redis_pipeline_message& msg)
{
	box_.push(&msg, false);
	return msg.wait();
}

void redis_client_pipeline::push(redis_pipeline_message *msg)
{
	box_.push(msg, false);
}

// called after the thread started
// @override from acl::thread
void* redis_client_pipeline::run(void)
{
	set_all_slot();
	if (preconn_) {
		start_channels();
	}

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
		if (msg != NULL) {
			redis_pipeline_type_t type = msg->get_type();
			if (type == redis_pipeline_t_stop) {
				break;
			}

			int slot = msg->get_cmd()->get_slot();
			if (type == redis_pipeline_t_redirect) {
				redirect(*msg, slot);
			} else if (type == redis_pipeline_t_clusterdonw) {
				cluster_down(*msg);
				break;
			}

			channel = get_channel(slot);
			if (channel == NULL) {
				logger_error("channel null, slot=%d", slot);
				msg->push(NULL);
				timeout = -1;
			} else {
				channel->push(msg);
				timeout = 0;
			}
#ifdef USE_MBOX
		} else if (!success) {
#else
		} else if (found) {
#endif
			break;
		} else {
			timeout = -1;
		}
	}

	stop_channels();
	return NULL;
}

void redis_client_pipeline::redirect(const redis_pipeline_message &msg, int slot)
{
	const char* addr = msg.get_addr();
	if (addr) {
		set_slot(slot, addr);
	}
}

void redis_client_pipeline::cluster_down(const redis_pipeline_message &msg)
{
	const char* addr = msg.get_addr();
	if (addr == NULL) {
		return;
	}

	// clear all slots' addrs same as the dead node
	for (size_t i = 0; i < max_slot_; i++) {
		if (strcmp(slot_addrs_[i], addr) == 0) {
			slot_addrs_[i] = NULL;
		}
	}

	// reset the default addr which different from the the dead node
	if (addr_ == addr) {
		for (std::vector<char*>::const_iterator it = addrs_.begin();
		     it != addrs_.end(); ++it) {
			if (strcmp(*it, addr) != 0) {
				addr_ = *it;
				break;
			}
		}

		// stop and remove the dead node
		stop_channel(addr);
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
		logger("can't get cluster slots");
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

void redis_client_pipeline::stop_channels(void)
{
	const token_node* iter = channels_->first_node();
	std::vector<redis_pipeline_channel*> channels;
	while (iter) {
		redis_pipeline_channel* channel = (redis_pipeline_channel*)
			iter->get_ctx();
		// notify and wait for the channel thread to exit
		channel->stop_thread();
		channels.push_back(channel);
		iter = channels_->next_node();
	}

	// delete all channels threads
	for (std::vector<redis_pipeline_channel*>::iterator
		     it = channels.begin(); it != channels.end(); ++it) {
		channels_->remove(((*it)->get_addr()));
		delete *it;
	}
	logger("all channels in pipeline have been stopped!");
}

void redis_client_pipeline::start_channels(void)
{
	for (std::vector<char*>::const_iterator cit = addrs_.begin();
		cit != addrs_.end(); ++cit) {
		(void) start_channel(*cit);
	}

	// 如果已经成功添加了集群节点，则说明为集群模式，按集群方式对待，
	// 否则，按单点方式对待
	if (channels_->first_node() == NULL) {
		(void) start_channel(addr_);
	}
}

redis_pipeline_channel* redis_client_pipeline::start_channel(const char *addr)
{
	redis_pipeline_channel* channel = NEW redis_pipeline_channel(
		*this, addr, conn_timeout_, rw_timeout_, retry_);
	if (!passwd_.empty()) {
		channel->set_passwd(passwd_);
	}
	if (channel->start_thread()) {
		channels_->insert(addr, channel);
		return channel;
	} else {
		delete channel;
		return NULL;
	}
}

void redis_client_pipeline::stop_channel(const char *addr)
{
	const token_node* node = channels_->find(addr);
	if (node) {
		redis_pipeline_channel* channel = (redis_pipeline_channel*)
			node->get_ctx();
		channels_->remove(addr);
		channel->stop_thread();
	}
}

redis_pipeline_channel* redis_client_pipeline::get_channel(int slot)
{
	const char* addr;
	// first, get one addr of cluster mode when slot is valid
	if (slot >= 0 && slot < (int) max_slot_) {
		addr = slot_addrs_[slot];
		if (addr == NULL) {
			addr = addr_.c_str();
		}
	}
	// then, use the default addr for mode in cluster or alone
	else {
		addr = addr_.c_str();
	}

	const token_node* node = channels_->find(addr);
	if (node) {
		return (redis_pipeline_channel*) node->get_ctx();
	}

	return start_channel(addr);
}

} // namespace acl

#endif // ACL_CLIENT_ONLY
