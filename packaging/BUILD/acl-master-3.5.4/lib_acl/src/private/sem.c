/* Semaphore functions using the Win32 API */
#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#endif

#ifdef	ACL_WINDOWS

#include <windows.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "thread/acl_pthread.h"
#include "thread/acl_sem.h"

#include "sem.h"

/* Create a semaphore */
ACL_SEM *sem_create2(const char *pathname, unsigned int initial_value)
{
	ACL_SEM *sem;

	/* Allocate sem memory */
	sem = (ACL_SEM *) malloc(sizeof(*sem));
	acl_assert(sem);
	/* Create the semaphore, with max value 32K */
	sem->id = CreateSemaphore(NULL, initial_value, 32 * 1024, pathname);
	sem->count = initial_value;
	acl_assert(sem->id);

	return(sem);
}

ACL_SEM *sem_create(unsigned int initial_value)
{
	return (sem_create2(NULL, initial_value));
}

/* Free the semaphore */
void sem_destroy(ACL_SEM *sem)
{
	if (sem) {
		if (sem->id) {
			CloseHandle(sem->id);
			sem->id = 0;
		}
		free(sem);
	}
}

int sem_wait_timeout(ACL_SEM *sem, unsigned int timeout)
{
	int   retval;
	DWORD dwMilliseconds;

	acl_assert(sem != NULL);

	if (timeout == ACL_MUTEX_MAXWAIT) {
		dwMilliseconds = INFINITE;
	} else {
		dwMilliseconds = (DWORD) timeout;
	}

	switch (WaitForSingleObject(sem->id, dwMilliseconds)) {
	case WAIT_OBJECT_0:
		--sem->count;
		retval = 0;
		break;
	case WAIT_TIMEOUT:
		retval = ACL_ETIMEDOUT;
		break;
	default:
		retval = -1;
		break;
	}
	return retval;
}

int sem_try_wait(ACL_SEM *sem)
{
	return sem_wait_timeout(sem, 0);
}

int sem_wait(ACL_SEM *sem)
{
	return sem_wait_timeout(sem, ACL_MUTEX_MAXWAIT);
}

/* Returns the current count of the semaphore */
unsigned int sem_value(ACL_SEM *sem)
{
	acl_assert(sem != NULL);
	return sem->count;
}

int sem_post(ACL_SEM *sem)
{
	acl_assert(sem != NULL);
	/* Increase the counter in the first place, because
	 * after a successful release the semaphore may
	 * immediately get destroyed by another thread which
	 * is waiting for this semaphore.
	 */
	++sem->count;

	if (ReleaseSemaphore(sem->id, 1, NULL) == FALSE) {
		--sem->count;	/* restore */
		return -1;
	}
	return 0;
}

#endif /* ACL_WINDOWS */
