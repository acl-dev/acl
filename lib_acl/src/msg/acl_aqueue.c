#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include <errno.h>
#ifdef	ACL_UNIX
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#elif	defined(WIN32)
#include <time.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#ifdef WIN32
#pragma once
#endif

#endif	/* defined(WIN32) */

#include "stdlib/acl_sys_patch.h"
#include "thread/acl_thread.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_mymalloc.h"
#include "msg/acl_aqueue.h"

#endif

#undef	__SET_ERRNO
#ifdef	WIN32
# define	__SET_ERRNO(_x_) (void) 0
#elif	defined(ACL_UNIX)
# define	__SET_ERRNO(_x_) (acl_set_error(_x_))
#else
# error "unknown OS type"
#endif

/* 内部结果类型定义 */

struct ACL_AQUEUE_ITEM {
	struct ACL_AQUEUE_ITEM *next;
	void *data;
};

struct ACL_AQUEUE {
	ACL_AQUEUE_ITEM *first, *last;
	int   qlen;
	int   error;
	int   quit;
	int   nlink;
	char  check_owner;
	unsigned long owner;
	acl_pthread_mutex_t lock;
	acl_pthread_cond_t  cond;
};

ACL_AQUEUE *acl_aqueue_new(void)
{
	const char *myname = "acl_aqueue_new";
	ACL_AQUEUE *queue;
	char  buf[256];

	queue = (ACL_AQUEUE *) acl_mymalloc(sizeof(ACL_AQUEUE));
	if (queue == NULL)
		acl_msg_fatal("%s: malloc error(%s)",
			myname, acl_last_strerror(buf, sizeof(buf)));

	acl_pthread_mutex_init(&queue->lock, NULL);
	acl_pthread_cond_init(&queue->cond, NULL);

	queue->first = queue->last = NULL;
	queue->qlen = 0;
	queue->error = ACL_AQUEUE_OK;
	queue->quit = 0;
	queue->nlink = 0;
	queue->owner = (unsigned long) acl_pthread_self();
	queue->check_owner = 0;
	
	return (queue);
}

void acl_aqueue_check_owner(ACL_AQUEUE *queue, char flag)
{
	if (queue)
		queue->check_owner = flag;
}

void acl_aqueue_set_owner(ACL_AQUEUE *queue, unsigned int owner)
{
	if (queue)
		queue->owner = owner;
}

void acl_aqueue_free(ACL_AQUEUE *queue, ACL_AQUEUE_FREE_FN free_fn)
{
	const char *myname = "acl_aqueue_free";
	ACL_AQUEUE_ITEM *qi;
	char  buf[256];
	int   status;

	if (queue == NULL)
		return;

	if (queue->check_owner && (unsigned long) acl_pthread_self() != queue->owner) {
		acl_msg_error("%s: cur tid(%lu) != owner(%lu), denied",
				myname, (unsigned long) acl_pthread_self(),
				queue->owner);
		return;
	}

	queue->quit = 1;
	status = acl_pthread_mutex_lock(&queue->lock);
	if (status != 0)
		acl_msg_error("%s: lock error(%s)",
				myname, acl_last_strerror(buf, sizeof(buf)));

	while (1) {
		qi = queue->first;
		if (qi == NULL)
			break;
		queue->first = qi->next;
		if (free_fn != NULL)
			free_fn(qi->data);
		acl_myfree(qi);
	}

	status = acl_pthread_mutex_unlock(&queue->lock);
	if (status != 0)
		acl_msg_error("%s: lock error(%s)",
				myname, acl_last_strerror(buf, sizeof(buf)));

	acl_pthread_mutex_destroy(&queue->lock);
	acl_pthread_cond_destroy(&queue->cond);
	acl_myfree(queue);
}

void *acl_aqueue_pop(ACL_AQUEUE *queue)
{
	return (acl_aqueue_pop_timedwait(queue, -1, -1));
}

static int aqueue_wait(ACL_AQUEUE *queue, const struct  timespec *ptimeout)
{
	const char *myname = "aqueue_wait";
	char  buf[256];
	int   status;

	while (queue->first == NULL && queue->quit == 0) {
		if (ptimeout != NULL)
			status = acl_pthread_cond_timedwait(&queue->cond,
					&queue->lock, ptimeout);
		else
			status = acl_pthread_cond_wait(&queue->cond, &queue->lock);

		if (ptimeout && status == ACL_ETIMEDOUT) {
			status = acl_pthread_mutex_unlock(&queue->lock);
			if (status != 0)
				acl_msg_error("%s(%d): unlock error(%s)",
					myname, __LINE__,
					acl_last_strerror(buf, sizeof(buf)));

			queue->error = ACL_AQUEUE_ERR_TIMEOUT;
			return (-1);
		} else if (status != 0) {
			status = acl_pthread_mutex_unlock(&queue->lock);
			if (status != 0)
				acl_msg_error("%s(%d): unlock error(%s)",
					myname, __LINE__,
					acl_last_strerror(buf, sizeof(buf)));

			queue->error = ACL_AQUEUE_ERR_COND_WAIT;
			__SET_ERRNO(status);
			acl_msg_error("%s: cond wait error(%s)",
				myname, acl_last_strerror(buf, sizeof(buf)));
			return (-1);
		}
	}

	return (0);
}

