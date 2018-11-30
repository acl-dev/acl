#include "stdafx.h"
#include "fiber/lib_fiber.h"
#include "fiber.h"

struct ACL_FIBER_MUTEX {
	ACL_RING   me;
	ACL_FIBER *owner;
	ACL_RING   waiting;
};

struct ACL_FIBER_RWLOCK {
	int        readers;
	ACL_FIBER *writer;
	ACL_RING   rwaiting;
	ACL_RING   wwaiting;
};

ACL_FIBER_MUTEX *acl_fiber_mutex_create(void)
{
	ACL_FIBER_MUTEX *lk = (ACL_FIBER_MUTEX *)
		acl_mymalloc(sizeof(ACL_FIBER_MUTEX));

	lk->owner = NULL;
	acl_ring_init(&lk->me);
	acl_ring_init(&lk->waiting);
	return lk;
}

void acl_fiber_mutex_free(ACL_FIBER_MUTEX *lk)
{
	acl_myfree(lk);
}

static int __lock(ACL_FIBER_MUTEX *lk, int block)
{
	ACL_FIBER *curr = acl_fiber_running();

	if (lk->owner == NULL) {
		lk->owner = acl_fiber_running();
		acl_ring_prepend(&curr->holding, &lk->me);
		return 0;
	}

	// xxx: no support recursion lock
	assert(lk->owner != curr);

	if (!block)
		return -1;

	acl_ring_prepend(&lk->waiting, &curr->me);
	curr->waiting = lk;

	acl_fiber_switch();

	/* if switch to me because other killed me, I should detach myself;
	 * else if because other unlock, I'll be detached twice which is
	 * hamless because ACL_RING can deal with it.
	 */
	acl_ring_detach(&curr->me);

	if (lk->owner == curr)
		return 0;

	if (acl_fiber_killed(curr))
		acl_msg_info("%s(%d), %s: lock fiber-%u was killed",
			__FILE__, __LINE__, __FUNCTION__, acl_fiber_id(curr));
	else
		acl_msg_warn("%s(%d), %s: qlock: owner=%p self=%p oops",
			__FILE__, __LINE__, __FUNCTION__, lk->owner, curr);

	return 0;
}

void acl_fiber_mutex_lock(ACL_FIBER_MUTEX *lk)
{
	__lock(lk, 1);
}

int acl_fiber_mutex_trylock(ACL_FIBER_MUTEX *lk)
{
	return __lock(lk, 0) ? 0 : -1;
}

#define RING_TO_FIBER(r) \
    ((ACL_FIBER *) ((char *) (r) - offsetof(ACL_FIBER, me)))

#define FIRST_FIBER(head) \
    (acl_ring_succ(head) != (head) ? RING_TO_FIBER(acl_ring_succ(head)) : 0)

void acl_fiber_mutex_unlock(ACL_FIBER_MUTEX *lk)
{
	ACL_FIBER *ready, *curr = acl_fiber_running();
	
	if (lk->owner == NULL)
		acl_msg_fatal("%s(%d), %s: qunlock: owner NULL",
			__FILE__, __LINE__, __FUNCTION__);
	if (lk->owner != curr)
		acl_msg_fatal("%s(%d), %s: invalid owner=%p, %p",
			__FILE__, __LINE__, __FUNCTION__, lk->owner, curr);

	acl_ring_detach(&lk->me);
	ready = FIRST_FIBER(&lk->waiting);

	if ((lk->owner = ready) != NULL) {
		acl_ring_detach(&ready->me);
		acl_fiber_ready(ready);
	}
}

/*--------------------------------------------------------------------------*/

ACL_FIBER_RWLOCK *acl_fiber_rwlock_create(void)
{
	ACL_FIBER_RWLOCK *lk = (ACL_FIBER_RWLOCK *)
		acl_mymalloc(sizeof(ACL_FIBER_RWLOCK));

	lk->readers = 0;
	lk->writer  = NULL;
	acl_ring_init(&lk->rwaiting);
	acl_ring_init(&lk->wwaiting);

	return lk;
}

void acl_fiber_rwlock_free(ACL_FIBER_RWLOCK *lk)
{
	acl_myfree(lk);
}

static int __rlock(ACL_FIBER_RWLOCK *lk, int block)
{
	ACL_FIBER *curr;

	if (lk->writer == NULL && FIRST_FIBER(&lk->wwaiting) == NULL) {
		lk->readers++;
		return 1;
	}

	if (!block)
		return 0;

	curr = acl_fiber_running();
	acl_ring_prepend(&lk->rwaiting, &curr->me);
	acl_fiber_switch();

	/* if switch to me because other killed me, I should detach myself */
	acl_ring_detach(&curr->me);

	return 1;
}

void acl_fiber_rwlock_rlock(ACL_FIBER_RWLOCK *lk)
{
	(void) __rlock(lk, 1);
}

int acl_fiber_rwlock_tryrlock(ACL_FIBER_RWLOCK *lk)
{
	return __rlock(lk, 0);
}

static int __wlock(ACL_FIBER_RWLOCK *lk, int block)
{
	ACL_FIBER *curr;

	if (lk->writer == NULL && lk->readers == 0) {
		lk->writer = acl_fiber_running();
		return 1;
	}

	if (!block)
		return 0;

	curr = acl_fiber_running();
	acl_ring_prepend(&lk->wwaiting, &curr->me);
	acl_fiber_switch();

	/* if switch to me because other killed me, I should detach myself */
	acl_ring_detach(&curr->me);

	return 1;
}

void acl_fiber_rwlock_wlock(ACL_FIBER_RWLOCK *lk)
{
	__wlock(lk, 1);
}

int acl_fiber_rwlock_trywlock(ACL_FIBER_RWLOCK *lk)
{
	return __wlock(lk, 0);
}

void acl_fiber_rwlock_runlock(ACL_FIBER_RWLOCK *lk)
{
	ACL_FIBER *fiber;

	if (--lk->readers == 0 && (fiber = FIRST_FIBER(&lk->wwaiting))) {
		acl_ring_detach(&lk->wwaiting);
		lk->writer = fiber;
		acl_fiber_ready(fiber);
	}
}

void acl_fiber_rwlock_wunlock(ACL_FIBER_RWLOCK *lk)
{
	ACL_FIBER *fiber;
	
	if (lk->writer == NULL)
		acl_msg_fatal("%s(%d), %s: wunlock: not locked",
			__FILE__, __LINE__, __FUNCTION__);

	lk->writer = NULL;

	if (lk->readers != 0)
		acl_msg_fatal("%s(%d), %s: wunlock: readers",
			__FILE__, __LINE__, __FUNCTION__);

	while ((fiber = FIRST_FIBER(&lk->rwaiting)) != NULL) {
		acl_ring_detach(&lk->rwaiting);
		lk->readers++;
		acl_fiber_ready(fiber);
	}

	if (lk->readers == 0 && (fiber = FIRST_FIBER(&lk->wwaiting)) != NULL) {
		acl_ring_detach(&lk->wwaiting);
		lk->writer = fiber;
		acl_fiber_ready(fiber);
	}
}
