#include "stdafx.h"
#include <errno.h>
#if defined(_WIN32) || defined(_WIN64)
#include "pthread_patch.h"
#else
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "msg.h"
#include "pthread_patch.h"
#include "queue.h"

#ifdef __linux__

struct QUEUE_ITEM {
	struct QUEUE_ITEM *next;
	void *data;
};

struct QUEUE {
	QUEUE_ITEM *first, *last;
	int   qlen;
	int   error;
	int   quit;
	int   nlink;
	char  check_owner;
	long  owner;
	pthread_mutex_t lock;
	pthread_cond_t  cond;
};

QUEUE *queue_new(void)
{
	QUEUE *que = (QUEUE *) malloc(sizeof(QUEUE));

	pthread_mutex_init(&que->lock, NULL);
	pthread_cond_init(&que->cond, NULL);

	que->first = que->last = NULL;
	que->qlen = 0;
	que->error = QUEUE_OK;
	que->quit = 0;
	que->nlink = 0;
	que->owner = thread_self();
	que->check_owner = 0;
	
	return que;
}

void queue_check_owner(QUEUE *que, char flag)
{
	if (que) {
		que->check_owner = flag;
	}
}

void queue_set_owner(QUEUE *que, unsigned int owner)
{
	if (que) {
		que->owner = owner;
	}
}

void queue_free(QUEUE *que, QUEUE_FREE_FN free_fn)
{
	QUEUE_ITEM *qi;
	int   status;

	if (que == NULL) {
		return;
	}

	if (que->check_owner && thread_self() != que->owner) {
		msg_error("%s: cur tid(%lu) != owner(%lu)!", __FUNCTION__,
			thread_self(), que->owner);
		return;
	}

	que->quit = 1;
	status = pthread_mutex_lock(&que->lock);
	if (status != 0) {
		msg_error("%s: lock error(%s)", __FUNCTION__, last_serror());
	}

	while (1) {
		qi = que->first;
		if (qi == NULL) {
			break;
		}
		que->first = qi->next;
		if (free_fn != NULL) {
			free_fn(qi->data);
		}
		free(qi);
	}

	status = pthread_mutex_unlock(&que->lock);
	if (status != 0) {
		msg_error("%s: lock error(%s)", __FUNCTION__, last_serror());
	}

	pthread_mutex_destroy(&que->lock);
	pthread_cond_destroy(&que->cond);
	free(que);
}

static int queue_wait(QUEUE *que, const struct timespec *ptimeo)
{
	int   status;

	while (que->first == NULL && que->quit == 0) {
		if (ptimeo != NULL) {
			status = pthread_cond_timedwait(&que->cond,
					&que->lock, ptimeo);
		} else {
			status = pthread_cond_wait(&que->cond, &que->lock);
		}

		if (ptimeo && status == FIBER_ETIME) {
			status = pthread_mutex_unlock(&que->lock);
			if (status != 0) {
				msg_error("%s(%d): unlock error(%s)",
					__FUNCTION__, __LINE__, last_serror());
			}

			que->error = QUEUE_ERR_TIMEOUT;
			return -1;
		} else if (status != 0) {
			status = pthread_mutex_unlock(&que->lock);
			if (status != 0) {
				msg_error("%s(%d): unlock error(%s)",
					__FUNCTION__, __LINE__, last_serror());
			}

			que->error = QUEUE_ERR_COND_WAIT;
			msg_error("%s: cond wait error(%s)",
				__FUNCTION__, last_serror());
			return -1;
		}
	}

	return 0;
}

void *queue_pop_timedwait(QUEUE *que, int tmo_sec, int tmo_usec)
{
	QUEUE_ITEM *qi;
	struct  timeval   tv;
	struct	timespec  timeo, *ptimeo;
	int   status;
	void *data;

	if (que == NULL) {
		msg_fatal("%s: que null", __FUNCTION__);
	}

	que->error = QUEUE_OK;

	status = pthread_mutex_lock(&que->lock);
	if (status) {
		que->error = QUEUE_ERR_LOCK;
		msg_error("%s: lock error(%s)", __FUNCTION__, last_serror());
		return NULL;
	}

	qi = NULL;

	while (1) {
		if (tmo_sec < 0 || tmo_usec < 0) {
			ptimeo = NULL;
		} else {
			gettimeofday(&tv, NULL);
			timeo.tv_sec   = tv.tv_sec + tmo_sec;
			timeo.tv_nsec  = tv.tv_usec * 1000 + tmo_usec * 1000;
			timeo.tv_sec  += timeo.tv_nsec / 1000000000;
			timeo.tv_nsec %= 1000000000;
			ptimeo         = &timeo;
		}

		if (queue_wait(que, ptimeo) < 0) {
			return NULL;
		}

		qi = que->first;
		if (qi != NULL) {
			que->first = qi->next;
			que->qlen--;
			if (que->last == qi) {
				que->last = NULL;
			}
			break;
		}
		if (que->quit != 0) {
			status = pthread_mutex_unlock(&que->lock);
			if (status != 0) {
				msg_error("%s(%d): unlock error(%s)",
					__FUNCTION__, __LINE__, last_serror());
				que->error = QUEUE_ERR_UNLOCK;
			}

			return NULL;
		}
	}

	status = pthread_mutex_unlock(&que->lock);
	if (status != 0)
		msg_error("%s(%d): unlock error(%s)",
			__FUNCTION__, __LINE__, last_serror());

	if (qi == NULL) {
		msg_fatal("%s(%d): qi null", __FUNCTION__, __LINE__);
	}
	
	data = qi->data;
	free(qi);

	return data;
}

void *queue_pop(QUEUE *que)
{
	return queue_pop_timedwait(que, -1, -1);
}

int queue_push(QUEUE *que, void *data)
{
	QUEUE_ITEM *qi;
	int   status;

	if (que == NULL) {
		msg_fatal("%s: aqueue null", __FUNCTION__);
	}

	qi = calloc(1, sizeof(QUEUE_ITEM));
	qi->data = data;

	status = pthread_mutex_lock(&que->lock);
	if (status != 0) {
		msg_error("%s: lock error(%s)", __FUNCTION__, last_serror());
		free(qi);
		que->error = QUEUE_ERR_LOCK;
		return -1;
	}

	if (que->first == NULL) {
		que->first = qi;
	} else {
		que->last->next = qi;
	}

	que->last = qi;
	que->qlen++;

	status = pthread_mutex_unlock(&que->lock);
	if (status != 0) {
		msg_error("%s: unlock error(%s)", __FUNCTION__, last_serror());
		return -1;
	}

	status = pthread_cond_signal(&que->cond);
	if (status != 0) {
		msg_error("%s: cond signal error(%s)", __FUNCTION__, last_serror());
		return -1;
	}

	return 0;
}

int queue_qlen(QUEUE* que)
{
	int   status, n;

	status = pthread_mutex_lock(&que->lock);
	if (status) {
		msg_error("%s(%d): pthread_mutex_lock error(%s)",
			__FUNCTION__, __LINE__, strerror(status));
		return -1;
	}

	n = que->qlen;

	status = pthread_mutex_unlock(&que->lock);
	if (status) {
		msg_error("%s(%d): pthread_mutex_unlock error(%s)",
			__FUNCTION__, __LINE__, strerror(status));
		return -1;
	}

	return n;
}

void queue_set_quit(QUEUE *que)
{
	if (que) {
		que->quit = 1;
	}
}
int queue_last_error(const QUEUE *que)
{
	if (que == NULL) {
		return QUEUE_ERR_UNKNOWN;
	}

	return que->error;
}

#endif // __linux__
