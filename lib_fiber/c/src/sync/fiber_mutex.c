#include "stdafx.h"
#include "common.h"

#include "fiber/libfiber.h"
#include "fiber/fiber_mutex.h"
#include "fiber.h"

#include "sync_type.h"
#include "sync_waiter.h"

typedef struct {
	unsigned long tid;
} THREAD_WAITER;

typedef struct {
	RING head;
} THREAD_MUTEXES;

static pthread_once_t  __once_control = PTHREAD_ONCE_INIT;
static THREAD_MUTEXES *__locks = NULL;
static pthread_mutex_t __lock;

static void show_status(struct ACL_FIBER_MUTEX *mutex)
{
	ITER iter;

	pthread_mutex_lock(&mutex->lock);
	if (mutex->owner == 0) {
		pthread_mutex_unlock(&mutex->lock);
		return;
	}

	printf(">>The mutex's waiters: mutex=%p\r\n", mutex);
	if (mutex->owner > 0) {
		printf("    owner  => fiber-%ld\r\n", mutex->owner);
	} else if (mutex->owner < 0) {
		printf("    owner  => thread-%lu\r\n", -mutex->owner);
	}
	foreach(iter, mutex->waiters) {
		ACL_FIBER *fb = (ACL_FIBER*) iter.data;
		printf("    waiter => fiber-%u\r\n", acl_fiber_id(fb));
	}
	foreach(iter, mutex->waiting_threads) {
		THREAD_WAITER *waiter = (THREAD_WAITER*) iter.data;
		printf("    waiter => thread-%lu\r\n", waiter->tid);
	}
	pthread_mutex_unlock(&mutex->lock);
}

void acl_fiber_mutex_profile(void)
{
	RING_ITER iter;

	if (__locks == NULL) {
		printf(">>>lock null\n");
		return;
	}

	//printf(">>>begin to check\n");
	pthread_mutex_lock(&__lock);
	ring_foreach(iter, &__locks->head) {
		ACL_FIBER_MUTEX *lock =
			RING_TO_APPL(iter.ptr, ACL_FIBER_MUTEX, me);
		show_status(lock);
	}
	pthread_mutex_unlock(&__lock);
	//printf("\r\n");
}

int acl_fiber_mutex_deadcheck(void)
{
	RING_ITER iter;

	if (__locks == NULL) {
		return 0;
	}

	pthread_mutex_lock(&__lock);
	ring_foreach(iter, &__locks->head) {
	}
	pthread_mutex_unlock(&__lock);
	return 0;
}

static void thread_waiter_add(ACL_FIBER_MUTEX *mutex, unsigned long tid)
{
	THREAD_WAITER *waiter = (THREAD_WAITER*) mem_malloc(sizeof(THREAD_WAITER));
	waiter->tid = tid;
	pthread_mutex_lock(&mutex->lock);
	array_append(mutex->waiting_threads, waiter);
	pthread_mutex_unlock(&mutex->lock);
}

static void thread_waiter_remove(ACL_FIBER_MUTEX *mutex, unsigned long tid)
{
	ITER iter;

	pthread_mutex_lock(&mutex->lock);
	foreach(iter, mutex->waiting_threads) {
		THREAD_WAITER *waiter = (THREAD_WAITER*) iter.data;
		if (waiter->tid == tid) {
			array_delete(mutex->waiting_threads, iter.i, NULL);
			mem_free(waiter);
			break;
		}
	}
	pthread_mutex_unlock(&mutex->lock);
}

static void thread_once(void)
{
	pthread_mutex_init(&__lock, NULL);
	__locks = (THREAD_MUTEXES*) mem_malloc(sizeof(THREAD_MUTEXES));
	ring_init(&__locks->head);
}

ACL_FIBER_MUTEX *acl_fiber_mutex_create(unsigned flags)
{
	ACL_FIBER_MUTEX *mutex;

	if (pthread_once(&__once_control, thread_once) != 0) {
		printf("%s(%d), %s: pthread_once error %s\r\n",
			__FILE__, __LINE__, __FUNCTION__, last_serror());
		abort();
	}

	mutex = (ACL_FIBER_MUTEX*) mem_calloc(1, sizeof(ACL_FIBER_MUTEX));
	ring_init(&mutex->me);

	mutex->flags  = flags;

	mutex->waiters = array_create(5, ARRAY_F_UNORDER);
	mutex->waiting_threads = array_create(5, ARRAY_F_UNORDER);
	pthread_mutex_init(&mutex->lock, NULL);
	pthread_mutex_init(&mutex->thread_lock, NULL);

	pthread_mutex_lock(&__lock);
	ring_prepend(&__locks->head, &mutex->me);
	pthread_mutex_unlock(&__lock);
	return mutex;
}

void acl_fiber_mutex_free(ACL_FIBER_MUTEX *mutex)
{
	pthread_mutex_lock(&__lock);
	ring_detach(&mutex->me);
	pthread_mutex_unlock(&__lock);

	array_free(mutex->waiters, NULL);
	array_free(mutex->waiting_threads, mem_free);
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
			thread_waiter_add(mutex, (unsigned long) pthread_self());
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
	ACL_FIBER *fiber = acl_fiber_running();

	while (1) {
		if (pthread_mutex_trylock(&mutex->thread_lock) == 0) {
			return 0;
		}

		// For the independent thread, only lock the thread mutex.
		if (!var_hook_sys_api) {
			thread_waiter_add(mutex, (unsigned long) pthread_self());
			return pthread_mutex_lock(&mutex->thread_lock);
		}

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
	int ret;

	if (mutex->flags & FIBER_MUTEX_F_LOCK_TRY) {
		ret = fiber_mutex_lock_try(mutex);
	} else {
		ret = fiber_mutex_lock_once(mutex);
	}
	if (ret == 0) {
		unsigned id = acl_fiber_self();  // 0 will return in no fiber mode.
		long me = id == 0 ? -pthread_self() : (long) id;
		mutex->owner = me;
		if (me < 0) {
			thread_waiter_remove(mutex, pthread_self());
		}

		mutex->fiber = acl_fiber_running();
	}
	return ret;
}

int acl_fiber_mutex_trylock(ACL_FIBER_MUTEX *mutex)
{
	int ret = pthread_mutex_trylock(&mutex->thread_lock);
	if (ret == 0) {
		unsigned id = acl_fiber_self();
		long me = id == 0 ? -pthread_self() : (long) id;
		mutex->owner = me;
	}
	return ret;
}

int acl_fiber_mutex_unlock(ACL_FIBER_MUTEX *mutex)
{
	ACL_FIBER *fiber;
	int ret;

	pthread_mutex_lock(&mutex->lock);
	fiber = (ACL_FIBER*) array_pop_front(mutex->waiters);

	// Just a sanity check!
	if (acl_fiber_running() != mutex->fiber) {
		msg_error("%s(%d): not the onwer fiber=%p, fiber=%p",
			__FUNCTION__, __LINE__, acl_fiber_running(),
			mutex->fiber);
	}

	mutex->owner = 0;
	mutex->fiber = NULL;

	ret = pthread_mutex_unlock(&mutex->thread_lock);

	// Unlock the internal prive lock must behind the public thread_lock,
	// or else the waiter maybe be skipped added in lock waiting process.
	pthread_mutex_unlock(&mutex->lock);

	if (ret != 0) {
		return ret;
	}

	if (fiber) {
		if ((unsigned long) pthread_self() == fiber->tid) {
			acl_fiber_ready(fiber);
		} else {
			sync_waiter_wakeup(fiber->sync, fiber);
		}
	}

	return 0;
}
