#include "stdafx.h"
#include "common.h"

#include "fiber/libfiber.h"
#include "fiber/fiber_mutex.h"
#include "fiber.h"
#include "sync_waiter.h"

struct ACL_FIBER_MUTEX {
	unsigned flags;
	ATOMIC *atomic;
	long long value;
	ARRAY *waiters;
	pthread_mutex_t lock;
	pthread_mutex_t thread_lock;
};

ACL_FIBER_MUTEX *acl_fiber_mutex_create(unsigned flags)
{
	ACL_FIBER_MUTEX *mutex = (ACL_FIBER_MUTEX*)
		mem_calloc(1, sizeof(ACL_FIBER_MUTEX));

	mutex->flags  = flags;
	mutex->atomic = atomic_new();
	atomic_set(mutex->atomic, &mutex->value);
	atomic_int64_set(mutex->atomic, 0);

	mutex->waiters = array_create(5, ARRAY_F_UNORDER);
	pthread_mutex_init(&mutex->lock, NULL);
	pthread_mutex_init(&mutex->thread_lock, NULL);

	return mutex;
}

void acl_fiber_mutex_free(ACL_FIBER_MUTEX *mutex)
{
	atomic_free(mutex->atomic);
	array_free(mutex->waiters, NULL);
	pthread_mutex_destroy(&mutex->lock);
	pthread_mutex_destroy(&mutex->thread_lock);
	mem_free(mutex);
}

static __thread int __cnt = 0;

int acl_fiber_mutex_lock(ACL_FIBER_MUTEX *mutex)
{
	int wakeup = 0;
	EVENT *ev;
	ACL_FIBER *fiber;

	while (1) {
		if (atomic_int64_cas(mutex->atomic, 0, 1) == 0) {
			//pthread_mutex_lock(&mutex->thread_lock);
			__cnt++;
			//printf(">>>>thread-%lu, cnt=%d\n", pthread_self(), __cnt);
			return 0;
		}

		// For the independent thread, only lock the thread mutex.
		if (!var_hook_sys_api) {
			pthread_mutex_lock(&mutex->thread_lock);
			pthread_mutex_unlock(&mutex->thread_lock);

			if (++wakeup > 5) {
				wakeup = 0;
				acl_fiber_delay(100);
			}
			continue;
		}

		fiber = acl_fiber_running();
		fiber->waiter = sync_waiter_get();
		//sync_waiter_append(fiber->waiter, fiber);

		pthread_mutex_lock(&mutex->lock);
		array_append(mutex->waiters, fiber);

		if (atomic_int64_cas(mutex->atomic, 0, 1) == 0) {
			int r = array_delete_obj(mutex->waiters, fiber, NULL);
			pthread_mutex_unlock(&mutex->lock);

			//pthread_mutex_lock(&mutex->thread_lock);
			printf("thread-%lu, ok fb=%p, wakeup=%d, cnt=%d, r=%d\n",
				pthread_self(), fiber, wakeup, __cnt, r);
			assert(r == 0);
			return 0;
		}
		pthread_mutex_unlock(&mutex->lock);

		ev = fiber_io_event();
		ev->waiter++;
		acl_fiber_switch();
		ev->waiter--;

#if 1
		pthread_mutex_lock(&mutex->lock);
		{
			ITER iter;
			foreach(iter, mutex->waiters) {
				ACL_FIBER *fb = (ACL_FIBER*) iter.data;
				if (fb == fiber) {
					printf(">>>thread-%lu, dup, why? fb=%p\n",
						pthread_self(), fb);
					assert(fb != fiber);
				}
			}
		}
		pthread_mutex_unlock(&mutex->lock);
#endif

		if (++wakeup > 5) {
			wakeup = 0;
			acl_fiber_delay(100);
		}
	}
}

int acl_fiber_mutex_unlock(ACL_FIBER_MUTEX *mutex)
{
	ACL_FIBER *fiber;

	pthread_mutex_lock(&mutex->lock);
	fiber = (ACL_FIBER*) array_pop_front(mutex->waiters);

	//pthread_mutex_unlock(&mutex->thread_lock);
	if (atomic_int64_cas(mutex->atomic, 1, 0) != 1) {
		assert(0);
		return -1;
	}
	pthread_mutex_unlock(&mutex->lock);

	if (fiber) {
		if (fiber->status == FIBER_STATUS_SUSPEND)
			printf(">>>thread-%lu, pop suspend fb=%p\n",
				pthread_self(), fiber);
		else
			printf(">>>thread-%lu, pop not %d fb=%p\n",
				pthread_self(), fiber->status, fiber);
		sync_waiter_wakeup(fiber->waiter, fiber);
	}

#if 0
	if (var_hook_sys_api) {
		acl_fiber_yield();
	}
#endif
	return 0;
}
