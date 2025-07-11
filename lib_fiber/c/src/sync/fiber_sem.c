#include "stdafx.h"
#include "common.h"

#include "fiber/libfiber.h"
#include "fiber.h"

struct ACL_FIBER_SEM {
	int num;
	int buf;
	unsigned flags;
	RING waiting;
	unsigned long tid;
};

ACL_FIBER_SEM *acl_fiber_sem_create(int num)
{
	return acl_fiber_sem_create2(num, 0);
}

ACL_FIBER_SEM *acl_fiber_sem_create2(int num, unsigned flags)
{
	int buf = (flags & ACL_FIBER_SEM_F_ASYNC) ? 50000000 : 0;
	return acl_fiber_sem_create3(num, buf, flags);
}

ACL_FIBER_SEM *acl_fiber_sem_create3(int num, int buf, unsigned flags)
{
	ACL_FIBER_SEM *sem = (ACL_FIBER_SEM *) mem_malloc(sizeof(ACL_FIBER_SEM));

	sem->tid   = 0;
	sem->num   = num;
	sem->buf   = buf;
	sem->flags = flags;
	ring_init(&sem->waiting);
	return sem;
}

void acl_fiber_sem_free(ACL_FIBER_SEM *sem)
{
	mem_free(sem);
}

unsigned long acl_fiber_sem_get_tid(ACL_FIBER_SEM *sem)
{
	return sem->tid;
}

void acl_fiber_sem_set_tid(ACL_FIBER_SEM *sem, unsigned long tid)
{
	if (sem->tid != tid && ring_size(&sem->waiting) > 0) {
		msg_fatal("%s(%d), %s: curr sem waiting=%d not empty",
			__FILE__, __LINE__, __FUNCTION__,
			(int) ring_size(&sem->waiting));
	}

	sem->tid = tid;
}

int acl_fiber_sem_num(ACL_FIBER_SEM *sem)
{
	return sem->num;
}

int acl_fiber_sem_waiters_num(ACL_FIBER_SEM *sem)
{
	return ring_size(&sem->waiting);
}

int acl_fiber_sem_timed_wait(ACL_FIBER_SEM *sem, int milliseconds)
{
	ACL_FIBER *curr;
	EVENT *ev;

	if (sem->tid == 0) {
		sem->tid = thread_self();
	}

	if (sem->num > 0) {
		sem->num--;
		return sem->num;
	}

	curr = acl_fiber_running();
	if (curr == NULL) {
		return -1;
	}

	if (milliseconds == 0) {
		acl_fiber_set_errno(curr, FIBER_EAGAIN);
		acl_fiber_set_error(FIBER_EAGAIN);
		return -1;
	}

	// Sanity check before suspending.
	if (acl_fiber_canceled(curr)) {
		acl_fiber_set_error(curr->errnum);
		return -1;
	}

	// Make sure to start wakeup_timers in fiber_io.c by the following:
	// fiber_io_event -> fiber_io_check -> fiber_io_loop -> wakeup_timers.
	ev = fiber_io_event();

	while (sem->num <= 0) {
		ring_prepend(&sem->waiting, &curr->me2);
		curr->wstatus |= FIBER_WAIT_SEM;

		if (milliseconds > 0) {
			fiber_timer_add(curr, (size_t) milliseconds);
		}

		WAITER_INC(ev);  // Just for avoiding fiber_io_loop to exit
		acl_fiber_switch();
		WAITER_DEC(ev);

		if (milliseconds > 0) {
			fiber_timer_del(curr);
		}

		curr->wstatus &= ~FIBER_WAIT_SEM;

		/* If switch to me because other killed me, I should detach myself;
		 * else if because other unlocks, I'll be detached twice, which is
		 * harmless because RING can deal with it.
		 */
		ring_detach(&curr->me);
		ring_detach(&curr->me2);

		if (acl_fiber_canceled(curr)) {
			acl_fiber_set_error(curr->errnum);
			return -1;
		}
		if (curr->flag & FIBER_F_TIMER) {
			// Clear FIBER_F_TIMER flag been set in wakeup_timers.
			curr->flag &= ~FIBER_F_TIMER;

			acl_fiber_set_errno(curr, FIBER_EAGAIN);
			acl_fiber_set_error(FIBER_EAGAIN);
			return -1;
		}
	}

	return --sem->num;
}

int acl_fiber_sem_wait(ACL_FIBER_SEM *sem)
{
	return acl_fiber_sem_timed_wait(sem, -1);
}

int acl_fiber_sem_trywait(ACL_FIBER_SEM *sem)
{
	if (sem->tid == 0) {
		sem->tid = thread_self();
	}

	if (sem->num > 0) {
		sem->num--;
		return sem->num;
	}

	return -1;
}

#define RING_TO_FIBER(r) \
    ((ACL_FIBER *) ((char *) (r) - offsetof(ACL_FIBER, me2)))

#define FIRST_FIBER(head) \
    (ring_succ(head) != (head) ? RING_TO_FIBER(ring_succ(head)) : 0)

int acl_fiber_sem_post(ACL_FIBER_SEM *sem)
{
	ACL_FIBER *ready;
	int num;

	if (sem->tid == 0) {
		sem->tid = thread_self();
	}
#if 1
	else if (sem->tid != (unsigned long) thread_self()) {
		msg_error("%s(%d): current tid=%lu, sem tid=%lu",
			__FUNCTION__, __LINE__, thread_self(), sem->tid);
		return -1;
	}
#endif

	sem->num++;

	if ((ready = FIRST_FIBER(&sem->waiting)) == NULL) {
#if 0
		// Don't yield here. --- 2025.1.8
		if (sem->num >= sem->buf) {
			acl_fiber_yield();
		}
#endif
		return sem->num;
	}

	/* Must clear the FIBER_F_TIMER flag for the waiting fiber to avoid
	 * the flag will be used incorrectly in acl_fiber_sem_timed_wait().
	 */
	ready->flag &= ~FIBER_F_TIMER;

	ring_detach(&ready->me2);
	FIBER_READY(ready);

	num = sem->num;
	if (num >= sem->buf) {
		acl_fiber_yield();
	}

	return num;
}
