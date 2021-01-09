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
	redis_pipeline_t_cmd,		// redis command type
	redis_pipeline_t_redirect,	// should redirect to another node
 	redis_pipeline_t_clusterdonw,	// the redis node has been down
	redis_pipeline_t_stop,		// the current channel should stop
} redis_pipeline_type_t;

/**
 * the message for transfering between redis command, redis client pipline
 * and redis pipeline channel, which holds the redis command or not.
 */
class redis_pipeline_message {
public:
	redis_pipeline_message(redis_command* cmd, redis_pipeline_type_t type)
	: cmd_(cmd)
	, type_(type)
	, nchild_(0)
	, timeout_(NULL)
	, box_(false)
	, result_(NULL)
	, addr_(NULL)
	, redirect_count_(0)
	, argc_(0)
	, argv_(NULL)
	, lens_(NULL)
	{}

	~redis_pipeline_message(void) {}

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
		argc_    = argc;
		argv_    = argv;
		lens_    = lens;
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
		box_.push(this);
	}

	const redis_result* wait(void) {
		box_.pop();
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
	mbox<redis_pipeline_message> box_;

	const redis_result* result_;
	const char* addr_;
	size_t redirect_count_;

public:
	size_t       argc_;
	const char** argv_;
	size_t*      lens_;
};

class redis_client_pipeline;

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
 * redis pipline communication, be set and used in redis_command to
 * improve the performance of redis commands, but not all redis commands
 * in acl can be used in pipeline mode, such as below:
 * 1. multiple keys operation
 * 2. blocked operation such as SUBSCRIBE in pubsub, BLPOP in list
 */
class ACL_CPP_API redis_client_pipeline : public thread {
public:
	redis_client_pipeline(const char* addr);
	~redis_client_pipeline(void);

	// start the pipeline thread
	void start_thread(void);

	// stop the pipeline thread
	void stop_thread(void);

public:
	// called by redis_command in pipeline mode
	const redis_result* run(redis_pipeline_message& msg);

	// called by redis_pipeline_channel
	void push(redis_pipeline_message* msg);

public:
	// set the password for connecting the redis server
	redis_client_pipeline& set_password(const char* passwd);

	// set network IO timeout
	redis_client_pipeline& set_timeout(int conn_timeout, int rw_timeout);

	// set if retry on IO failed in redis_client
	redis_client_pipeline& set_retry(bool on);

	// set the max hash slot of redis, the default valud is 16384
	redis_client_pipeline& set_max_slot(size_t max_slot);

	// set if connecting all the redis nodes after starting
	redis_client_pipeline& set_preconnect(bool yes);

	// get the max hash slot of redis
	size_t get_max_slot(void) const {
		return max_slot_;
	}

protected:
	// @override from acl::thread
	void* run(void);

private:
	string addr_;		// the default redis address
	string passwd_;		// password for connecting redis
	size_t max_slot_;	// the max hash slot for redis cluster
	int    conn_timeout_;	// timeout to connect redis
	int    rw_timeout_;	// IO timeout with redis
	bool   retry_;		// if try again when disconnect from redis
	bool   preconn_;	// if connecting all redis nodes when starting

	token_tree* channels_;	// holds and manage all pipeline channels

	// the message queue for receiving redis message from other threads
	BOX<redis_pipeline_message> box_;

	std::vector<char*> addrs_;	// hold all redises addresses
	const char** slot_addrs_;	// map hash slot with address

	// set the hash slot with the specified redis address
	void set_slot(size_t slot, const char* addr);

	// set all hash slots' addresses of all redises
	void set_all_slot(void);

	// start all pipeline channels threads
	void start_channels(void);

	// stop all pipeline channels threads
	void stop_channels(void);

	// start one pipeline channel thread with the specified redis address
	redis_pipeline_channel* start_channel(const char* addr);

	// stop one pipeline channel thread with the specified redis address
	void stop_channel(const char* addr);

	// get one pipeline channel thread with the specified hash slot
	redis_pipeline_channel* get_channel(int slot);

	// redirect one slot to another redis address
	void redirect(const redis_pipeline_message& msg, int slot);

	// when one redis node down, we should clear the node's hash slot map
	// and stop the pipeline channel thread
	void cluster_down(const redis_pipeline_message& msg);
};

/**
 * sample:
 * void main_thread(void) {
 *	acl::redis_client_pipeline pipeline("127.0.0.1:6379");
 *	pipeline.start_thread();
 *	// start some threads
 *	...
 * }
 * // execute redis command in one thread
 * void test_thread(acl::redis_client_pipeline& pipeline) {
 *	acl::redis cmd;
 *	cmd.set_pipeline(&pipeline);
 *	for (size_t i = 0; i < 100000; i++) {
 *		cmd.del("test_key");
 *	}
 */
} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)
