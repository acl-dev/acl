#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/stdlib/noncopyable.hpp"

struct ACL_FIBER_SEM;

namespace acl {

class fiber_sem : public noncopyable
{
public:
	fiber_sem(int max);
	~fiber_sem(void);

	int wait(void);
	int trywait(void);
	int post(void);

private:
	ACL_FIBER_SEM* sem_;
};

class fiber_sem_guard
{
public:
	fiber_sem_guard(fiber_sem& sem) : sem_(sem)
	{
		(void) sem_.wait();
	}

	~fiber_sem_guard(void)
	{
		sem_.post();
	}

private:
	fiber_sem& sem_;
};

} // namespace acl
