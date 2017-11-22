#include "stdafx.h"
#include <pthread.h>
#include <sys/eventfd.h>
#include "fiber/lib_fiber.h"
#include "fiber.h"

ACL_FIBER_MUTEX_R *acl_fiber_mutex_r_create(void)
{
	pthread_mutexattr_t attr;
	ACL_FIBER_MUTEX_R *mutex = (ACL_FIBER_MUTEX_R *)
		acl_mymalloc(sizeof(ACL_FIBER_MUTEX_R));

	mutex->mutex = (acl_pthread_mutex_t*)
		acl_mycalloc(1, sizeof(acl_pthread_mutex_t));

	pthread_mutexattr_init(&attr);
	acl_pthread_mutex_init(mutex->mutex, &attr);
	pthread_mutexattr_destroy(&attr);

	acl_ring_init(&mutex->waiters);
	mutex->atomic = acl_atomic_new();
	acl_atomic_set(mutex->atomic, &mutex->value);
	acl_atomic_int64_set(mutex->atomic, 0);

	return mutex;
}

void acl_fiber_mutex_r_free(ACL_FIBER_MUTEX_R *mutex)
{
	acl_atomic_free(mutex->atomic);
	acl_pthread_mutex_destroy(mutex->mutex);
	acl_myfree(mutex);
}

static inline void __lock_channel_open(ACL_FIBER *fiber)
{
	if (fiber->evfd == -1)
		fiber->evfd = eventfd(0, 0);
}

static inline void __lock_channel_close(ACL_FIBER *fiber)
{
	if (fiber->evfd >= 0) {
		close(fiber->evfd);
		fiber->evfd = -1;
	}
}

static inline void __lock_wait(ACL_FIBER *fiber)
{
	long long n;

	read(fiber->evfd, &n, sizeof(n));
}

int acl_fiber_mutex_r_lock(ACL_FIBER_MUTEX_R *mutex)
{
	ACL_FIBER *curr;

	if (acl_atomic_int64_cas(mutex->atomic, 0, 1) == 0)
		return 0;

	curr = acl_fiber_running();
	__lock_channel_open(curr);

	acl_pthread_mutex_lock(mutex->mutex);
	acl_ring_prepend(&mutex->waiters, &curr->mutex_waiter);
	acl_pthread_mutex_unlock(mutex->mutex);

	while (1) {
		if (acl_atomic_int64_cas(mutex->atomic, 0, 1) == 0) {
			acl_pthread_mutex_lock(mutex->mutex);
			acl_ring_detach(&curr->mutex_waiter);
			acl_pthread_mutex_unlock(mutex->mutex);

			break;
		}

		__lock_wait(curr);
	}

	__lock_channel_close(curr);
	return 0;
}

static inline void __lock_wakeup(ACL_FIBER *fiber)
{
	long long n = 1;
	assert(fiber->evfd >= 0);
	write(fiber->evfd, &n, sizeof(n));
}

#define RING_TO_FIBER(r) \
    ((ACL_FIBER *) ((char *) (r) - offsetof(ACL_FIBER, mutex_waiter)))

#define FIRST_FIBER(head) \
    (acl_ring_succ(head) != (head) ? RING_TO_FIBER(acl_ring_succ(head)) : 0)

int acl_fiber_mutex_r_unlock(ACL_FIBER_MUTEX_R *mutex)
{
	ACL_FIBER *waiter;

	if (acl_atomic_int64_cas(mutex->atomic, 1, 0) != 1)
		abort();

	acl_pthread_mutex_lock(mutex->mutex);
	waiter = FIRST_FIBER(&mutex->waiters);
	if (waiter)
		__lock_wakeup(waiter);
	acl_pthread_mutex_unlock(mutex->mutex);

	return 0;
}
