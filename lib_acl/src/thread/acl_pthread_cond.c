#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_mymalloc.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#endif

#ifndef	ACL_HAS_PTHREAD

#include "stdlib/acl_sys_patch.h"
#include "thread/acl_sem.h"
#include "thread/acl_pthread.h"

int acl_pthread_cond_init(acl_pthread_cond_t *cond,
	acl_pthread_condattr_t *cond_attr)
{
	const char *myname = "acl_pthread_cond_init";

	(void) cond_attr;

	if (cond == NULL)
		acl_msg_fatal("%s, %s(%d): input invalid",
			__FILE__, myname, __LINE__);

	cond->lock      = acl_mycalloc(1, sizeof(acl_pthread_mutex_t));
	if (acl_pthread_mutex_init(cond->lock, NULL) != 0)
		acl_msg_fatal("%s, %s(%d): acl_pthread_mutex_init error",
			__FILE__, myname, __LINE__);
	cond->dynamic   = 0;
	cond->wait_sem  = acl_sem_create(0);
	cond->wait_done = acl_sem_create(0);
	cond->waiting   = cond->nsignal = 0;

	if (!cond->lock || !cond->wait_sem || !cond->wait_done)
		return -1;
	return 0;
}

/* Create a condition variable */
acl_pthread_cond_t * acl_thread_cond_create(void)
{
	const char *myname = "acl_thread_cond_create";
	acl_pthread_cond_t *cond;

	cond = (acl_pthread_cond_t *)
		acl_mycalloc(1, sizeof(acl_pthread_cond_t));
	if (cond == NULL) {
		acl_msg_error("%s, %s(%d): calloc error(%s)",
			__FILE__, myname, __LINE__, acl_last_serror());
		return NULL;
	}
	
	if (acl_pthread_cond_init(cond, NULL) < 0) {
		acl_pthread_cond_destroy(cond);
		return NULL;
	}

	cond->dynamic = 1;
	return cond;
}

/* Destroy a condition variable */

int acl_pthread_cond_destroy(acl_pthread_cond_t *cond)
{
	if (cond == NULL)
		return -1;

	if (cond->wait_sem)
		acl_sem_destroy(cond->wait_sem);
	if (cond->wait_done)
		acl_sem_destroy(cond->wait_done);
	if (cond->lock) {
		acl_pthread_mutex_destroy(cond->lock);
		acl_myfree(cond->lock);
	}

	if (cond->dynamic)
		acl_myfree(cond);

	return 0;
}

/* Restart one of the threads that are waiting on the condition variable */
int acl_pthread_cond_signal(acl_pthread_cond_t *cond)
{
	const char *myname = "acl_pthread_cond_signal";

	if (cond == NULL) {
		acl_msg_error("%s, %s(%d): input invalid",
			__FILE__, myname, __LINE__);
		return -1;
	}

	/* If there are waiting threads not already signalled, then
	 *  signal the condition and wait for the thread to respond.
	 */
	acl_pthread_mutex_lock(cond->lock);
	if (cond->waiting > cond->nsignal) {
		++cond->nsignal;
		acl_sem_post(cond->wait_sem);
		acl_pthread_mutex_unlock(cond->lock);
		acl_sem_wait(cond->wait_done);
	} else
		acl_pthread_mutex_unlock(cond->lock);

	return 0;
}

/* Restart all threads that are waiting on the condition variable */
int acl_pthread_cond_broadcast(acl_pthread_cond_t *cond)
{
	const char *myname = "acl_pthread_cond_broadcast";

	if (cond == NULL) {
		acl_msg_error("%s, %s(%d): input invalid",
			__FILE__, myname, __LINE__);
		return -1;
	}

	/* If there are waiting threads not already signalled, then
	 * signal the condition and wait for the thread to respond.
	 */
	acl_pthread_mutex_lock(cond->lock);
	if (cond->waiting > cond->nsignal) {
		int i, num_waiting;

		num_waiting = (cond->waiting - cond->nsignal);
		cond->nsignal = cond->waiting;
		for (i = 0; i < num_waiting; ++i)
			acl_sem_post(cond->wait_sem);

		/* Now all released threads are blocked here, waiting for us.
		 * Collect them all (and win fabulous prizes!) :-)
		 */
		acl_pthread_mutex_unlock(cond->lock);
		for (i = 0; i < num_waiting; ++i)
			acl_sem_wait(cond->wait_done);
	} else
		acl_pthread_mutex_unlock(cond->lock);

	return 0;
}

/* Wait on the condition variable for at most 'ms' milliseconds.
 * The mutex must be locked before entering this function!
 * The mutex is unlocked during the wait, and locked again after the wait.

Typical use:

Thread A:
	pthread_mutex_lock(lock);
	while ( ! condition ) {
		SDL_CondWait(cond);
	}
	pthread_mutex_unlock(lock);

Thread B:
	pthread_mutex_lock(lock);
	...
	condition = true;
	...
	pthread_mutex_unlock(lock);
 */
int acl_pthread_cond_timedwait(acl_pthread_cond_t *cond,
	acl_pthread_mutex_t *mutex, const struct timespec *timeout)
{
	const char *myname = "acl_pthread_cond_timedwait";
	int   retval;
	
	if (cond == NULL) {
		acl_msg_error("%s, %s(%d): input invalid",
			__FILE__, myname, __LINE__);
		return -1;
	}

	/* Obtain the protection mutex, and increment the number of waiters.
	 * This allows the signal mechanism to only perform a signal if there
	 * are waiting threads.
	 */
	acl_pthread_mutex_lock(cond->lock);
	++cond->waiting;
	acl_pthread_mutex_unlock(cond->lock);

	/* Unlock the mutex, as is required by condition variable semantics */
	acl_pthread_mutex_unlock(mutex);

	/* Wait for a signal */
	if (timeout == NULL)
		retval = acl_sem_wait(cond->wait_sem);
	else {
		int ms;
		struct timeval tv;

		gettimeofday(&tv, NULL);
		ms = (int) (timeout->tv_sec * 1000 + timeout->tv_nsec / 1000000);
		ms -= tv.tv_sec * 1000 + tv.tv_usec / 1000;
		if (ms < 0)
			ms = 0;
		retval = acl_sem_wait_timeout(cond->wait_sem, (unsigned int) ms);
	}

	/* Let the signaler know we have completed the wait, otherwise
         * the signaler can race ahead and get the condition semaphore
         * if we are stopped between the mutex unlock and semaphore wait,
         * giving a deadlock.  See the following URL for details:
         * http://www-classic.be.com/aboutbe/benewsletter/volume_III/Issue40.html
	 */
	acl_pthread_mutex_lock(cond->lock);
	if (cond->nsignal > 0) {
		/* If we timed out, we need to eat a condition signal */
		if (retval > 0)
			acl_sem_wait(cond->wait_sem);

		/* We always notify the signal thread that we are done */
		acl_sem_post(cond->wait_done);

		/* Signal handshake complete */
		--cond->nsignal;
	}
	--cond->waiting;
	acl_pthread_mutex_unlock(cond->lock);

	/* Lock the mutex, as is required by condition variable semantics */
	acl_pthread_mutex_lock(mutex);

	return retval;
}

/* Wait on the condition variable forever */

int acl_pthread_cond_wait(acl_pthread_cond_t *cond, acl_pthread_mutex_t *mutex)
{
	return acl_pthread_cond_timedwait(cond, mutex, NULL);
}

#endif /* !ACL_HAS_PTHREAD */
