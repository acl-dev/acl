#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/thread.hpp"
#include "../stdlib/string.hpp"
#include "../stdlib/tbox.hpp"
#include "../stdlib/mbox.hpp"
#include "redis_command.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl {

#define USE_MBOX

#ifdef USE_MBOX
# define BOX	mbox
#else
# define BOX	tbox
#endif

class token_tree;
class redis_client;

typedef enum {
	redis_pipeline_t_cmd,		// Redis command type
	redis_pipeline_t_redirect,	// Should redirect to another node
 	redis_pipeline_t_clusterdonw,	// The redis node has been down
	redis_pipeline_t_stop,		// The current channel should stop
} redis_pipeline_type_t;

/**
 * The message for transfering between redis command, redis client pipline
 * and redis pipeline channel, which holds the redis command.
 */
class redis_pipeline_message {
public:
	redis_pipeline_message(redis_command* cmd, redis_pipeline_type_t type,
		bool use_mbox = true)
	: cmd_(cmd)
	, type_(type)
	, nchild_(0)
	, timeout_(NULL)
	, result_(NULL)
	, addr_(NULL)
	, redirect_count_(0)
	, argc_(0)
	, argv_(NULL)
	, lens_(NULL)
	{
		if (use_mbox) {
			mbox_ = new mbox<redis_pipeline_message>(false, false);
			tbox_ = NULL;
		} else {
			tbox_ = new tbox<redis_pipeline_message>(false);
			mbox_ = NULL;
		}

		size_ = 10;
		argc_ = 0;
		argv_ = new const char* [size_];
		lens_ = new size_t [size_];
	}

	~redis_pipeline_message(void) {
		delete mbox_;
		delete tbox_;
		delete [] argv_;
		delete [] lens_;
	}

	redis_pipeline_message& set_type(redis_pipeline_type_t type) {
		type_ = type;
		return *this;
	}

	redis_pipeline_type_t get_type(void) const {
		return type_;
	}

	redis_command* get_cmd(void) {
		return cmd_;
	}

	void set_option(size_t nchild, int* timeout) {
		nchild_  = nchild;
		timeout_ = timeout;
		result_  = NULL;
		addr_    = NULL;
		redirect_count_ = 0;
	}

	void set_request(size_t argc, const char** argv, size_t* lens) {
		// When running in coroutine of shared stack mode,
		// the variables on stack are volatile, so we should save
		// the request data in heap.
#if 0
		argc_    = argc;
		argv_    = argv;
		lens_    = lens;
#else
		if (argc > size_) {
			delete [] argv_;
			delete [] lens_;
			size_ = argc;
			argv_ = new const char* [size_];
			lens_ = new size_t [size_];
		}

		argc_ = argc;
		for (size_t i = 0; i < size_; i++) {
			argv_[i] = argv[i];
			lens_[i] = lens[i];
		}
#endif
	}

	void set_addr(const char* addr) {
		addr_ = addr;
		if (addr) {
			redirect_count_++;
		}
	}

	size_t get_nchild(void) const {
		return nchild_;
	}

	int* get_timeout(void) const {
		return timeout_;
	}

	void push(const redis_result* result) {
		result_ = result;
		if (mbox_) {
			mbox_->push(this, false);
		} else {
			tbox_->push(this, false);
		}
	}

	const redis_result* wait(void) {
		if (mbox_) {
			mbox_->pop();
		} else {
			tbox_->pop();
		}
		return result_;
	}

	const char* get_addr(void) const {
		return addr_;
	}

	size_t get_redirect_count(void) const {
		return redirect_count_;
	}

private:
	redis_command* cmd_;
	redis_pipeline_type_t type_;
	size_t nchild_;
	int* timeout_;
	mbox<redis_pipeline_message>* mbox_;
	tbox<redis_pipeline_message>* tbox_;

	const redis_result* result_;
	const char* addr_;
	size_t redirect_count_;

public:
	size_t       size_;
	size_t       argc_;
	const char** argv_;
	size_t*      lens_;
};

class redis_client_pipeline;

/**
 * One pipeline channel thread for one redis node, which waits for message
 * from pipline thread and try to combine more messages and sends to redis.
 */
class redis_pipeline_channel : public thread {
public:
	redis_pipeline_channel(redis_client_pipeline& pipeline,
		const char* addr, int conn_timeout, int rw_timeout, bool retry);
	~redis_pipeline_channel(void);

