#include "stdafx.h"
#include "fiber/lib_fiber.h"
#include "fiber.h"

FIBER_MUTEX *fiber_mutex_create(void)
{
	FIBER_MUTEX *l = (FIBER_MUTEX *) acl_mymalloc(sizeof(FIBER_MUTEX));

	l->owner = NULL;
	acl_ring_init(&l->waiting);
	return l;
}

void fiber_mutex_free(FIBER_MUTEX *l)
{
	acl_myfree(l);
}

static int __lock(FIBER_MUTEX *l, int block)
{
	FIBER *curr;

	if (l->owner == NULL){
		l->owner = fiber_running();
		return 1;
	}

	if(!block)
		return 0;

	curr = fiber_running();
	acl_ring_prepend(&l->waiting, &curr->me);

	fiber_switch();

	if (l->owner != curr)
		acl_msg_fatal("%s(%d), %s: qlock: owner=%p self=%p oops",
			__FILE__, __LINE__, __FUNCTION__, l->owner, curr);

	return 1;
}

void fiber_mutex_lock(FIBER_MUTEX *l)
{
	__lock(l, 1);
}

int fiber_mutex_trylock(FIBER_MUTEX *l)
{
	return __lock(l, 0);
}

#define RING_TO_FIBER(r) \
	((FIBER *) ((char *) (r) - offsetof(FIBER, me)))

#define FIRST_FIBER(head) \
	(acl_ring_succ(head) != (head) ? RING_TO_FIBER(acl_ring_succ(head)) : 0)

void fiber_mutex_unlock(FIBER_MUTEX *l)
{
	FIBER *ready;
	
	if (l->owner == 0)
		acl_msg_fatal("%s(%d), %s: qunlock: owner NULL",
			__FILE__, __LINE__, __FUNCTION__);

	ready = FIRST_FIBER(&l->waiting);

	if ((l->owner = ready) != NULL) {
		acl_ring_detach(&ready->me);
		fiber_ready(ready);
	}
}

/*--------------------------------------------------------------------------*/

FIBER_RWLOCK *fiber_rwlock_create(void)
{
	FIBER_RWLOCK *l = (FIBER_RWLOCK *) acl_mymalloc(sizeof(FIBER_RWLOCK));

	l->readers = 0;
	l->writer  = NULL;
	acl_ring_init(&l->rwaiting);
	acl_ring_init(&l->wwaiting);

	return l;
}

void fiber_rwlock_free(FIBER_RWLOCK *l)
{
	acl_myfree(l);
}

static int __rlock(FIBER_RWLOCK *l, int block)
{
	FIBER *curr;

	if (l->writer == NULL && FIRST_FIBER(&l->wwaiting) == NULL) {
		l->readers++;
		return 1;
	}

	if (!block)
		return 0;

	curr = fiber_running();
	acl_ring_prepend(&l->rwaiting, &curr->me);
	fiber_switch();

	return 1;
}

void fiber_rwlock_rlock(FIBER_RWLOCK *l)
{
	(void) __rlock(l, 1);
}

int fiber_rwlock_tryrlock(FIBER_RWLOCK *l)
{
	return __rlock(l, 0);
}

static int __wlock(FIBER_RWLOCK *l, int block)
{
	FIBER *curr;

	if (l->writer == NULL && l->readers == 0) {
		l->writer = fiber_running();
		return 1;
	}

	if (!block)
		return 0;

	curr = fiber_running();
	acl_ring_prepend(&l->wwaiting, &curr->me);
	fiber_switch();

	return 1;
}

void fiber_rwlock_wlock(FIBER_RWLOCK *l)
{
	__wlock(l, 1);
}

int fiber_rwlock_trywlock(FIBER_RWLOCK *l)
{
	return __wlock(l, 0);
}

void fiber_rwlock_runlock(FIBER_RWLOCK *l)
{
	FIBER *fiber;

	if (--l->readers == 0 && (fiber = FIRST_FIBER(&l->wwaiting)) != NULL) {
		acl_ring_detach(&l->wwaiting);
		l->writer = fiber;
		fiber_ready(fiber);
	}
}

void fiber_rwlock_wunlock(FIBER_RWLOCK *l)
{
	FIBER *fiber;
	
	if (l->writer == NULL)
		acl_msg_fatal("%s(%d), %s: wunlock: not locked",
			__FILE__, __LINE__, __FUNCTION__);

	l->writer = NULL;

	if (l->readers != 0)
		acl_msg_fatal("%s(%d), %s: wunlock: readers",
			__FILE__, __LINE__, __FUNCTION__);

	while ((fiber = FIRST_FIBER(&l->rwaiting)) != NULL) {
		acl_ring_detach(&l->rwaiting);
		l->readers++;
		fiber_ready(fiber);
	}

	if (l->readers == 0 && (fiber = FIRST_FIBER(&l->wwaiting)) != NULL) {
		acl_ring_detach(&l->wwaiting);
		l->writer = fiber;
		fiber_ready(fiber);
	}
}
