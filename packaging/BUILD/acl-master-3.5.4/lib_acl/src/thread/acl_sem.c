/* Semaphore functions using the Win32 API */
#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_msg.h"

#endif

#ifdef	ACL_WINDOWS

#include <windows.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "thread/acl_pthread.h"
#include "thread/acl_sem.h"

/* Create a semaphore */
ACL_SEM *acl_sem_create2(const char *pathname, unsigned int initial_value)
{
	const char *myname = "acl_sem_create2";
	ACL_SEM *sem;

	/* Allocate sem memory */
	sem = (ACL_SEM *) acl_mymalloc(sizeof(*sem));
	if (sem == NULL) {
		acl_msg_error("%s, %s(%d): malloc error(%s)",
			__FILE__, myname, __LINE__, acl_last_serror());
		return NULL;
	}
	/* Create the semaphore, with max value 32K */
	sem->id = CreateSemaphore(NULL, initial_value, 32 * 1024, pathname);
	sem->count = initial_value;
	if (!sem->id) {
		acl_msg_error("%s, %s(%d): Couldn't create semaphore(%s)",
			__FILE__, myname, __LINE__, acl_last_serror());
		acl_myfree(sem);
		return NULL;
	}

	return sem;
}

ACL_SEM *acl_sem_create(unsigned int initial_value)
{
	return acl_sem_create2(NULL, initial_value);
}

/* Free the semaphore */
void acl_sem_destroy(ACL_SEM *sem)
{
	if (sem) {
		if (sem->id) {
			CloseHandle(sem->id);
			sem->id = 0;
		}
		acl_myfree(sem);
	}
}

int acl_sem_wait_timeout(ACL_SEM *sem, unsigned int timeout)
{
	const char *myname = "acl_sem_wait_timeout";
	int   retval;
	DWORD dwMilliseconds;

	if (sem == NULL) {
		acl_msg_error("%s, %s(%d): input invalid",
			__FILE__, myname, __LINE__);
		return -1;
	}

	if (timeout == ACL_MUTEX_MAXWAIT)
		dwMilliseconds = INFINITE;
	else
		dwMilliseconds = (DWORD) timeout;

	switch (WaitForSingleObject(sem->id, dwMilliseconds)) {
	case WAIT_OBJECT_0:
		--sem->count;
		retval = 0;
		break;
	case WAIT_TIMEOUT:
		retval = ACL_ETIMEDOUT;
		break;
	default:
		acl_msg_error("%s, %s(%d): WaitForSingleObject() failed",
			__FILE__, myname, __LINE__, acl_last_serror());
		retval = -1;
		break;
	}
	return retval;
}

int acl_sem_try_wait(ACL_SEM *sem)
{
	return acl_sem_wait_timeout(sem, 0);
}

int acl_sem_wait(ACL_SEM *sem)
{
	return acl_sem_wait_timeout(sem, ACL_MUTEX_MAXWAIT);
}

/* Returns the current count of the semaphore */
unsigned int acl_sem_value(ACL_SEM *sem)
{
	const char *myname = "acl_sem_value";

	if (sem == NULL) {
		acl_msg_error("%s, %s(%d): input invalid",
			__FILE__, myname, __LINE__);
		return 0;
	}
	return sem->count;
}

int acl_sem_post(ACL_SEM *sem)
{
	const char *myname = "acl_sem_post";

	if (sem == NULL) {
		acl_msg_error("%s, %s(%d): input invalid",
			__FILE__, myname, __LINE__);
		return -1;
	}

	/* Increase the counter in the first place, because
	 * after a successful release the semaphore may
	 * immediately get destroyed by another thread which
	 * is waiting for this semaphore.
	 */
	++sem->count;

	if (ReleaseSemaphore(sem->id, 1, NULL) == FALSE) {
		--sem->count;	/* restore */
		acl_msg_error("%s, %s(%d): ReleaseSemaphore() failed",
			__FILE__, myname, __LINE__, acl_last_serror());
		return -1;
	}

	return 0;
}

#endif /* ACL_WINDOWS */
