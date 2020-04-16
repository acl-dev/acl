#include "lib_acl.h"
#include <semaphore.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "taskq.h"

typedef struct TASK {
	void  (*callback)(void *ctx);
	void *ctx;
} TASK;

typedef struct TASKQ {
	acl_pthread_mutex_t lock;
	TASK    *tasks;
	unsigned qsize;
	unsigned slot_empty;
	unsigned slot_full;
	unsigned nthreads;
	acl_pthread_t *threads;
#if defined(__APPLE__)
	char  *path_empty;
	char  *path_full;
#endif
	sem_t *sem_empty;
	sem_t *sem_full;
} TASKQ;

static void *taskq_pop(void *ctx);

TASKQ *taskq_create(unsigned qsize, unsigned nthreads)
{
	TASKQ *taskq = (TASKQ*) acl_mycalloc(1, sizeof(TASKQ));
	acl_pthread_attr_t attr;
#if defined(__APPLE__)
	const char *path = "/tmp";
#endif
	int    ret, i;

	ret = acl_pthread_mutex_init(&taskq->lock, NULL);
	assert(ret == 0);

	assert(qsize > 0);
	assert(nthreads > 0);

	taskq->tasks    = (TASK*) acl_mycalloc(qsize, sizeof(TASK));
	taskq->qsize    = qsize;
	taskq->nthreads = nthreads;
	taskq->threads  = (acl_pthread_t*) acl_mycalloc(nthreads, sizeof(acl_pthread_t));

#if defined(__APPLE__)
	assert(path && *path);
	taskq->path_empty = acl_concatenate(path, "_", "sem_empty", NULL);
	taskq->sem_empty = sem_open(taskq->path_empty, O_CREAT, S_IRUSR | S_IWUSR, qsize);
	assert(taskq->sem_empty != SEM_FAILED);
#else
	taskq->sem_empty = (sem_t*) acl_mycalloc(1, sizeof(sem_t));
	ret = sem_init(taskq->sem_empty, 0, qsize);
	assert(ret == 0);
#endif
#if defined(__APPLE__)
	taskq->path_full = acl_concatenate(path, "_", "sem_full", NULL);
	taskq->sem_full = sem_open(taskq->path_full, O_CREAT, S_IRUSR | S_IWUSR, 0);
	assert(taskq->sem_full != SEM_FAILED);
#else
	taskq->sem_full = (sem_t*) acl_mycalloc(1, sizeof(sem_t));
	ret = sem_init(taskq->sem_full, 0, 0);
	assert(ret == 0);
#endif

	taskq->slot_empty = 0;
	taskq->slot_full  = 0;

	ret = acl_pthread_attr_init(&attr);
	assert(ret == 0);

	for (i = 0; i < (int) nthreads; i++) {
		ret = pthread_create(&taskq->threads[i], &attr, taskq_pop, taskq);
		assert(ret == 0);
	}
	return taskq;
}

void taskq_destroy(TASKQ *taskq)
{
	size_t i;

	for (i = 0; i < taskq->nthreads; i++) {
		(void) pthread_cancel(taskq->threads[i]);
		(void) acl_pthread_join(taskq->threads[i], NULL);
	}

#if defined(__APPLE__)
	sem_close(taskq->sem_empty);
	sem_close(taskq->sem_full);
	if (sem_unlink(taskq->path_empty) == -1) {
		printf("sem_unlink %s error %s\r\n", taskq->path_empty,
			acl_last_serror());
	}
	if (sem_unlink(taskq->path_full) == -1) {
		printf("sem_unlink %s error %s\r\n", taskq->path_full,
			acl_last_serror());
	}
	acl_myfree(taskq->path_empty);
	acl_myfree(taskq->path_full);
#else
	(void) sem_destroy(taskq->sem_empty);
	(void) sem_destroy(taskq->sem_full);
	acl_myfree(taskq->sem_empty);
	acl_myfree(taskq->sem_full);
#endif
	(void) acl_pthread_mutex_destroy(&taskq->lock);
	acl_myfree(taskq->threads);
	acl_myfree(taskq->tasks);
	acl_myfree(taskq);
}

static void *taskq_pop(void *ctx)
{
	TASKQ *taskq = (TASKQ*) ctx;
	TASK   task;

	while (1) {
		int ret = sem_wait(taskq->sem_full);
		if (ret != 0) {
			if (errno == EINTR) {
				continue;
			}
			printf("sem_wait error %s\r\n", acl_last_serror());
			assert(0);
		}

		ret = acl_pthread_mutex_lock(&taskq->lock);
		assert(ret == 0);

		task = taskq->tasks[taskq->slot_full];
		memset(&taskq->tasks[taskq->slot_full], 0, sizeof(TASK));
		taskq->slot_full = (taskq->slot_full + 1) % taskq->qsize;

		ret = acl_pthread_mutex_unlock(&taskq->lock);
		assert(ret == 0);

		ret = sem_post(taskq->sem_empty);
		assert(ret == 0);

		task.callback(task.ctx);

	}
	return NULL;
}

void taskq_push(TASKQ *taskq, void (*callback)(void*), void *ctx)
{
	int ret = sem_wait(taskq->sem_empty);
	assert(ret == 0);

	ret = acl_pthread_mutex_lock(&taskq->lock);
	assert(ret == 0);

	taskq->tasks[taskq->slot_empty].callback = callback;
	taskq->tasks[taskq->slot_empty].ctx      = ctx;

	taskq->slot_empty = (taskq->slot_empty + 1) % taskq->qsize;
	ret = acl_pthread_mutex_unlock(&taskq->lock);
	assert(ret == 0);

	ret = sem_post(taskq->sem_full);
	assert(ret == 0);
}
