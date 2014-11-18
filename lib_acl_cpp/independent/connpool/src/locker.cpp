#include <assert.h>
#include "locker.hpp"

namespace acl_min {

locker::locker(bool nowait /* = false */)
: nowait_(nowait)
{
	init_mutex();
}

locker::~locker()
{
	pthread_mutexattr_destroy(&mutexAttr_);
	pthread_mutex_destroy(pMutex_);
	free(pMutex_);
}

void locker::init_mutex()
{
	pMutex_ = (pthread_mutex_t*) calloc(1, sizeof(pthread_mutex_t));
	pthread_mutexattr_init(&mutexAttr_);
	pthread_mutexattr_settype(&mutexAttr_, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(pMutex_, &mutexAttr_);
}

bool locker::lock()
{
	if (nowait_)
		return pthread_mutex_trylock(pMutex_) == -1 ? false : true;
	else
		return pthread_mutex_lock(pMutex_) == -1 ? false : true;
}

bool locker::unlock()
{
	return pthread_mutex_unlock(pMutex_) == -1 ? false : true;
}

} // namespace acl_min
