#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>

#include "memory.h"
#include "msg.h"
#include "atomic.h"

struct ATOMIC {
	void *value;
};

ATOMIC *atomic_new(void)
{
	ATOMIC *self = (ATOMIC*) mem_malloc(sizeof(ATOMIC));

	self->value = NULL;
	return self;
}

void atomic_free(ATOMIC *self)
{
	self->value = NULL;
	mem_free(self);
}

void atomic_set(ATOMIC *self, void *value)
{
#if defined(__GNUC__) && (__GNUC__ >= 4)
	(void) __sync_lock_test_and_set(&self->value, value);
#elif defined(SYS_WIN)
	InterlockedExchangePointer((volatile PVOID*) &self->value, value);
#else
	(void) self;
	(void) value;
	msg_error("%s(%d), %s: not support!", __FILE__, __LINE__, __FUNCTION__);
#endif
}

void *atomic_cas(ATOMIC *self, void *cmp, void *value)
{
#if defined(__GNUC__) && (__GNUC__ >= 4)
	return __sync_val_compare_and_swap(&self->value, cmp, value);
#elif defined(SYS_WIN)
	return InterlockedCompareExchangePointer(
		(volatile PVOID*)&self->value, value, cmp);
#else
	(void) self;
	(void) cmp;
	(void) value;
	msg_error("%s(%d), %s: not support!",
		 __FILE__, __LINE__, __FUNCTION__);
	return NULL;
#endif
}

void *atomic_xchg(ATOMIC *self, void *value)
{
#if defined(__GNUC__) && (__GNUC__ >= 4)
	return __sync_lock_test_and_set(&self->value, value);
#elif defined(SYS_WIN)
	return InterlockedExchangePointer((volatile PVOID*)&self->value, value);
#else
	(void) self;
	(void) value;
	msg_error("%s(%d), %s: not support!",
		 __FILE__, __LINE__, __FUNCTION__);
	return NULL;
#endif
}

void atomic_int64_set(ATOMIC *self, long long n)
{
#if defined(__GNUC__) && (__GNUC__ >= 4)
	(void) __sync_lock_test_and_set((long long *) self->value, n);
#elif defined(SYS_WIN)
	InterlockedExchangePointer((volatile PVOID*) self->value, (PVOID) n);
#else
	(void) self;
	(void) n;
	msg_error("%s(%d), %s: not support!",
		 __FILE__, __LINE__, __FUNCTION__);
#endif
}

long long atomic_int64_fetch_add(ATOMIC *self, long long n)
{
#if defined(__GNUC__) && (__GNUC__ >= 4)
	return (long long) __sync_fetch_and_add((long long *) self->value, n);
#elif defined(SYS_WIN)
	long long k = InterlockedExchangeAdd64(
		(volatile LONGLONG*) self->value, n);
	return k - n;
#else
	(void) self;
	(void) n;
	msg_error("%s(%d), %s: not support!",
		__FILE__, __LINE__, __FUNCTION__);
	return -1;
#endif
}

long long atomic_int64_add_fetch(ATOMIC *self, long long n)
{
#if defined(__GNUC__) && (__GNUC__ >= 4)
	return (long long) __sync_add_and_fetch((long long *) self->value, n);
#elif defined(SYS_WIN)
	return InterlockedExchangeAdd64((volatile LONGLONG*) self->value, n);
#else
	(void) self;
	(void) n;
	msg_error("%s(%d), %s: not support!",
		__FILE__, __LINE__, __FUNCTION__);
	return -1;
#endif
}

long long atomic_int64_cas(ATOMIC *self, long long cmp, long long n)
{
#if defined(__GNUC__) && (__GNUC__ >= 4)
	return (long long) __sync_val_compare_and_swap(
			(long long*) self->value, cmp, n);
#elif defined(SYS_WIN)
	return InterlockedCompareExchange64(
		(volatile LONGLONG*)&self->value, n, cmp);
#else
	(void) self;
	(void) cmp;
	(void) n;
	msg_error("%s(%d), %s: not support!",
		__FILE__, __LINE__, __FUNCTION__);
	return -1;
#endif
}
