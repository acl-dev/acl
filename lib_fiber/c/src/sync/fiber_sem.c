#include "stdafx.h"
#include "common.h"

#include "fiber/libfiber.h"
#include "fiber.h"

struct ACL_FIBER_SEM {
	int num;
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
	ACL_FIBER_SEM *sem = (ACL_FIBER_SEM *) mem_malloc(sizeof(ACL_FIBER_SEM));

	sem->tid   = 0;
	sem->num   = num;
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

int acl_fiber_sem_wait(ACL_FIBER_SEM *sem)
{
	ACL_FIBER *curr;
	EVENT *ev;

	if (sem->tid == 0) {
		sem->tid = thread_self();
	}
#if 0
	else if (sem->tid != (unsigned long) thread_self()) {
		msg_error("%s(%d): current tid=%lu, sem tid=%lu",
			__FUNCTION__, __LINE__, thread_self(), sem->tid);
		return -1;
	}
#endif

	if (sem->num > 0) {
		sem->num--;
		return sem->num;
	}

	curr = acl_fiber_running();
	if (curr == NULL) {
		return -1;
	}

	// Sanity check befor suspending.
	if (acl_fiber_canceled(curr)) {
		acl_fiber_set_error(curr->errnum);
		//msg_info("%s(%d): fiber-%d be killed",
		//	__FUNCTION__, __LINE__, acl_fiber_id(curr));
		return -1;
	}

	ring_prepend(&sem->waiting, &curr->me);

	curr->wstatus |= FIBER_WAIT_SEM;

	ev = fiber_io_event();
	WAITER_INC(ev);  // Just for avoiding fiber_io_loop to exit
	acl_fiber_switch();
	WAITER_DEC(ev);

	curr->wstatus &= ~FIBER_WAIT_SEM;

	/* If switch to me because other killed me, I should detach myself;
	 * else if because other unlock, I'll be detached twice which is
	 * hamless because RIGN can deal with it.
	 */
	ring_detach(&curr->me);

	if (acl_fiber_canceled(curr)) {
		acl_fiber_set_error(curr->errnum);
		//msg_info("%s(%d): fiber-%d be killed",
		//	__FUNCTION__, __LINE__, acl_fiber_id(curr));
		return -1;
	}

	/* Needn't decrease the sem number, because the notifier has decreased
	 * it for me.
	 */
	return sem->num;
}

int acl_fiber_sem_trywait(ACL_FIBER_SEM *sem)
{
	if (sem->tid == 0) {
		sem->tid = thread_self();
	}
#if 0
	else if (sem->tid != thread_self()) {
		msg_error("%s(%d): current tid=%lu, sem tid=%lu",
			__FUNCTION__, __LINE__, thread_self(), sem->tid);
		return -1;
	}
#endif

	if (sem->num > 0) {
		sem->num--;
		return sem->num;
	}

	return -1;
}

#define RING_TO_FIBER(r) \
    ((ACL_FIBER *) ((char *) (r) - offsetof(ACL_FIBER, me)))

#define FIRST_FIBER(head) \
    (ring_succ(head) != (head) ? RING_TO_FIBER(ring_succ(head)) : 0)

int acl_fiber_sem_post(ACL_FIBER_SEM *sem)
{
	ACL_FIBER *ready;
	int num;

	if (sem->tid == 0) {
		sem->tid = thread_self();
	}
#if 0
	else if (sem->tid != thread_self()) {
		msg_error("%s(%d): current tid=%lu, sem tid=%lu",
			__FUNCTION__, __LINE__, thread_self(), sem->tid);
		return -1;
	}
#endif

	sem->num++;

	if ((ready = FIRST_FIBER(&sem->waiting)) == NULL) {
		return sem->num;
	}

	ring_detach(&ready->me);
	FIBER_READY(ready);

	/* Help the fiber to be wakeup to decrease the sem number. */
	num = sem->num--;

	if (!(sem->flags & ACL_FIBER_SEM_F_ASYNC)) {
		acl_fiber_yield();
	}

	return num;
}
