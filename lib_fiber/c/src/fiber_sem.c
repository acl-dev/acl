#include "stdafx.h"
#include "fiber/lib_fiber.h"
#include "fiber.h"

struct ACL_FIBER_SEM {
	int num;
	ACL_RING waiting;
	acl_pthread_t tid;
};

ACL_FIBER_SEM *acl_fiber_sem_create(int num)
{
	ACL_FIBER_SEM *sem = (ACL_FIBER_SEM *)
		acl_mymalloc(sizeof(ACL_FIBER_SEM));

	sem->num = num;
	acl_ring_init(&sem->waiting);
	sem->tid = acl_pthread_self();
	return sem;
}

void acl_fiber_sem_free(ACL_FIBER_SEM *sem)
{
	acl_myfree(sem);
}

acl_pthread_t acl_fiber_sem_get_tid(ACL_FIBER_SEM *sem)
{
	return sem->tid;
}

void acl_fiber_sem_set_tid(ACL_FIBER_SEM *sem, acl_pthread_t tid)
{
	if (sem->tid != tid && acl_ring_size(&sem->waiting) > 0) {
		acl_msg_fatal("%s(%d), %s: curr sem waiting=%d not empty",
			__FILE__, __LINE__, __FUNCTION__,
			(int) acl_ring_size(&sem->waiting));
	}

	sem->tid = tid;
}

int acl_fiber_sem_num(ACL_FIBER_SEM *sem)
{
	return sem->num;
}

int acl_fiber_sem_wait(ACL_FIBER_SEM *sem)
{
	ACL_FIBER *curr;

	if (sem->tid != acl_pthread_self())
		return -1;

	if (sem->num > 0) {
		sem->num--;
		return sem->num;
	}

	curr = acl_fiber_running();
	if (curr == NULL)
		return -1;

	acl_ring_prepend(&sem->waiting, &curr->me);
	acl_fiber_switch();

	/* if switch to me because other killed me, I should detach myself;
	 * else if because other unlock, I'll be detached twice which is
	 * hamless because ACL_RING can deal with it.
	 */
	acl_ring_detach(&curr->me);

	return sem->num;
}

int acl_fiber_sem_trywait(ACL_FIBER_SEM *sem)
{
	if (sem->tid != acl_pthread_self())
		return -1;

	if (sem->num > 0) {
		sem->num--;
		return sem->num;
	}

	return -1;
}

#define RING_TO_FIBER(r) \
    ((ACL_FIBER *) ((char *) (r) - offsetof(ACL_FIBER, me)))

#define FIRST_FIBER(head) \
    (acl_ring_succ(head) != (head) ? RING_TO_FIBER(acl_ring_succ(head)) : 0)

int acl_fiber_sem_post(ACL_FIBER_SEM *sem)
{
	ACL_FIBER *ready;

	if (sem->tid != acl_pthread_self())
		return -1;

	if ((ready = FIRST_FIBER(&sem->waiting)) == NULL) {
		sem->num++;
		return sem->num;
	}

	acl_ring_detach(&ready->me);
	acl_fiber_ready(ready);

	return sem->num;
}
