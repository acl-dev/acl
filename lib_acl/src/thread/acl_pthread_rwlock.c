#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#endif

#ifndef	_GNU_SOURCE
# ifdef	ACL_HAVE_NO_RWLOCK
#  ifdef	ACL_UNIX
#   include <limits.h>
#  endif
#  include <stdlib.h>
#  include <errno.h>
#  include "stdlib/acl_mymalloc.h"
#  include "thread/acl_pthread.h"
#  include "thread/acl_pthread_rwlock.h"

/* maximum number of times a read lock may be obtained */
#  define	MAX_READ_LOCKS		65535

int acl_pthread_rwlock_destroy(acl_pthread_rwlock_t *rwlock)
{
	acl_pthread_rwlock_t prwlock;

	if (rwlock == NULL)
		return EINVAL;

	prwlock = *rwlock;

	acl_pthread_mutex_destroy(&prwlock->lock);
	acl_pthread_cond_destroy(&prwlock->read_signal);
	acl_pthread_cond_destroy(&prwlock->write_signal);
	acl_myfree(prwlock);

	*rwlock = NULL;

	return 0;
}

int acl_pthread_rwlock_init (acl_pthread_rwlock_t *rwlock,
	const acl_pthread_rwlockattr_t *attr acl_unused)
{
	acl_pthread_rwlock_t prwlock;
	int   ret;

	/* allocate rwlock object */
	prwlock = (acl_pthread_rwlock_t)
		acl_mymalloc(sizeof(struct acl_pthread_rwlock));

	if (prwlock == NULL)
		return ENOMEM;

	/* initialize the lock */
	if ((ret = acl_pthread_mutex_init(&prwlock->lock, NULL)) != 0) {
		acl_myfree(prwlock);
		return ret;
	}

	/* initialize the read condition signal */
	ret = acl_pthread_cond_init(&prwlock->read_signal, NULL);
	if (ret != 0) {
		acl_pthread_mutex_destroy(&prwlock->lock);
		acl_myfree(prwlock);
		return ret;
	}

	/* initialize the write condition signal */
	ret = acl_pthread_cond_init(&prwlock->write_signal, NULL);
	if (ret != 0) {
		acl_pthread_cond_destroy(&prwlock->read_signal);
		acl_pthread_mutex_destroy(&prwlock->lock);
		acl_myfree(prwlock);
		return ret;
	}

	/* success */
	prwlock->state = 0;
	prwlock->blocked_writers = 0;
	*rwlock = prwlock;

	return 0;
}

int acl_pthread_rwlock_rdlock(acl_pthread_rwlock_t *rwlock)
{
	acl_pthread_rwlock_t prwlock;
	int   ret;

	if (rwlock == NULL)
		return EINVAL;

	prwlock = *rwlock;

	/* check for static initialization */
	if (prwlock == NULL)
		return EINVAL;

	/* grab the monitor lock */
	if ((ret = acl_pthread_mutex_lock(&prwlock->lock)) != 0)
		return ret;

	/* give writers priority over readers */
	while (prwlock->blocked_writers || prwlock->state < 0) {
		ret = acl_pthread_cond_wait(&prwlock->read_signal,
				&prwlock->lock);

		if (ret != 0) {
			/* can't do a whole lot if this fails */
			acl_pthread_mutex_unlock(&prwlock->lock);
			return ret;
		}
	}

	/* check lock count */
	if (prwlock->state == MAX_READ_LOCKS)
		ret = EAGAIN;
	else
		++prwlock->state; /* indicate we are locked for reading */

	/*
	 * Something is really wrong if this call fails.  Returning
	 * error won't do because we've already obtained the read
	 * lock.  Decrementing 'state' is no good because we probably
	 * don't have the monitor lock.
	 */
	acl_pthread_mutex_unlock(&prwlock->lock);

	return ret;
}

int acl_pthread_rwlock_tryrdlock(acl_pthread_rwlock_t *rwlock)
{
	acl_pthread_rwlock_t prwlock;
	int   ret;

	if (rwlock == NULL)
		return EINVAL;

	prwlock = *rwlock;

	/* check for static initialization */
	if (prwlock == NULL)
		return EINVAL;

	/* grab the monitor lock */
	if ((ret = acl_pthread_mutex_lock(&prwlock->lock)) != 0)
		return ret;

	/* give writers priority over readers */
	if (prwlock->blocked_writers || prwlock->state < 0)
		ret = ACL_EWOULDBLOCK;
	else if (prwlock->state == MAX_READ_LOCKS)
		ret = ACL_EAGAIN; /* too many read locks acquired */
	else
		++prwlock->state; /* indicate we are locked for reading */

	/* see the comment on this in pthread_rwlock_rdlock */
	acl_pthread_mutex_unlock(&prwlock->lock);

	return ret;
}