void *acl_aqueue_pop_timedwait(ACL_AQUEUE *queue, int tmo_sec, int tmo_usec)
{
	const char *myname = "acl_aqueue_pop_timedwait";
	ACL_AQUEUE_ITEM *qi;
	struct  timeval   tv;
	struct	timespec  timeout, *ptimeout;
	int   status;
	void *data;
	char  buf[256];

	if (queue == NULL)
		acl_msg_fatal("%s: queue null", myname);

	queue->error = ACL_AQUEUE_OK;

	status = acl_pthread_mutex_lock(&queue->lock);
	if (status) {
		__SET_ERRNO(status);
		acl_msg_error("%s: lock error(%s)",
				myname, acl_last_strerror(buf, sizeof(buf)));
		
		queue->error = ACL_AQUEUE_ERR_LOCK;
		return (NULL);
	}

	qi = NULL;

	while (1) {
		if (tmo_sec < 0 || tmo_usec < 0)
			ptimeout = NULL;
		else {
			gettimeofday(&tv, NULL);
			timeout.tv_sec = tv.tv_sec + tmo_sec;
			timeout.tv_nsec = tv.tv_usec * 1000 + tmo_usec * 1000;
			ptimeout = &timeout;
		}

		if (aqueue_wait(queue, ptimeout) < 0)
			return (NULL);

		qi = queue->first;
		if (qi != NULL) {
			queue->first = qi->next;
			queue->qlen--;
			if (queue->last == qi)
				queue->last = NULL;
			break;
		}
		if (queue->quit != 0) {
			status = acl_pthread_mutex_unlock(&queue->lock);
			if (status != 0) {
				acl_msg_error("%s(%d): unlock error(%s)",
					myname, __LINE__, acl_last_strerror(buf, sizeof(buf)));
				queue->error = ACL_AQUEUE_ERR_UNLOCK;
			}

			return (NULL);
		}
	}

	status = acl_pthread_mutex_unlock(&queue->lock);
	if (status != 0)
		acl_msg_error("%s(%d): unlock error(%s)",
			myname, __LINE__, acl_last_strerror(buf, sizeof(buf)));

	if (qi == NULL)
		acl_msg_fatal("%s(%d): qi null", myname, __LINE__);
	
	data = qi->data;
	acl_myfree(qi);

	return (data);
}

int acl_aqueue_push(ACL_AQUEUE *queue, void *data)
{
	const char *myname = "acl_aqueue_push";
	ACL_AQUEUE_ITEM *qi;
	char  buf[256];
	int   status;

	if (queue == NULL)
		acl_msg_fatal("%s: aqueue null", myname);

	qi = acl_mycalloc(1, sizeof(ACL_AQUEUE_ITEM));
	if (qi == NULL)
		acl_msg_fatal("%s: calloc error(%s)",
			myname, acl_last_strerror(buf, sizeof(buf)));

	qi->data = data;

	status = acl_pthread_mutex_lock(&queue->lock);
	if (status != 0) {
		__SET_ERRNO(status);
		acl_msg_error("%s: lock error(%s)",
				myname, acl_last_strerror(buf, sizeof(buf)));
		acl_myfree(qi);
		queue->error = ACL_AQUEUE_ERR_LOCK;
		return (-1);
	}

	if (queue->first == NULL)
		queue->first = qi;
	else
		queue->last->next = qi;

	queue->last = qi;
	queue->qlen++;

	status = acl_pthread_mutex_unlock(&queue->lock);
	if (status != 0) {
		__SET_ERRNO(status);
		acl_msg_error("%s: unlock error(%s)",
				myname, acl_last_strerror(buf, sizeof(buf)));
		return (-1);
	}

	status = acl_pthread_cond_signal(&queue->cond);
	if (status != 0) {
		__SET_ERRNO(status);
		acl_msg_error("%s: cond signal error(%s)",
				myname, acl_last_strerror(buf, sizeof(buf)));
		return (-1);
	}

	return (0);
}

void acl_aqueue_set_quit(ACL_AQUEUE *queue)
{
	if (queue)
		queue->quit = 1;
}
int acl_aqueue_last_error(const ACL_AQUEUE *queue)
{
	if (queue == NULL)
		return (ACL_AQUEUE_ERR_UNKNOWN);

	return (queue->error);
}
