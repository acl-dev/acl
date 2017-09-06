#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdlib.h>
#include <string.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "thread/acl_pthread.h"

#endif

#include "sem.h"
#include "thread.h"
#ifndef	ACL_HAS_PTHREAD

int thread_mutex_init(acl_pthread_mutex_t *mutex,
	const acl_pthread_mutexattr_t *mattr)
{
	acl_assert(mutex != NULL);

	mutex->dynamic = 0;

	/* Create the mutex, with initial value signaled */
	mutex->id = CreateMutex((SECURITY_ATTRIBUTES *) mattr, FALSE, NULL);
	acl_assert(mutex->id);
	return 0;
}

acl_pthread_mutex_t *thread_mutex_create(void)
{
	acl_pthread_mutex_t *mutex;

	mutex = calloc(1, sizeof(acl_pthread_mutex_t));
	acl_assert(mutex);
	mutex->dynamic = 1;

	/* Create the mutex, with initial value signaled */
	mutex->id = CreateMutex(NULL, FALSE, NULL);
	acl_assert(mutex->id);
	return mutex;
}

/* Free the mutex */
int thread_mutex_destroy(acl_pthread_mutex_t *mutex)
{
	if (mutex) {
		if (mutex->id) {
			CloseHandle(mutex->id);
			mutex->id = 0;
		}
		if (mutex->dynamic)
			free(mutex);
		return 0;
	} else
		return -1;
}

int thread_mutex_lock(acl_pthread_mutex_t *mutex)
{
	acl_assert(mutex);

	if (WaitForSingleObject(mutex->id, INFINITE) == WAIT_FAILED)
		return -1;

	return 0;
}

int thread_mutex_unlock(acl_pthread_mutex_t *mutex)
{
	acl_assert(mutex);

	if (ReleaseMutex(mutex->id) == FALSE)
		return -1;

	return 0;
}

#elif	defined(ACL_UNIX)

#include <pthread.h>

acl_pthread_mutex_t *thread_mutex_create(void)
{
	acl_pthread_mutex_t *mutex;
	int   ret;

	mutex = (acl_pthread_mutex_t *) malloc(sizeof(acl_pthread_mutex_t));
	acl_assert(mutex);
	if ((ret = pthread_mutex_init(mutex, NULL)) != 0) {
		printf("pthread_mutex_init error %s\r\n", strerror(ret));
		free(mutex);
		return NULL;
	}

	return mutex;
}

int thread_mutex_destroy(acl_pthread_mutex_t *mutex)
{
	pthread_mutex_destroy(mutex);
	free(mutex);
	return 0;
}

#endif /* ACL_HAS_PTHREAD */
