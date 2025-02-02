#include "stdafx.hpp"
#include "fiber/fiber_sem.hpp"

namespace acl {

fiber_sem::fiber_sem(size_t max, fiber_sem_attr_t attr)
{
	unsigned flags = 0;

	if (attr & fiber_sem_t_async) {
		flags |= ACL_FIBER_SEM_F_ASYNC;
	}

	sem_ = acl_fiber_sem_create2(max, flags);
}

fiber_sem::fiber_sem(size_t max, size_t buf)
{
	sem_ = acl_fiber_sem_create3((int) max, (int) buf, 0);
}

fiber_sem::~fiber_sem()
{
	acl_fiber_sem_free(sem_);
}

int fiber_sem::wait(int ms)
{
	return acl_fiber_sem_timed_wait(sem_, ms);
}

int fiber_sem::trywait()
{
	return acl_fiber_sem_trywait(sem_);
}

int fiber_sem::post()
{
	return acl_fiber_sem_post(sem_);
}

size_t fiber_sem::num() const
{
	return (size_t) acl_fiber_sem_num(sem_);
}

}
