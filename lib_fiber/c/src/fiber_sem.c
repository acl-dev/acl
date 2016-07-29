#include "stdafx.h"
#include "fiber/lib_fiber.h"
#include "fiber.h"

ACL_FIBER_SEM *acl_fiber_sem_create(int num)
{
	ACL_FIBER_SEM *sem = (ACL_FIBER_SEM *)
		acl_mymalloc(sizeof(ACL_FIBER_SEM));

	sem->num = num;
	acl_ring_init(&sem->waiting);
	return sem;
}

void acl_fiber_sem_free(ACL_FIBER_SEM *sem)
{
	acl_myfree(sem);
}

int acl_fiber_sem_wait(ACL_FIBER_SEM *sem)
{
	ACL_FIBER *curr;

	if (sem->num > 0) {
		sem->num--;
		return sem->num;
	}

	curr = fiber_running();
	acl_ring_prepend(&sem->waiting, &curr->me);
	acl_fiber_switch();

	return sem->num;
}

int acl_fiber_sem_trywait(ACL_FIBER_SEM *sem)
{
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
	ACL_FIBER *ready = FIRST_FIBER(&sem->waiting);

	if (ready == NULL) {
		sem->num++;
		return sem->num;
	}

	acl_ring_detach(&ready->me);
	acl_fiber_ready(ready);

	return sem->num;
}
