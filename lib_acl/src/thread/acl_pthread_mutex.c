#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <string.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_dbuf_pool.h"
#include "stdlib/acl_ring.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_msg.h"
#include "thread/acl_pthread.h"
#endif

#ifdef	ACL_WINDOWS

int acl_pthread_mutex_init(acl_pthread_mutex_t *mutex,
	const acl_pthread_mutexattr_t *mattr)
{
	const char *myname = "acl_pthread_mutex_init";

	if (mutex == NULL) {
		acl_msg_error("%s, %s(%d): input invalid",
			__FILE__, myname, __LINE__);
		return -1;
	}

	mutex->dynamic = 0;

	/* Create the mutex, with initial value signaled */
	mutex->id = CreateMutex((SECURITY_ATTRIBUTES *) mattr, FALSE, NULL);
	if (!mutex->id) {
		acl_msg_error("%s, %s(%d): CreateMutex error(%s)",
			__FILE__, myname, __LINE__, acl_last_serror());
		acl_myfree(mutex);
		return -1;
	}

	return 0;
}

/* Free the mutex */
int acl_pthread_mutex_destroy(acl_pthread_mutex_t *mutex)
{
	if (mutex) {
		if (mutex->id) {
			CloseHandle(mutex->id);
			mutex->id = 0;
		}
		if (mutex->dynamic)
			acl_myfree(mutex);
		return 0;
	} else
		return -1;
}

int acl_pthread_mutex_lock(acl_pthread_mutex_t *mutex)
{
	const char *myname = "acl_pthread_mutex_lock";

	if (mutex == NULL) {
		acl_msg_error("%s, %s(%d): input invalid",
			__FILE__, myname, __LINE__);
		return -1;
	}

	if (WaitForSingleObject(mutex->id, INFINITE) == WAIT_FAILED) {
		acl_msg_error("%s, %s(%d): WaitForSingleObject error(%s)",
			__FILE__, myname, __LINE__, acl_last_serror());
		return -1;
	}

	return 0;
}

int acl_pthread_mutex_unlock(acl_pthread_mutex_t *mutex)
{
	const char *myname = "acl_pthread_mutex_unlock";

	if (mutex == NULL) {
		acl_msg_error("%s, %s(%d): input invalid",
			__FILE__, myname, __LINE__);
		return -1;
	}

	if (ReleaseMutex(mutex->id) == FALSE) {
		acl_msg_error("%s, %s(%d): ReleaseMutex error(%s)",
			__FILE__, myname, __LINE__, acl_last_serror());
		return -1;
	}

	return 0;
}

#endif /* ACL_WINDOWS */

typedef struct acl_pthread_nested_mutex_t acl_pthread_nested_mutex_t;

struct acl_pthread_nested_mutex_t {
	acl_pthread_mutex_t *mutex;
	ACL_RING ring;
	int   nrefer;
	char *ptr;
};

static acl_pthread_key_t __header_key;

static void free_header(void *arg)
{
	ACL_RING *header_ptr = (ACL_RING*) arg;
	ACL_RING_ITER iter;
	acl_pthread_nested_mutex_t *tmp;

	if (header_ptr == NULL)
		return;

	acl_ring_foreach(iter, header_ptr) {
		tmp = ACL_RING_TO_APPL(iter.ptr,
			acl_pthread_nested_mutex_t, ring);
		if (tmp->mutex != NULL && tmp->nrefer > 0)
			acl_pthread_mutex_unlock(tmp->mutex);
	}
}

static void acl_thread_mutex_init_once(void)
{
	acl_pthread_key_create(&__header_key, free_header);
}

static acl_pthread_once_t thread_mutex_once_control = ACL_PTHREAD_ONCE_INIT;

int acl_thread_mutex_lock(acl_pthread_mutex_t *mutex)
{
	ACL_RING_ITER iter;
	acl_pthread_nested_mutex_t *tmp, *nested_mutex = NULL;
	ACL_RING *header_ptr;

	if (mutex == NULL)
		return -1;

	acl_pthread_once(&thread_mutex_once_control,
		acl_thread_mutex_init_once);

	header_ptr = acl_pthread_getspecific(__header_key);

	if (header_ptr == NULL) {
		header_ptr = (ACL_RING*) acl_mymalloc(sizeof(ACL_RING));
		acl_ring_init(header_ptr);
		acl_pthread_setspecific(__header_key, header_ptr);
	}

	acl_ring_foreach(iter, header_ptr) {
		tmp = ACL_RING_TO_APPL(iter.ptr,
			acl_pthread_nested_mutex_t, ring);
		if (tmp->mutex == mutex) {
			nested_mutex = tmp;
			break;
		}
	}

	if (nested_mutex == NULL) {
		nested_mutex = (acl_pthread_nested_mutex_t*)
			acl_mymalloc(sizeof(acl_pthread_nested_mutex_t));
		acl_pthread_mutex_lock(mutex);
		nested_mutex->mutex = mutex;
		nested_mutex->nrefer = 1;
		ACL_RING_APPEND(header_ptr, &nested_mutex->ring);
	} else
		nested_mutex->nrefer++;
	return 0;
}

int acl_thread_mutex_unlock(acl_pthread_mutex_t *mutex)
{
	ACL_RING_ITER iter;
	acl_pthread_nested_mutex_t *tmp, *nested_mutex = NULL;
	ACL_RING *header_ptr = acl_pthread_getspecific(__header_key);

	if (mutex == NULL || header_ptr == NULL)
		return -1;

	acl_ring_foreach(iter, header_ptr) {
		tmp = ACL_RING_TO_APPL(iter.ptr,
			acl_pthread_nested_mutex_t, ring);
		if (tmp->mutex == mutex) {
			nested_mutex = tmp;
			break;
		}
	}
	
	if (nested_mutex == NULL)
		return -1;
	if (--nested_mutex->nrefer == 0) {
		ACL_RING_DETACH(&nested_mutex->ring);
		acl_myfree(nested_mutex);
		acl_pthread_mutex_unlock(mutex);
	}

	return 0;
}

int acl_thread_mutex_nested(acl_pthread_mutex_t *mutex)
{
	ACL_RING_ITER iter;
	acl_pthread_nested_mutex_t *tmp, *nested_mutex = NULL;
	ACL_RING *header_ptr = acl_pthread_getspecific(__header_key);

	if (mutex == NULL || header_ptr == NULL)
		return -1;

	acl_ring_foreach(iter, header_ptr) {
		tmp = ACL_RING_TO_APPL(iter.ptr,
			acl_pthread_nested_mutex_t, ring);
		if (tmp->mutex == mutex) {
			nested_mutex = tmp;
			break;
		}
	}

	if (nested_mutex == NULL)
		return 0;
	return nested_mutex->nrefer;
}
