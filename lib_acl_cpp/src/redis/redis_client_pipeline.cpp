#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/token_tree.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_command.hpp"
#include "acl_cpp/redis/redis_cluster.hpp"
#include "acl_cpp/redis/redis_client_pipeline.hpp"
#endif

#include "acl_cpp/stdlib/class_counter.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl {

redis_pipeline_message::redis_pipeline_message(redis_pipeline_type_t type,
	box<redis_pipeline_message>* box)
: type_(type)
, box_(box)
, timeout_(-1)
, nchild_(0)
, dbuf_(NULL)
, req_(NULL)
, result_(NULL)
, slot_(-1)
, redirect_count_(0)
, channel_(NULL)
{
	++refers_;
	COUNTER_INC(redis_pipeline_message);
}

redis_pipeline_message::~redis_pipeline_message() {
	delete box_;
	COUNTER_DEC(redis_pipeline_message);
}

void redis_pipeline_message::set_option(size_t nchild, const int* timeout) {
	nchild_  = nchild;
	timeout_ = timeout ? *timeout : -1;
	result_  = NULL;
	redirect_count_ = 0;
	addr_.clear();
}

void redis_pipeline_message::set_slot(size_t slot) {
	slot_ = slot;
}

void redis_pipeline_message::set(const string* req) {
	req_ = req;
}

void redis_pipeline_message::set(dbuf_pool* dbuf) {
	dbuf_ = dbuf;
}

void redis_pipeline_message::set_addr(const char* addr) {
	if (addr) {
		addr_ = addr;
		redirect_count_++;
	}
}

void redis_pipeline_message::push(const redis_result* result) {
	// One reference for the message itself, and the other one was added
	// in redis_client_pipeline::exec().

	result_ = result;
	box_->push(this, false);

	// Just decrease the reference added in redis_client_pipeline::exec().
	unrefer();
}

const redis_result* redis_pipeline_message::wait() const {
	box_->pop(-1, NULL);
	return result_;
}

//////////////////////////////////////////////////////////////////////////////

redis_pipeline_channel::redis_pipeline_channel(redis_client_pipeline& pipeline,
	const char* addr, int conn_timeout, int rw_timeout, bool retry)
: pipeline_(pipeline)
, addr_(addr)
, buf_(81920)
{
	client_ = NEW redis_client(addr, conn_timeout, rw_timeout, retry);
	box_ = NEW mbox<redis_pipeline_message>;
}

redis_pipeline_channel::~redis_pipeline_channel()
{
	delete client_;
	delete box_;
}

redis_pipeline_channel &redis_pipeline_channel::set_ssl_conf(sslbase_conf *ssl_conf)
{
	client_->set_ssl_conf(ssl_conf);
	return *this;
}

redis_pipeline_channel& redis_pipeline_channel::set_passwd(const char *passwd)
{
	if (passwd && *passwd) {
		client_->set_password(passwd);
	}
	return *this;
}

bool redis_pipeline_channel::start_thread()
{
	if (!static_cast<connect_client *>(client_)->open()) {
		logger_error("open %s error %s", addr_.c_str(), last_serror());
		return false;
	}
	this->start();
	return true;
}

void redis_pipeline_channel::stop_thread()
{
	redis_pipeline_message *message = NEW
		redis_pipeline_message(redis_pipeline_t_stop, NULL);
	push(message);
	this->wait(); // Call the base class thread::wait().
	message->unrefer();
}

void redis_pipeline_channel::push(redis_pipeline_message* msg) const
{
	box_->push(msg, false);
}

bool redis_pipeline_channel::flush_requests()
{
	if (msgs_.empty()) {
		//logger("The messages are empty!");
		return true;
	}

	buf_.clear();

	for (std::list<redis_pipeline_message*>::iterator it = msgs_.begin();
		  it != msgs_.end(); ++it) {
		buf_.append((*it)->get_request());
	}

#if 0
	if (msgs_.size() > 10) {
		logger(">>>messages size is %zd, buf size=%zd<<<<",
			msgs_.size(), buf_.size());
	}
#endif

	bool retried = false;
	while (true) {
		socket_stream* conn = client_->get_stream(false);
		if (conn) {
			if (conn->write(buf_) == static_cast<int>(buf_.size())) {
				return true;
			}

			logger_error("Write error=%s, addr=%s, buf=%s",
				last_serror(), addr_.c_str(), buf_.c_str());
		}

		// Return false if we have retried
		if (retried) {
			logger_error("Retried failed!");
			return false;
		}

		// Close the old connection
		client_->close();

		// Reopen the new connection
		if (!static_cast<connect_client *>(client_)->open()) {
			logger_error("Reopen connection failed!");
			return false;
		}
		retried = true;
	}
}

