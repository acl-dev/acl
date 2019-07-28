#include "stdafx.hpp"
#include "fiber/fiber_sem.hpp"

namespace acl {

fiber_sem::fiber_sem(int max)
{
	sem_ = acl_fiber_sem_create(max);
}

fiber_sem::~fiber_sem(void)
{
	acl_fiber_sem_free(sem_);
}

int fiber_sem::wait(void)
{
	return acl_fiber_sem_wait(sem_);
}

int fiber_sem::trywait(void)
{
	return acl_fiber_sem_trywait(sem_);
}

int fiber_sem::post(void)
{
	return acl_fiber_sem_post(sem_);
}

}