int acl_pthread_rwlock_trywrlock(acl_pthread_rwlock_t *rwlock)
{
	acl_pthread_rwlock_t prwlock;
	int   ret;

	if (rwlock == NULL)
		return EINVAL;

	prwlock = *rwlock;

	/* check for static initialization */
	if (prwlock == NULL)
		return EINVAL;

	/* grab the monitor lock */
	if ((ret = acl_pthread_mutex_lock(&prwlock->lock)) != 0)
		return ret;

	if (prwlock->state != 0)
		ret = ACL_EWOULDBLOCK;
	else
		/* indicate we are locked for writing */
		prwlock->state = -1;

	/* see the comment on this in pthread_rwlock_rdlock */
	acl_pthread_mutex_unlock(&prwlock->lock);

	return ret;
}

int acl_pthread_rwlock_unlock(acl_pthread_rwlock_t *rwlock)
{
	acl_pthread_rwlock_t prwlock;
	int   ret;

	if (rwlock == NULL)
		return EINVAL;

	prwlock = *rwlock;

	/* check for static initialization */
	if (prwlock == NULL)
		return EINVAL;

	/* grab the monitor lock */
	if ((ret = acl_pthread_mutex_lock(&prwlock->lock)) != 0)
		return ret;

	if (prwlock->state > 0) {
		if (--prwlock->state == 0 && prwlock->blocked_writers)
			ret = acl_pthread_cond_signal(&prwlock->write_signal);
	} else if (prwlock->state < 0) {
		prwlock->state = 0;

		if (prwlock->blocked_writers)
			ret = acl_pthread_cond_signal(&prwlock->write_signal);
		else
			ret = acl_pthread_cond_broadcast(&prwlock->read_signal);
	} else
		ret = EINVAL;

	/* see the comment on this in pthread_rwlock_rdlock */
	acl_pthread_mutex_unlock(&prwlock->lock);

	return ret;
}

int acl_pthread_rwlock_wrlock(acl_pthread_rwlock_t *rwlock)
{
	acl_pthread_rwlock_t prwlock;
	int   ret;

	if (rwlock == NULL)
		return EINVAL;

	prwlock = *rwlock;

	/* check for static initialization */
	if (prwlock == NULL)
		return EINVAL;

	/* grab the monitor lock */
	if ((ret = acl_pthread_mutex_lock(&prwlock->lock)) != 0)
		return ret;

	while (prwlock->state != 0) {
		++prwlock->blocked_writers;

		ret = acl_pthread_cond_wait(&prwlock->write_signal,
				&prwlock->lock);

		if (ret != 0) {
			--prwlock->blocked_writers;
			acl_pthread_mutex_unlock(&prwlock->lock);
			return ret;
		}

		--prwlock->blocked_writers;
	}

	/* indicate we are locked for writing */
	prwlock->state = -1;

	/* see the comment on this in pthread_rwlock_rdlock */
	acl_pthread_mutex_unlock(&prwlock->lock);

	return ret;
}

int acl_pthread_rwlockattr_destroy(acl_pthread_rwlockattr_t *rwlockattr)
{
	acl_pthread_rwlockattr_t prwlockattr;

	if (rwlockattr == NULL)
		return EINVAL;

	prwlockattr = *rwlockattr;

	/* check for static initialization */
	if (prwlockattr == NULL)
		return EINVAL;

	acl_myfree(prwlockattr);

	return 0;
}

int acl_pthread_rwlockattr_getpshared(const acl_pthread_rwlockattr_t *rwlockattr,
	int *pshared)
{
	*pshared = (*rwlockattr)->pshared;

	return 0;
}

int acl_pthread_rwlockattr_init(acl_pthread_rwlockattr_t *rwlockattr)
{
	acl_pthread_rwlockattr_t prwlockattr;

	if (rwlockattr == NULL)
		return EINVAL;

	prwlockattr = (acl_pthread_rwlockattr_t)
		acl_mymalloc(sizeof(struct acl_pthread_rwlockattr));

	if (prwlockattr == NULL)
		return ENOMEM;

	prwlockattr->pshared 	= ACL_PTHREAD_PROCESS_PRIVATE;
	*rwlockattr		= prwlockattr;

	return 0;
}

int acl_pthread_rwlockattr_setpshared(acl_pthread_rwlockattr_t *rwlockattr,
	int pshared)
{
	/* Only PTHREAD_PROCESS_PRIVATE is supported. */
	if (pshared != ACL_PTHREAD_PROCESS_PRIVATE)
		return EINVAL;

	(*rwlockattr)->pshared = pshared;

	return 0;
}

# endif /* ACL_HAVE_NO_RWLOCK */
#endif /* _GNU_SOURCE */
