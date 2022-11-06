#include "stdafx.h"
#include "common.h"

#include "fiber/libfiber.h"
#include "fiber_mutex.h"

struct ACL_FIBER_MUTEX {
	unsigned flag;
	ATOMIC *atomic;
	long long value;
	ARRAY *waiters;
	pthread_mutex_t lock;
	pthread_mutex_t thread_lock;
};

ACL_FIBER_MUTEX *acl_fiber_mutex_create(unsigned flag)
{
	ACL_FIBER_MUTEX *mutex = mem_calloc(1, sizeof(ACL_FIBER_MUTEX));

	mutex->flags  = flag;
	mutex->atomic = atomic_new();
	atomic_set(mutex->atomic, &event->value);
	atomic_int64_set(mutex->atomic, 0);

	ring_init(&mutex->waiters);
	pthread_mutex_init(&mutex->lock, NULL);
	pthread_mutex_init(&mutex->thread_lock, NULL);

	return mutex;
}

void acl_fiber_mutex_free(ACL_FIBER_MUTEX *mutex)
{
	atomic_free(mutex->atomic);
	pthread_mutex_destroy(&mutex->tlock);
	mem_free(mutex);
}

static int thread_mutex_lock(ACL_FIBER_MUTEX *mutex)
{

}

int acl_fiber_mutex_lock(ACL_FIBER_MUTEX *mutex)
{
	int wakeup;
	EVENT *ev;
	SYNC_WAITER *waiter;

	if (atomic_int64_cas(event->atomic, 0, 1) == 0) {
		pthread_mutex_lock(&mutex->thread_lock);
		return 0;
	}

	wakeup = 0;

	if (var_hook_sys_api) {
		waiter = sync_waiter_get();
		pthread_mutex_lock(&mutex->lock);
		array_append(mutex->waiters, waiter);
	} else {
		waiter = NULL;
	}

	while (1) {
		if (atomic_int64_cas(event->atomic, 0, 1) == 0) {
			if (var_hook_sys_api && !wakeup) {
				array_delete_obj(mutex->waiters, waiter, NULL);
				pthread_mutex_unlock(&mutex->lock);
			}

			pthread_mutex_lock(&mutex->thread_lock);
			return 0;
		}

		if (!var_hook_sys_api) {
			// For the alone thread, just lock the thread mutex.
			pthread_mutex_lock(&mutex->thread_lock);
			pthread_mutex_unlock(&mutex->thread_lock);

			if (++wakeup > 5) {
				sleep(1);
			}
			continue;
		}

		if (wakeup) {
			ACL_FIBER *me = acl_fiber_running();

			array_append(mutex->waiters, waiter);
		}

		pthread_mutex_unlock(&mutex->lock);

		ev = fiber_io_event();
		ev->waiter++;
		acl_fiber_switch();
		ev->waiter--;

		if (++wakeup == 0) {  // Overflow ?
			wakeup = 1;
		}

		pthread_mutex_lock(&mutex->lock);
	}
}

int acl_fiber_mutex_unlock(ACL_FIBER_MUTEX *mutex)
{
	SYNC_WAITER *waiter;

	pthread_mutex_lock(&mutex->lock);
	waiter = (SYNC_WAITER*) mutex->waiters->pop_back(mutex->waiters);
	pthread_mutex_unlock(&mutex->lock);

	if (atomic_int64_cas(event->atomic, 1, 0) != 1) {
		pthread_mutex_unlock(&mutex->lock);
		return -1;
	}

	pthread_mutex_unlock(&mutex->thread_lock);

	if (waiter && sync_waiter_wakeup(waiter) == -1) {
		return -1;
	}

	return 0;
}
