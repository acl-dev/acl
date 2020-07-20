#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/thread.hpp"
#include "../stdlib/string.hpp"
#include "../stdlib/tbox.hpp"
#include "redis_command.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl {

class redis_reader : public thread {
public:
	redis_reader(tbox<redis_command>& box);
	~redis_reader(void);

protected:
	// @override
	void* run(void);

private:
	tbox<redis_command>& box_;
};

class ACL_CPP_API redis_client_pipeline : public thread {
public:
	redis_client_pipeline(const char* addr, int conn_timeout = 60,
		int rw_timeout = 30, bool retry = true);
	~redis_client_pipeline(void);

	void push(redis_command* command);

protected:
	// @override
	void* run(void);

private:
	string addr_;
	int conn_timeout_;
	int rw_timeout_;
	bool retry_;

	redis_reader* reader_;

	tbox<redis_command> box_;

	void send(std::vector<redis_command*>& cmds);
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)