const redis_result* redis_pipeline_channel::get_result(socket_stream& conn,
	redis_pipeline_message& msg) const
{
	dbuf_pool* dbuf = msg.get_dbuf();
	assert(dbuf);

	const int* timeout = msg.get_timeout();
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
		logger_error("Can't get result");
		return NULL;
	}

	return result;
}

bool redis_pipeline_channel::handle_result(redis_pipeline_message* msg,
	const redis_result* result) const
{
	int type = result->get_type();
	if (type == REDIS_RESULT_UNKOWN) {
		logger_warn("Unknown type=%d", (int) type);
		msg->push(result);
		return true;
	}
	if (type != REDIS_RESULT_ERROR) {
		msg->push(result);
		return true;
	}

#define	EQ(x, y) !strncasecmp((x), (y), sizeof(y) -1)

	const char* ptr = result->get_error();
	if (ptr == NULL || *ptr == 0) {
		logger_error("error info null");
		msg->push(result);
		return true;
	}

	if (EQ(ptr, "MOVED") || EQ(ptr, "ASK")) {
		dbuf_pool* dbuf = msg->get_dbuf();
		assert(dbuf);

		const char* addr = redis_command::get_addr(dbuf, ptr);
		if (addr == NULL) {
			logger_error("No redirect addr got");
			msg->push(result);
		} else if (msg->get_redirect_count() >= 5) {
			logger_error("Redirect count(%zd) exceed limit(5)",
				msg->get_redirect_count());
			msg->push(result);
		} else {
			msg->set_addr(addr);
			msg->set_type(redis_pipeline_t_redirect);

			// Transfer the msg back to the pipeline thread again.
			pipeline_.push(msg);
		}
		return true;
	}
	
	if (EQ(ptr, "CLUSTERDOWN")) {
		// Notify the waiter the result.
		msg->push(result);

		// And notify the pipeline thread the redis node down now,
		// the message created here will be deleted in pipeline thread.
		redis_pipeline_message* m = NEW redis_pipeline_message(
				redis_pipeline_t_clusterdonw, NULL);
		m->set_addr(this->get_addr());
		// Transfer the cluster down msg to the pipeline thread.
		pipeline_.push(m);
		return false; // Return false to break the loop process.
	}

	logger_error("Unknown error: %s", ptr);
	msg->push(result);
	return true;
}

bool redis_pipeline_channel::wait_results()
{
	if (msgs_.empty()) {
		return true;
	}

	socket_stream* conn = client_->get_stream(false);
	if (conn == NULL) {
		logger_error("Get_stream null");
		return false;
	}

	size_t n = 0;
	std::list<redis_pipeline_message*>::iterator it = msgs_.begin();

	while (it != msgs_.end()) {
		// The msg's reference has been increased in advance, so it's
		// safe to operate the msg.
		const redis_result* result = get_result(*conn, **it);
		if (result == NULL) {
			break;
		}

		n++;
		redis_pipeline_message* msg = *it;
		it = msgs_.erase(it);

		// Handle the result.
		if (!handle_result(msg, result)) {
			break;
		}
	}

	// If we can't get the first result, the socket maybe be disconnected,
	// so we should return false and retry again.
	if (n == 0) {
		logger_error("Get the first result failed, msgs: %s",
			msgs_.empty() ? "empty" : "not empty");
		return msgs_.empty();
	}

	// Return NULL for the left failed results
	for (; it != msgs_.end(); ++it) {
		(*it)->push(NULL);
	}

	// We'll return true even some messages were failed, because we've
	// handled some messages correctly. If return false, all the messages
	// will all be notified again including the handled messages.
	return true;
}

void redis_pipeline_channel::all_failed()
{
	for (std::list<redis_pipeline_message*>::iterator it = msgs_.begin();
		  it != msgs_.end(); ++it) {
		(*it)->push(NULL);
	}
}

