#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include "stdlib/acl_mymalloc.h"
#include "thread/acl_pthread.h"
#include "stdlib/acl_atomic.h"

#endif

#if	defined(ACL_WINDOWS) || defined(ACL_LINUX)
# define HAS_ATOMIC
#else
# undef  HAS_ATOMIC
#endif

struct ACL_ATOMIC {
	void *value;
#ifndef HAS_ATOMIC
	acl_pthread_mutex_t lock;
#endif
};

ACL_ATOMIC *acl_atomic_new(void)
{
	ACL_ATOMIC *self = (ACL_ATOMIC*) acl_mymalloc(sizeof(ACL_ATOMIC));

#ifndef HAS_ATOMIC
	acl_pthread_mutex_init(&self->lock, NULL);
#endif
	self->value = NULL;
	return self;
}

void acl_atomic_free(ACL_ATOMIC *self)
{
	self->value = NULL;
#ifndef HAS_ATOMIC
	acl_pthread_mutex_destroy(&self->lock);
	acl_myfree(self);
#endif
}

void acl_atomic_set(ACL_ATOMIC *self, void *value)
{
#ifndef HAS_ATOMIC
	acl_pthread_mutex_lock(&self->lock);
	self->value = value;
	acl_pthread_mutex_unlock(&self->lock);
#elif	defined(ACL_WINDOWS)
	InterlockedExchangePointer((volatile PVOID*) &self->value, value);
#elif	defined(ACL_LINUX)
	(void) __sync_lock_test_and_set(&self->value, value);
#endif
}

void *acl_atomic_cas(ACL_ATOMIC *self, void *cmp, void *value)
{
#ifndef HAS_ATOMIC
	void *old;

	acl_pthread_mutex_lock(&self->lock);
	old = self->value;
	if (self->value == cmp)
		self->value = value;
	acl_pthread_mutex_unlock(&self->lock);

	return old;
#elif	defined(ACL_WINDOWS)
	return InterlockedCompareExchangePointer(
		(volatile PVOID*)&self->value, value, cmp);
#elif	defined(ACL_LINUX)
	return __sync_val_compare_and_swap(&self->value, cmp, value);
#endif
}

void * acl_atomic_xchg(ACL_ATOMIC *self, void *value)
{
#ifndef HAS_ATOMIC
	void *old;

	acl_pthread_mutex_lock(&self->lock);
	old = self->value;
	self->value = value;
	acl_pthread_mutex_unlock(&self->lock);

	return old;
#elif	defined(ACL_WINDOWS)
	return InterlockedExchangePointer((volatile PVOID*)&self->value, value);
#elif	defined(ACL_LINUX)
	return __sync_lock_test_and_set(&self->value, value);
#endif
}
