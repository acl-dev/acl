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
	redis_pipeline_message(redis_command* cmd, size_t nchild,
		int* timeout, mbox<redis_pipeline_message>& box)
	: cmd_(cmd)
	, nchild_(nchild)
	, timeout_(timeout)
	, box_(box)
	, result_(NULL)
	{}

	~redis_pipeline_message(void) {}

	redis_command* cmd_;
	size_t nchild_;
	int* timeout_;
	mbox<redis_pipeline_message>& box_;

	const redis_result* result_;
};

class redis_reader : public thread {
public:
	redis_reader(redis_client& conn);
	~redis_reader(void);

	void push(redis_pipeline_message* msg);

protected:
	// @override
	void* run(void);

private:
	BOX<redis_pipeline_message> box_;
	redis_client& conn_;
};

class ACL_CPP_API redis_client_pipeline : public thread {
public:
	redis_client_pipeline(const char* addr, int conn_timeout = 60,
		int rw_timeout = 30, bool retry = true);
	~redis_client_pipeline(void);

	void push(redis_pipeline_message* msg);

	const redis_result* run(redis_command* cmd, size_t nchild, int* timeout);

protected:
	// @override
	void* run(void);

private:
	string addr_;
	int conn_timeout_;
	int rw_timeout_;
	bool retry_;
	redis_client* conn_;

	redis_reader* reader_;

	BOX<redis_pipeline_message> box_;

	void send(std::vector<redis_pipeline_message*>& msgs);
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)
