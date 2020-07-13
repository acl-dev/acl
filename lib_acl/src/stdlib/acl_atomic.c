#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_mymalloc.h"
#include "thread/acl_pthread.h"
#include "stdlib/acl_atomic.h"

#endif

/* In lower NDK, atomic not support for lower Android, see:
 * https://android.googlesource.com/toolchain/gcc/+/refs/heads/master/gcc-4.9/libgcc/config/arm/linux-atomic-64bit.c#53
 * So, we must exclude atomic for lower Android version.
 */
#if !defined(ACL_ANDROID) && defined(ACL_LINUX)
# if defined(__GNUC__) && (__GNUC__ >= 4)
#  define HAS_ATOMIC
# endif
#endif

#if defined(ACL_FREEBSD)
# define HAS_ATOMIC
#endif

/*
#ifdef arm64
#error "arm64"
#endif

#ifdef arm64e
#error "arm64e"
#endif

#ifdef armv7
#error "armv7"
#endif

#ifdef armv7s
#error "arm7s"
#endif

#ifdef i386
#error "i386"
#endif

#ifdef x86_64
#error "x86_64"
#endif

*/
/* don't use atomic for IOS and MacOS */
#if defined(ACL_MACOSX)
#  undef HAS_ATOMIC
#endif

#if defined(ACL_WINDOWS)
# define HAS_ATOMIC
#endif

/*
#if !defined(HAS_ATOMIC)
# pragma message "Atomic not support, using thread mutex instead!"
#endif
*/

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
#endif
	acl_myfree(self);
}

void acl_atomic_set(ACL_ATOMIC *self, void *value)
{
#ifndef HAS_ATOMIC
	acl_pthread_mutex_lock(&self->lock);
	self->value = value;
	acl_pthread_mutex_unlock(&self->lock);
#elif	defined(ACL_WINDOWS)
	InterlockedExchangePointer((volatile PVOID*) &self->value, value);
#else
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
#else
	return __sync_val_compare_and_swap(&self->value, cmp, value);
#endif
}

void *acl_atomic_xchg(ACL_ATOMIC *self, void *value)
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
#else
	return __sync_lock_test_and_set(&self->value, value);
#endif
}

void acl_atomic_int64_set(ACL_ATOMIC *self, long long n)
{
#ifndef HAS_ATOMIC
	acl_pthread_mutex_lock(&self->lock);
	*((long long *) self->value) = n;
	acl_pthread_mutex_unlock(&self->lock);
#elif	defined(ACL_WINDOWS)
	InterlockedExchangePointer((volatile PVOID*) self->value, (PVOID) n);
#else
	(void) __sync_lock_test_and_set((long long *) self->value, n);
#endif
}

long long acl_atomic_int64_fetch_add(ACL_ATOMIC *self, long long n)
{
#ifndef HAS_ATOMIC
	acl_pthread_mutex_lock(&self->lock);
	long long v = *(long long *) self->value;
	*((long long *) self->value) = v + n;
	acl_pthread_mutex_unlock(&self->lock);
	return v;
#elif	defined(ACL_WINDOWS)
	return InterlockedExchangeAdd64((volatile LONGLONG*) self->value, n);
#else
	return (long long) __sync_fetch_and_add((long long *) self->value, n);
#endif
}

long long acl_atomic_int64_add_fetch(ACL_ATOMIC *self, long long n)
{
#ifndef HAS_ATOMIC
	acl_pthread_mutex_lock(&self->lock);
	long long v = *(long long *) self->value + n;
	*((long long *) self->value) = v;
	acl_pthread_mutex_unlock(&self->lock);
	return v;
#elif	defined(ACL_WINDOWS)
	return n + InterlockedExchangeAdd64((volatile LONGLONG*) self->value, n);
#else
	return (long long) __sync_add_and_fetch((long long *) self->value, n);
#endif
}

