#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/atomic.hpp"
#include "../stdlib/noncopyable.hpp"

namespace acl
{

class ACL_CPP_API event_mutex : public noncopyable
{
public:
	event_mutex(bool recursive = true);
	~event_mutex(void);

	bool lock(void);
	bool unlock(void);

private:
	bool recursive_;
	unsigned int nested_;
#if defined(_WIN32) || defined(_WIN64)
	SOCKET in_;
	SOCKET out_;
#else
	int    in_;
	int    out_;
#endif
	atomic_long count_;
	unsigned long tid_;
};

} // namespace acl
