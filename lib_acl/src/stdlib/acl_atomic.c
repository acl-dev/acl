#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include "stdlib/acl_mymalloc.h"
#include "thread/acl_pthread.h"
#include "stdlib/acl_atomic.h"

#endif

struct ACL_ATOMIC {
	void *value;
#ifndef ACL_WINDOWS
	acl_pthread_mutex_t lock;
#endif
};

ACL_ATOMIC *acl_atomic_new(void)
{
	ACL_ATOMIC *self = (ACL_ATOMIC*) acl_mymalloc(sizeof(ACL_ATOMIC));

#ifndef ACL_WINDOWS
	acl_pthread_mutex_init(&self->lock, NULL);
#endif
	self->value = NULL;
	return self;
}

void acl_atomic_free(ACL_ATOMIC *self)
{
#ifdef ACL_WINDOWS
	self->value = NULL;
#else
	acl_pthread_mutex_destroy(&self->lock);
	acl_myfree(self);
#endif
}

void acl_atomic_set(ACL_ATOMIC *self, void *value)
{
#ifdef ACL_WINDOWS
	InterlockedExchangePointer((volatile PVOID*)&self->value, value);
#else
	acl_pthread_mutex_lock(&self->lock);
	self->value = value;
	acl_pthread_mutex_unlock(&self->lock);
#endif
}

void *acl_atomic_cas(ACL_ATOMIC *self, void *cmp, void *value)
{
#ifdef ACL_WINDOWS
	return InterlockedCompareExchangePointer(
		(volatile PVOID*)&self->value, value, cmp);
#else 
	void *old = self->value;

	acl_pthread_mutex_lock(&self->lock);
	if (self->value == cmp)
		self->value = value;
	acl_pthread_mutex_unlock(&self->lock);

	return old;
#endif
}

void * acl_atomic_xchg(ACL_ATOMIC *self, void *value)
{
#ifdef ACL_WINDOWS
	return InterlockedExchangePointer((volatile PVOID*)&self->value, value);
#else

	void *old = self->value;
	acl_pthread_mutex_lock(&self->lock);
	self->value = value;
	acl_pthread_mutex_unlock(&self->lock);

	return old;
#endif
}