bool redis_pipeline_channel::handle_messages()
{
	bool retried = false;

	while (true) {
		if (!flush_requests()) {
			logger_error("All failed ...");
			break;
		}

		if (wait_results()) {
			msgs_.clear();
			return true;
		}

		logger_warn("wait results failed, try again!");

		if (retried) {
			logger_error("Retried failed");
			break;
		}
		retried = true;

		client_->close();
		if (!static_cast<connect_client *>(client_)->open()) {
			logger_error("Reopen failed");
			break;
		}
		logger("Connection opened OK, addr=%s", client_->get_addr());
	}

	all_failed();
	msgs_.clear();
	return false;
}

void* redis_pipeline_channel::run()
{
	bool success;
	int timeout = -1;

	while (!client_->eof()) {
		// Get one message coming from redis_client_pipeline.
		redis_pipeline_message* msg = box_->pop(timeout, &success);

		if (msg != NULL) {
			timeout = 0;

			switch (msg->get_type()) {

			// Handle normal message for handling redis command.
			case redis_pipeline_t_cmd:
				msgs_.push_back(msg);
				break;

			// Handle stop message from redis_client_pipeline
			// and stop the channel.
			case redis_pipeline_t_stop:
				return NULL;

			// XXX: ignore other message type.
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

	logger("Channel thread exit now, addr=%s, connection=%s",
		addr_.c_str(), client_->eof() ? "closed" : "opened");

	redis_pipeline_message* m = NEW
		redis_pipeline_message(redis_pipeline_t_channel_closed, NULL);
	m->set_channel(this);
	pipeline_.push(m);
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////

redis_client_pipeline::redis_client_pipeline(const char* addr, box_type_t type)
: addr_(addr)
, ssl_conf_(NULL)
, box_type_(type)
, max_slot_(16384)
, conn_timeout_(10)
, rw_timeout_(10)
, retry_(true)
, preconn_(true)
{
	slot_addrs_ = static_cast<const char **>(acl_mycalloc(max_slot_, sizeof(char*)));
	channels_   = NEW token_tree;
	box_        = new mbox<redis_pipeline_message>;
}

redis_client_pipeline::~redis_client_pipeline()
{
	for (std::vector<char*>::iterator it = addrs_.begin();
		it != addrs_.end(); ++it) {
		acl_myfree(*it);
	}
	acl_myfree(slot_addrs_);

	const token_node *node = channels_->first_node();
	while (node) {
		redis_pipeline_channel *channel =
			static_cast<redis_pipeline_channel*>(node->get_ctx());
		delete channel;
		node = channels_->next_node();
	}
	delete channels_;
	delete box_;
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

redis_client_pipeline& redis_client_pipeline::set_ssl_conf(acl::sslbase_conf *ssl_conf)
{
	ssl_conf_ = ssl_conf;
	return *this;
}

redis_client_pipeline& redis_client_pipeline::set_password(const char* passwd)
{
	if (passwd && *passwd) {
		passwd_ = passwd;
	}
	return *this;
}

redis_client_pipeline & redis_client_pipeline::set_max_slot(int max_slot)
{
	max_slot_ = max_slot;
	return *this;
}

redis_client_pipeline & redis_client_pipeline::set_preconnect(bool yes)
{
	preconn_ = yes;
	return *this;
}

void redis_client_pipeline::start_thread()
{
	this->start();
}

void redis_client_pipeline::stop_thread()
{
	box<redis_pipeline_message>* box = NEW mbox<redis_pipeline_message>;
	redis_pipeline_message *msg =
		NEW redis_pipeline_message(redis_pipeline_t_stop, box);
	push(msg);
	this->wait();  // Wait for the thread to exit
	msg->unrefer();
}

const redis_result* redis_client_pipeline::exec(redis_pipeline_message* msg) const
{
	// The box in msg is not safety in the tail return after push back,
	// because the consumer may free the box in msg at once after getting
	// one result. See acl_ypipe_flush(), if the consumer frees
	// the mbox after acl_atomic_cas() but before acl_atomic_set(),
	// the producer will crash, so we should increase the msg's reference
	// here to make sure the box in msg is not freed before the producer
	// returns the result. And the msg's reference will be decreased
	// in redis_pipeline_channel::wait_results() after the result is pushed
	// to the channel's message queue.

	msg->refer();
	box_->push(msg, false);
	return msg->wait();
}

void redis_client_pipeline::push(redis_pipeline_message *msg) const
{
	box_->push(msg, false);
}

box<redis_pipeline_message>* redis_client_pipeline::create_box()
{
	switch (box_type_) {
	case BOX_TYPE_TBOX:
		return new tbox<redis_pipeline_message>(false);
	case BOX_TYPE_TBOX_ARRAY:
		return new tbox_array<redis_pipeline_message>(false);
	case BOX_TYPE_MBOX:
	default:
		return new mbox<redis_pipeline_message>(false, false);
	}
}

// Called after the thread started
// @override from acl::thread
void* redis_client_pipeline::run()
{
	set_all_slot();
	if (preconn_) {
		start_channels();
	}

	if (channels_->first_node() == NULL) {
		logger_warn("No channel created in the beginning!");
		// return NULL;
	}

	int  timeout = -1;
	bool flag;

	while (true) {
		// Get one message from the message queue, the timeout
		// is -1 means waiting forever, and the flag indicates
		// if one empty message has been got.
		redis_pipeline_message* msg = box_->pop(timeout, &flag);
		if (msg != NULL) {
			redis_pipeline_type_t type = msg->get_type();
			if (type == redis_pipeline_t_stop) {
				logger("Got stop message");
				break;
			}

			const size_t slot = msg->get_slot();

			// When the message was coming from redis_pipeline_channel,
			// the type should be redis_pipeline_t_clusterdonw,
			// redis_pipeline_t_channel_closed or redis_pipeline_t_redirect.
			// We should handle these three messages carefully.

			if (type == redis_pipeline_t_clusterdonw) {
				logger_error("Redis cluster down");
				cluster_down(*msg);
				// The msg was created in channel thread.
				msg->unrefer();
				continue;
			}

			if (type == redis_pipeline_t_channel_closed) {
				channel_closed(msg->get_channel());
				// The msg was created in channel thread.
				msg->unrefer();
				continue;
			}

			if (type == redis_pipeline_t_redirect) {
				// Reset to cmd type from redirect type which
				// will be used in redis_pipeline_channel::run().
				msg->set_type(redis_pipeline_t_cmd);
				redirect(*msg, slot);
			}

			const redis_pipeline_channel *channel = get_channel(slot);
			if (channel == NULL) {
				logger_error("Channel null, slot=%zd", slot);
				msg->push(NULL);
				timeout = -1;
			} else {
				// Push the message to the channel's message queue
				channel->push(msg);
				timeout = 0;
			}
		} else if (box_->has_null() && flag) {
			logger_error("Got empty message successfully");
			break;
		} else if (!box_->has_null() && !flag) {
			logger_error("Got message failed");
			break;
		} else {
			timeout = -1;
		}
	}

	// Stop all channels before the pipeline thread exit.
	stop_channels();
	return NULL;
}

void redis_client_pipeline::cluster_down(const redis_pipeline_message &msg)
{
	const std::string& addr = msg.get_addr();

	// Clear all slots' addrs same as the dead node
	for (size_t i = 0; i < max_slot_; i++) {
		if (addr == slot_addrs_[i]) {
			slot_addrs_[i] = NULL;
		}
	}

	// Reset the default addr which different from the dead node
	if (addr_ == addr) {
		for (std::vector<char*>::const_iterator it = addrs_.begin();
		     it != addrs_.end(); ++it) {
			if (addr == *it) {
				addr_ = *it;
				break;
			}
		}

		// Stop and remove the dead node
		logger("Stop one channel thread, addr=%s", addr.c_str());
		stop_channel(addr.c_str());
	}
}

void redis_client_pipeline::channel_closed(redis_pipeline_channel* channel) const
{
	if (channel == NULL) {
		logger_error("The channel null!");
		return;
	}

	const char* addr = channel->get_addr();
	const token_node* node = channels_->find(addr);

	if (node == NULL) {
		channel->wait(); // Wait the thread to exit.
		delete channel;
		return;
	}

	redis_pipeline_channel* chan =
		static_cast<redis_pipeline_channel*>(node->get_ctx());
	if (chan == NULL || chan != channel) {
		logger_warn("The channel=%p not mine=%p", channel, chan);
	}

	logger("The channel closed, addr=%s", addr);
	channels_->remove(addr);
	channel->wait();
	delete channel;
}

void redis_client_pipeline::redirect(const redis_pipeline_message &msg, size_t slot)
{
	const std::string& addr = msg.get_addr();
	set_slot(slot, addr.c_str());
}

void redis_client_pipeline::set_slot(size_t slot, const char* addr)
{
	if (slot >= max_slot_ || addr == NULL || *addr == 0) {
		return;
	}

	// Traverse all cached addresses, add them directly if they do not exist,
	// and then associate them with the slot.

	std::vector<char*>::const_iterator cit = addrs_.begin();
	for (; cit != addrs_.end(); ++cit) {
		if (strcmp(*cit, addr) == 0) {
			break;
		}
	}

	// Associate the slot with one address.
	if (cit != addrs_.end()) {
		slot_addrs_[slot] = *cit;
	} else {
		// The reason for using dynamic allocation is that when adding
		// objects to an array, regardless of how the array is
		// dynamically adjusted, the dynamic memory address to be added
		// is fixed. Therefore, the index address of slot_dedrs_
		// is relatively unchanged.
		char* buf = acl_mystrdup(addr);
		addrs_.push_back(buf);
		slot_addrs_[slot] = buf;
	}
}

void redis_client_pipeline::set_all_slot()
{
	redis_client client(addr_, 30, 60, false);

	if (!passwd_.empty()) {
		client.set_password(passwd_);
	}

	redis_cluster cluster(&client);

	const std::vector<redis_slot*>* slots = cluster.cluster_slots();
	if (slots == NULL) {
		logger("Can't get cluster slots");
		return;
	}

	for (std::vector<redis_slot*>::const_iterator cit = slots->begin();
		  cit != slots->end(); ++cit) {
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

void redis_client_pipeline::stop_channels() const
{
	const token_node* iter = channels_->first_node();
	std::vector<redis_pipeline_channel*> channels;
	while (iter) {
		redis_pipeline_channel* channel =
			static_cast<redis_pipeline_channel*>(iter->get_ctx());
		// Notify and wait for the channel thread to exit
		channel->stop_thread();
		channels.push_back(channel);
		iter = channels_->next_node();
	}

	// Delete all channels threads
	for (std::vector<redis_pipeline_channel*>::iterator
		   it = channels.begin(); it != channels.end(); ++it) {
		channels_->remove((*it)->get_addr());
		delete *it;
	}

	logger("All channels in pipeline have been stopped!");
}

void redis_client_pipeline::start_channels()
{
	for (std::vector<char*>::const_iterator cit = addrs_.begin();
		cit != addrs_.end(); ++cit) {
		if (start_channel(*cit) == NULL) {
			logger_error("start channel %s failed", *cit);
		}
	}

	// If the cluster node has been successfully added, it indicates that
	// it is in cluster mode and should be treated as such. Otherwise,
	// it should be treated as a single point.
	if (channels_->first_node() == NULL) {
		if (start_channel(addr_) == NULL) {
			logger_error("start channel %s failed", addr_.c_str());
		}
	}
}

redis_pipeline_channel* redis_client_pipeline::start_channel(const char *addr)
{
	redis_pipeline_channel* channel = NEW redis_pipeline_channel(
		*this, addr, conn_timeout_, rw_timeout_, retry_);

	channel->set_ssl_conf(ssl_conf_);

	if (!passwd_.empty()) {
		channel->set_passwd(passwd_);
	}

	if (channel->start_thread()) {
		channels_->insert(addr, channel);
		return channel;
	}
	delete channel;
	return NULL;
}

void redis_client_pipeline::stop_channel(const char *addr) const
{
	const token_node* node = channels_->find(addr);
	if (node) {
		redis_pipeline_channel* channel =
			static_cast<redis_pipeline_channel*>(node->get_ctx());
		channels_->remove(addr);
		channel->stop_thread();
		delete channel;
	}
}

redis_pipeline_channel* redis_client_pipeline::get_channel(size_t slot)
{
	const char* addr;
	// First, get one addr of cluster mode when slot is valid
	if (slot < max_slot_) {
		addr = slot_addrs_[slot];
		if (addr == NULL) {
			addr = addr_.c_str();
		}
	}
	// Then, use the default addr for mode in cluster or alone
	else {
		addr = addr_.c_str();
	}

	const token_node* node = channels_->find(addr);
	if (node) {
		return static_cast<redis_pipeline_channel *>(node->get_ctx());
	}

	return start_channel(addr);
}

} // namespace acl

#endif // ACL_CLIENT_ONLY