long long acl_atomic_int64_cas(ACL_ATOMIC *self, long long cmp, long long n)
{
#if !defined(HAS_ATOMIC)
	acl_pthread_mutex_lock(&self->lock);
	long long old = *(long long *) self->value;
	if (old == cmp)
		*((long long *) self->value) = n;
	acl_pthread_mutex_unlock(&self->lock);
	return old;
#elif	defined(ACL_WINDOWS)
	return InterlockedCompareExchange64(
		(volatile LONGLONG*)&self->value, n, cmp);
#else
	return (long long) __sync_val_compare_and_swap(
			(long long*) self->value, cmp, n);
#endif
}

/****************************************************************************/

struct ACL_ATOMIC_CLOCK {
	long long   atime;
	long long   count;
	long long   users;
	ACL_ATOMIC *atime_atomic;
	ACL_ATOMIC *count_atomic;
	ACL_ATOMIC *users_atomic;
};

ACL_ATOMIC_CLOCK *acl_atomic_clock_alloc(void)
{
	ACL_ATOMIC_CLOCK *clk = (ACL_ATOMIC_CLOCK *)
		acl_mycalloc(1, sizeof(ACL_ATOMIC_CLOCK));
	struct timeval tv;

	gettimeofday(&tv, NULL);

	clk->atime = (long long) tv.tv_sec * 1000000 + (long long) tv.tv_usec;
	clk->count = 0;
	clk->users = 0;
	clk->atime_atomic = acl_atomic_new();
	clk->count_atomic = acl_atomic_new();
	clk->users_atomic = acl_atomic_new();

	acl_atomic_set(clk->atime_atomic, &clk->atime);
	acl_atomic_set(clk->count_atomic, &clk->count);
	acl_atomic_set(clk->users_atomic, &clk->users);

	return clk;
}

void acl_atomic_clock_free(ACL_ATOMIC_CLOCK *clk)
{
	acl_atomic_free(clk->atime_atomic);
	acl_atomic_free(clk->count_atomic);
	acl_atomic_free(clk->users_atomic);
	acl_myfree(clk);
}

long long acl_atomic_clock_count_add(ACL_ATOMIC_CLOCK *clk, int n)
{
	long long now;
	struct timeval tv;

	gettimeofday(&tv, NULL);
	now = (long long) tv.tv_sec * 1000000 + (long long) tv.tv_usec;
	acl_atomic_int64_set(clk->atime_atomic, now);

	return acl_atomic_int64_add_fetch(clk->count_atomic, n);
}

long long acl_atomic_clock_users_add(ACL_ATOMIC_CLOCK *clk, int n)
{
	long long now;
	struct timeval tv;

	gettimeofday(&tv, NULL);
	now = (long long) tv.tv_sec * 1000000 + (long long) tv.tv_usec;
	acl_atomic_int64_set(clk->atime_atomic, now);

	return acl_atomic_int64_add_fetch(clk->users_atomic, n);
}

void acl_atomic_clock_users_count_inc(ACL_ATOMIC_CLOCK *clk)
{
	long long now;
	struct timeval tv;

	gettimeofday(&tv, NULL);
	now = (long long) tv.tv_sec * 1000000 + (long long) tv.tv_usec;
	acl_atomic_int64_set(clk->atime_atomic, now);

	acl_atomic_int64_add_fetch(clk->count_atomic, 1);
	acl_atomic_int64_add_fetch(clk->users_atomic, 1);
}

long long acl_atomic_clock_count(ACL_ATOMIC_CLOCK *clk)
{
	return acl_atomic_int64_add_fetch(clk->count_atomic, 0);
}

long long acl_atomic_clock_atime(ACL_ATOMIC_CLOCK *clk)
{
	return acl_atomic_int64_add_fetch(clk->atime_atomic, 0);
}

long long acl_atomic_clock_users(ACL_ATOMIC_CLOCK *clk)
{
	return acl_atomic_int64_add_fetch(clk->users_atomic, 0);
}
