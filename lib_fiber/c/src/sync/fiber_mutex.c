#include "stdafx.h"
#include "common.h"

#include "fiber/libfiber.h"
#include "fiber/fiber_mutex.h"
#include "fiber.h"

#include "sync_type.h"
#include "sync_waiter.h"

ACL_FIBER_MUTEX *acl_fiber_mutex_create(unsigned flags)
{
	ACL_FIBER_MUTEX *mutex = (ACL_FIBER_MUTEX*)
		mem_calloc(1, sizeof(ACL_FIBER_MUTEX));

	mutex->flags  = flags;

	mutex->waiters = array_create(5, ARRAY_F_UNORDER);
	pthread_mutex_init(&mutex->lock, NULL);
	pthread_mutex_init(&mutex->thread_lock, NULL);

	return mutex;
}

void acl_fiber_mutex_free(ACL_FIBER_MUTEX *mutex)
{
	array_free(mutex->waiters, NULL);
	pthread_mutex_destroy(&mutex->lock);
	pthread_mutex_destroy(&mutex->thread_lock);
	mem_free(mutex);
}

static int fiber_mutex_lock_once(ACL_FIBER_MUTEX *mutex)
{
	int wakeup = 0, pos;
	EVENT *ev;
	ACL_FIBER *fiber;

	while (1) {
		pthread_mutex_lock(&mutex->lock);
		if (pthread_mutex_trylock(&mutex->thread_lock) == 0) {
			pthread_mutex_unlock(&mutex->lock);
			return 0;
		}

		// For the independent thread, only lock the thread mutex.
		if (!var_hook_sys_api) {
			pthread_mutex_unlock(&mutex->lock);
			return pthread_mutex_lock(&mutex->thread_lock);
		}

		fiber = acl_fiber_running();
		fiber->sync = sync_waiter_get();

		pos = array_append(mutex->waiters, fiber);

		if (pthread_mutex_trylock(&mutex->thread_lock) == 0) {
			array_delete(mutex->waiters, pos, NULL);
			pthread_mutex_unlock(&mutex->lock);
			return 0;
		}

		pthread_mutex_unlock(&mutex->lock);

		ev = fiber_io_event();
		WAITER_INC(ev);
		acl_fiber_switch();
		WAITER_DEC(ev);

		if (++wakeup > 5) {
			wakeup = 0;
			acl_fiber_delay(100);
		}
	}
}

static int fiber_mutex_lock_try(ACL_FIBER_MUTEX *mutex)
{
	int wakeup = 0, pos;
	EVENT *ev;
	ACL_FIBER *fiber;

	while (1) {
		if (pthread_mutex_trylock(&mutex->thread_lock) == 0) {
			return 0;
		}

		// For the independent thread, only lock the thread mutex.
		if (!var_hook_sys_api) {
			return pthread_mutex_lock(&mutex->thread_lock);
		}

		fiber = acl_fiber_running();
		fiber->sync = sync_waiter_get();

		pthread_mutex_lock(&mutex->lock);
		pos = array_append(mutex->waiters, fiber);

		if (pthread_mutex_trylock(&mutex->thread_lock) == 0) {
			array_delete(mutex->waiters, pos, NULL);
			pthread_mutex_unlock(&mutex->lock);

			return 0;
		}

		pthread_mutex_unlock(&mutex->lock);

		ev = fiber_io_event();
		WAITER_INC(ev);
		acl_fiber_switch();
		WAITER_DEC(ev);

		if (++wakeup > 5) {
			wakeup = 0;
			acl_fiber_delay(100);
		}
	}
}

int acl_fiber_mutex_lock(ACL_FIBER_MUTEX *mutex)
{
	if (mutex->flags & FIBER_MUTEX_F_LOCK_TRY) {
		return fiber_mutex_lock_try(mutex);
	} else {
		return fiber_mutex_lock_once(mutex);
	}
}

int acl_fiber_mutex_trylock(ACL_FIBER_MUTEX *mutex)
{
	return pthread_mutex_trylock(&mutex->thread_lock);
}

int acl_fiber_mutex_unlock(ACL_FIBER_MUTEX *mutex)
{
	ACL_FIBER *fiber;
	int ret;

	pthread_mutex_lock(&mutex->lock);
	fiber = (ACL_FIBER*) array_pop_front(mutex->waiters);
	pthread_mutex_unlock(&mutex->lock);

	ret = pthread_mutex_unlock(&mutex->thread_lock);
	if (ret != 0) {
		return ret;
	}

	if (fiber) {
		if (pthread_self() == fiber->tid) {
			acl_fiber_ready(fiber);
		} else {
			sync_waiter_wakeup(fiber->sync, fiber);
		}
	}

	return 0;
}
