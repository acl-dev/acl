#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>

#include "memory.h"
#include "msg.h"
#include "atomic.h"

/* don't use atomic for IOS and MacOS */
#if defined(MACOSX)
#  undef HAS_ATOMIC
#endif

#if defined(_WIN32) || defined(_WIN64)
# ifndef WINDOWS
#  define WINDOWS
# endif
#endif

#if defined(WINDOWS)
# define HAS_ATOMIC
#endif

/*
#if !defined(HAS_ATOMIC)
# pragma message "Atomic not support, using thread mutex instead!"
#endif
*/

struct ATOMIC {
	void *value;
#ifndef HAS_ATOMIC
	pthread_mutex_t lock;
#endif
};

ATOMIC *atomic_new(void)
{
	ATOMIC *self = (ATOMIC*) mem_malloc(sizeof(ATOMIC));

#ifndef HAS_ATOMIC
	pthread_mutex_init(&self->lock, NULL);
#endif
	self->value = NULL;
	return self;
}

void atomic_free(ATOMIC *self)
{
	self->value = NULL;
#ifndef HAS_ATOMIC
	pthread_mutex_destroy(&self->lock);
#endif
	mem_free(self);
}

void atomic_set(ATOMIC *self, void *value)
{
#ifndef HAS_ATOMIC
	pthread_mutex_lock(&self->lock);
	self->value = value;
	pthread_mutex_unlock(&self->lock);
#elif	defined(WINDOWS)
	InterlockedExchangePointer((volatile PVOID*) &self->value, value);
#else
	(void) __sync_lock_test_and_set(&self->value, value);
#endif
}

void *atomic_cas(ATOMIC *self, void *cmp, void *value)
{
#ifndef HAS_ATOMIC
	void *old;

	pthread_mutex_lock(&self->lock);
	old = self->value;
	if (self->value == cmp)
		self->value = value;
	pthread_mutex_unlock(&self->lock);

	return old;
#elif	defined(WINDOWS)
	return InterlockedCompareExchangePointer(
		(volatile PVOID*)&self->value, value, cmp);
#else
	return __sync_val_compare_and_swap(&self->value, cmp, value);
#endif
}

void *atomic_xchg(ATOMIC *self, void *value)
{
#ifndef HAS_ATOMIC
	void *old;

	pthread_mutex_lock(&self->lock);
	old = self->value;
	self->value = value;
	pthread_mutex_unlock(&self->lock);

	return old;
#elif	defined(WINDOWS)
	return InterlockedExchangePointer((volatile PVOID*)&self->value, value);
#else
	return __sync_lock_test_and_set(&self->value, value);
#endif
}

void atomic_int64_set(ATOMIC *self, long long n)
{
#ifndef HAS_ATOMIC
	pthread_mutex_lock(&self->lock);
	*((long long *) self->value) = n;
	pthread_mutex_unlock(&self->lock);
#elif	defined(WINDOWS)
	InterlockedExchangePointer((volatile PVOID*) self->value, (PVOID) n);
#else
	(void) __sync_lock_test_and_set((long long *) self->value, n);
#endif
}

long long atomic_int64_fetch_add(ATOMIC *self, long long n)
{
#ifndef HAS_ATOMIC
	pthread_mutex_lock(&self->lock);
	long long v = *(long long *) self->value;
	*((long long *) self->value) = v + n;
	pthread_mutex_unlock(&self->lock);
	return v;
#elif	defined(WINDOWS)
	return InterlockedExchangeAdd64((volatile LONGLONG*) self->value, n);
#else
	return (long long) __sync_fetch_and_add((long long *) self->value, n);
#endif
}

long long atomic_int64_add_fetch(ATOMIC *self, long long n)
{
#ifndef HAS_ATOMIC
	pthread_mutex_lock(&self->lock);
	long long v = *(long long *) self->value + n;
	*((long long *) self->value) = v;
	pthread_mutex_unlock(&self->lock);
	return v;
#elif	defined(WINDOWS)
	return n + InterlockedExchangeAdd64((volatile LONGLONG*) self->value, n);
#else
	return (long long) __sync_add_and_fetch((long long *) self->value, n);
#endif
}

long long atomic_int64_cas(ATOMIC *self, long long cmp, long long n)
{
#if !defined(HAS_ATOMIC)
	pthread_mutex_lock(&self->lock);
	long long old = *(long long *) self->value;
	if (old == cmp)
		*((long long *) self->value) = n;
	pthread_mutex_unlock(&self->lock);
	return old;
#elif	defined(WINDOWS)
	return InterlockedCompareExchange64(
		(volatile LONGLONG*)&self->value, n, cmp);
#else
	return (long long) __sync_val_compare_and_swap(
			(long long*) self->value, cmp, n);
#endif
}

