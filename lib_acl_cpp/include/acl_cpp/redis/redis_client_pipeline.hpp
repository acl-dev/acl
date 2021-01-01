#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/thread.hpp"
#include "../stdlib/string.hpp"
#include "../stdlib/tbox.hpp"
#include "../stdlib/mbox.hpp"
#include "redis_command.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl {

//#define DEBUG_BOX
//#define USE_MBOX

#ifdef USE_MBOX
# define BOX	mbox
#else
# define BOX	tbox
#endif

class redis_client;

class redis_pipeline_message {
public:
	redis_pipeline_message(redis_command* cmd)
	: cmd_(cmd)
	, nchild_(0)
	, timeout_(NULL)
	, box_(false)
	, result_(NULL)
	{}

	~redis_pipeline_message(void) {}

	redis_command& get_cmd(void) {
		return *cmd_;
	}

	void set_option(size_t nchild, int* timeout) {
		nchild_ = nchild;
		timeout_ = timeout;
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

private:
	redis_command* cmd_;
	size_t nchild_;
	int* timeout_;
	mbox<redis_pipeline_message> box_;

	const redis_result* result_;
};

class redis_pipeline_channel : public thread {
public:
	redis_pipeline_channel(const char* addr, int conn_timeout,
		int rw_timeout, bool retry);
	~redis_pipeline_channel(void);

	void push(redis_pipeline_message* msg);
	void flush(void);

	bool start_thread(void);

protected:
	// @override
	void* run(void);

private:
	string addr_;
	string buf_;
	redis_client* conn_;
	BOX<redis_pipeline_message> box_;
	std::vector<redis_pipeline_message*> msgs_;
};

class ACL_CPP_API redis_client_pipeline : public thread {
public:
	redis_client_pipeline(const char* addr);
	~redis_client_pipeline(void);

	void push(redis_pipeline_message* msg);

	const redis_result* run(redis_pipeline_message& msg);

public:
	redis_client_pipeline& set_timeout(int conn_timeout, int rw_timeout);
	redis_client_pipeline& set_retry(bool on);
	redis_client_pipeline& set_channels(size_t n);

protected:
	// @override
	void* run(void);

private:
	string addr_;
	int    conn_timeout_;
	int    rw_timeout_;
	bool   retry_;
	size_t nchannels_;

	std::vector<redis_pipeline_channel*> channels_;
	BOX<redis_pipeline_message> box_;

	void flush_all(void);
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)