	bool start_thread(void);
	void stop_thread(void);

public:
	redis_pipeline_channel& set_passwd(const char* passwd);
	const char* get_addr(void) const {
		return addr_.c_str();
	}

protected:
	// @override from acl::thread
	void* run(void);

private:
	redis_client_pipeline& pipeline_;
	string addr_;
	string buf_;
	redis_client* client_;
	BOX<redis_pipeline_message> box_;
	std::vector<redis_pipeline_message*> msgs_;
public:
	void push(redis_pipeline_message* msg);

private:
	bool handle_messages(void);
	bool flush_all(void);
	bool wait_results(void);
	bool wait_one(socket_stream& conn, redis_pipeline_message& msg);
	void all_failed(void);
};

/**
 * Redis pipline communication, be set and used in redis_command to
 * improve the performance of redis commands, but not all redis commands
 * in acl can be used in pipeline mode, such as below:
 * 1. multiple keys operation
 * 2. blocked operation such as SUBSCRIBE in pubsub, BLPOP in list
 */
class ACL_CPP_API redis_client_pipeline : public thread {
public:
	redis_client_pipeline(const char* addr);
	~redis_client_pipeline(void);

	// Start the pipeline thread
	void start_thread(void);

	// Stop the pipeline thread
	void stop_thread(void);

public:
	// Called by redis_command in pipeline mode
	const redis_result* run(redis_pipeline_message& msg);

	// Called by redis_pipeline_channel
	void push(redis_pipeline_message* msg);

public:
	// Set the password for connecting the redis server
	redis_client_pipeline& set_password(const char* passwd);

	// Set network IO timeout
	redis_client_pipeline& set_timeout(int conn_timeout, int rw_timeout);

	// Set if retry on IO failed in redis_client
	redis_client_pipeline& set_retry(bool on);

	// Set the max hash slot of redis, the default valud is 16384
	redis_client_pipeline& set_max_slot(int max_slot);

	// Set if connecting all the redis nodes after starting
	redis_client_pipeline& set_preconnect(bool yes);

	// Get the max hash slot of redis
	int get_max_slot(void) const {
		return max_slot_;
	}

protected:
	// @override from acl::thread
	void* run(void);

private:
	string addr_;		// The default redis address
	string passwd_;		// Password for connecting redis
	int max_slot_;		// The max hash slot for redis cluster
	int    conn_timeout_;	// Timeout to connect redis
	int    rw_timeout_;	// IO timeout with redis
	bool   retry_;		// If try again when disconnect from redis
	bool   preconn_;	// If connecting all redis nodes when starting

	token_tree* channels_;	// holds and manage all pipeline channels

	// The message queue for receiving redis message from other threads
	BOX<redis_pipeline_message> box_;

	std::vector<char*> addrs_;	// Hold all redis's addresses
	const char** slot_addrs_;	// Map hash slot with address

	// Set the hash slot with the specified redis address
	void set_slot(int slot, const char* addr);

	// Set all hash slots' addresses of all redises
	void set_all_slot(void);

	// Start all pipeline channels threads
	void start_channels(void);

	// Stop all pipeline channels threads
	void stop_channels(void);

	// Start one pipeline channel thread with the specified redis address
	redis_pipeline_channel* start_channel(const char* addr);

	// Stop one pipeline channel thread with the specified redis address
	void stop_channel(const char* addr);

	// Get one pipeline channel thread with the specified hash slot
	redis_pipeline_channel* get_channel(int slot);

	// Redirect one slot to another redis address
	void redirect(const redis_pipeline_message& msg, int slot);

	// When one redis node down, we should clear the node's hash slot map
	// and stop the pipeline channel thread
	void cluster_down(const redis_pipeline_message& msg);
};

/**
 * Sample:
 * void main_thread(void) {
 *	acl::redis_client_pipeline pipeline("127.0.0.1:6379");
 *	pipeline.start_thread();
 *	// Start some threads
 *	...
 *	// Wait for thease threads to exit and stop pipeline thread.
 *	pipeline.stop_thread();
 * }
 * // Execute redis command in one thread
 * void test_thread(acl::redis_client_pipeline& pipeline) {
 *	acl::redis cmd;
 *	cmd.set_pipeline(&pipeline);
 *	acl::string key;
 *	for (size_t i = 0; i < 100000; i++) {
 *		key.format("test-key-%d", (int) i);
 *		cmd.del(key);
 *	}
 */
} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)
