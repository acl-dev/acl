#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#include <errno.h>
#ifdef	ACL_UNIX
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#endif

#include "stdlib/acl_sys_patch.h"
#include "stdlib/acl_msg.h"
#include "thread/acl_pthread.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_debug.h"
#include "stdlib/acl_slice.h"
#include "thread/acl_pthread_pool.h"

#endif

#define	ACL_PTHREAD_POOL_VALID		0x0decca62

struct acl_pthread_job_t {
	struct acl_pthread_job_t *next;
	void (*worker_fn)(void *arg);         /* user function              */
	void *worker_arg;
	int   fixed;
};

typedef struct thread_worker {
	struct thread_worker *next;
	struct thread_worker *prev;
	unsigned long id;
	int   quit;                           /* if thread need quit ?       */
	int   idle_timeout;                   /* thread wait timeout         */
	acl_pthread_job_t    *job_first;      /* thread's work queue first   */
	acl_pthread_job_t    *job_last;       /* thread's work queue last    */
	int   qlen;                           /* the work queue's length     */
	acl_pthread_cond_t    cond;
	acl_pthread_mutex_t  *mutex;
} thread_worker;

#undef	USE_SLOT

struct acl_pthread_pool_t {
	acl_pthread_mutex_t   worker_mutex;   /* control access to queue     */
	acl_pthread_cond_t    cond;           /* wait for worker quit        */
	acl_pthread_mutex_t   poller_mutex;   /* just for wait poller exit   */
	acl_pthread_cond_t    poller_cond;    /* just for wait poller exit   */
	acl_pthread_attr_t    attr;           /* create detached             */
	acl_pthread_job_t    *job_first;      /* work queue first            */
	acl_pthread_job_t    *job_last;       /* work queue last             */
	acl_pthread_job_t    *job_slot_first; /* work queue first            */
	acl_pthread_job_t    *job_slot_last;  /* work queue last             */
#ifdef	USE_SLOT
	acl_pthread_mutex_t   slot_mutex;
#endif
	thread_worker        *thr_first;      /* first idle thread           */
	thread_worker        *thr_idle;       /* for single operation        */
	thread_worker        *thr_iter;       /* for bat operation           */
	int   poller_running;                 /* is poller thread running ?  */
	int   qlen;                           /* the work queue's length     */
	int   job_nslot;
	int   qlen_warn;                      /* the work queue's length     */
	int   valid;                          /* valid                       */
	int   quit;                           /* worker should quit          */
	int   poller_quit;                    /* poller should quit          */
	int   parallelism;                    /* maximum threads             */
	int   count;                          /* current threads             */
	int   idle;                           /* idle threads                */
	int   idle_timeout;                   /* idle timeout second         */
	int   overload_timewait;              /* when too busy, need sleep ? */
	time_t last_warn;                     /* last warn time              */
	int  (*poller_fn)(void *arg);         /* worker poll function        */
	void *poller_arg;                     /* the arg of poller_fn        */
	int  (*worker_init_fn)(void *arg);    /* the arg is worker_init_arg  */
	void *worker_init_arg;
	void (*worker_free_fn)(void *arg);    /* the arg is worker_free_arg  */
	void *worker_free_arg;
};

#undef	SET_ERRNO
#ifdef	WIN32
# define	SET_ERRNO(_x_) (void) 0
#elif	defined(ACL_UNIX)
# define	SET_ERRNO(_x_) (acl_set_error(_x_))
#else
# error "unknown OS type"
#endif

#ifdef	WIN32
#define	sleep(_x_) do {  \
	Sleep(_x_ * 1000);  \
} while (0)
#endif

static void *poller_thread(void *arg)
{
	const char *myname = "poller_thread";
	acl_pthread_pool_t   *thr_pool = (acl_pthread_pool_t*) arg;
	const int wait_sec = 1, max_loop_persec = 81920;
	int   loop_count;
	time_t now_t, pre_loop_t;
#ifdef	ACL_UNIX
	pthread_t id = pthread_self();
#elif	defined(WIN32)
	unsigned long id = acl_pthread_self();
#else
        # error "unknown OS"
#endif

	if (thr_pool->poller_fn == NULL)
		acl_msg_fatal("%s, %s(%d): poller_fn is null!",
			__FILE__, myname, __LINE__);

	acl_debug(ACL_DEBUG_THR_POOL, 2) ("%s(%d): poller(tid=%lu) started ...",
		myname, __LINE__, (unsigned long) id);
	loop_count = 0;
	pre_loop_t = time(NULL);

	acl_assert(acl_pthread_mutex_lock(&thr_pool->poller_mutex) == 0);

	thr_pool->poller_running = 1;

	for (;;) {
		if (thr_pool->poller_quit)
			break;
		
		now_t = time(NULL);
		loop_count++;
		if (loop_count >= max_loop_persec) {
			/* avoid loop too quickly in one second */
			if (now_t - pre_loop_t <= wait_sec) {
				acl_msg_warn("%s: loop too fast, sleep %d "
					"seconds", myname, wait_sec);
				sleep(wait_sec);
				/* adjust the time of now */
				now_t = time(NULL);
			}
			/*adjust the pre_loop_t time */
			pre_loop_t = now_t;
			loop_count = 0;
		}

		if (thr_pool->poller_fn(thr_pool->poller_arg) < 0)
			break;
	}

	acl_debug(ACL_DEBUG_THR_POOL, 2) ("%s(%d): poller(%lu) thread quit ...",
		myname, __LINE__, (unsigned long) id);

	thr_pool->poller_running = 0;
		
#ifdef	WIN32 
	acl_assert(acl_pthread_cond_signal(&thr_pool->poller_cond) == 0);
#else
	acl_assert(pthread_cond_broadcast(&thr_pool->poller_cond) == 0);
#endif

	acl_debug(ACL_DEBUG_THR_POOL, 3) ("poller broadcast ok");
	acl_assert(acl_pthread_mutex_unlock(&thr_pool->poller_mutex) == 0);
	acl_debug(ACL_DEBUG_THR_POOL, 3) ("poller unlock ok");

	return NULL;
}

static thread_worker *worker_create(acl_pthread_pool_t *thr_pool)
{
	thread_worker *thr = (thread_worker*) acl_mycalloc(1,
			sizeof(thread_worker));

	thr->id = (unsigned long) acl_pthread_self();
	thr->idle_timeout = thr_pool->idle_timeout;
	acl_assert(acl_pthread_cond_init(&thr->cond, NULL) == 0);
	thr->mutex = &thr_pool->worker_mutex;
	return thr;
}

static void worker_free(thread_worker *thr)
{
	acl_pthread_cond_destroy(&thr->cond);
	acl_myfree(thr);
}

static int worker_wait(acl_pthread_pool_t *thr_pool, thread_worker *thr)
{
	const char *myname = "worker_wait";
	int    status;

	while (1) {
		/* if there are jobs in thread self' queue or in pool's queue
		 * just return 1
		 */
		if (thr->job_first != NULL || thr_pool->job_first != NULL)
			return 1;

		/* if there are no jobs and the thread pool want to quit */
		if (thr_pool->quit)
			return 0;

		/* add the idle thread to thread pool */

		if (thr_pool->thr_first == NULL) {
			thr_pool->thr_first = thr;
			thr->next = NULL;
			thr->prev = NULL;
		} else {
			thr_pool->thr_first->prev = thr;
			thr->next = thr_pool->thr_first;
			thr->prev = NULL;
			thr_pool->thr_first = thr;
		}
		thr_pool->thr_idle = thr;
		thr_pool->idle++;

		if (thr->idle_timeout > 0) {
			struct timespec  timeout;
			struct timeval   tv;

			gettimeofday(&tv, NULL);
			timeout.tv_sec = tv.tv_sec + thr->idle_timeout;
			timeout.tv_nsec = tv.tv_usec * 1000;

			status = acl_pthread_cond_timedwait(&thr->cond,
					thr->mutex, &timeout);
		} else
			status = acl_pthread_cond_wait(&thr->cond, thr->mutex);

		/* remove the thread from thread pool */

		if (thr_pool->thr_first == thr) {
			if (thr->next)
				thr->next->prev = NULL;
			thr_pool->thr_first = thr->next;
		} else {
			if (thr->next)
				thr->next->prev = thr->prev;
			thr->prev->next = thr->next;
		}

		thr_pool->thr_idle = NULL;
		thr_pool->idle--;
		thr->next = NULL;
		thr->prev = NULL;

		if (status == 0)
			continue;

		if (status == ACL_ETIMEDOUT) {
			thr->quit = 1;
			return -1;
		}

		/* xxx */
		SET_ERRNO(status);
		thr->quit = 1;
		acl_msg_error("%s(%d), %s: tid: %lu, cond timewait error: %s",
			__FILE__, __LINE__, myname, (unsigned long)
			acl_pthread_self(), acl_last_serror());
		return -1;
	}
}

#ifdef	USE_SLOT
static void worker_run(acl_pthread_pool_t *thr_pool, thread_worker *thr,
	acl_pthread_job_t *job)
#else
static void worker_run(acl_pthread_pool_t *thr_pool acl_unused,
	thread_worker *thr,
	acl_pthread_job_t *job)
#endif
{
	const char *myname = "worker_run";
	void (*worker_fn)(void*) = job->worker_fn;
	void  *worker_arg = job->worker_arg;
	int   status;

	/* shuld unlock before enter working process */

	status = acl_pthread_mutex_unlock(thr->mutex);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_fatal("%s(%d), %s: unlock error: %s, tid: %lu",
			__FILE__, __LINE__, myname, acl_last_serror(),
			(unsigned long) acl_pthread_self());
	}

#ifdef	USE_SLOT
	status = acl_pthread_mutex_trylock(&thr_pool->slot_mutex);
	if (status == 0) {
		if (job->fixed)
			;  /* do nothing */
		else if (thr_pool->job_nslot < thr_pool->qlen_warn) {
			/* must reset the job's next before
			 * add it to jobs slot
			 */
			job->next = NULL;

			if (thr_pool->job_slot_first == NULL)
				thr_pool->job_slot_first = job;
			else
				thr_pool->job_slot_last->next = job;
			thr_pool->job_slot_last = job;
			thr_pool->job_nslot++;
			job = NULL;
		}

		status = acl_pthread_mutex_unlock(&thr_pool->slot_mutex);
		if (status != 0) {
			SET_ERRNO(status);
			acl_msg_fatal("%s(%d), %s: pthread_mutex_unlock: %s",
				__FILE__, __LINE__, myname, acl_last_serror());
		}
	}

	if (job && !job->fixed)
		acl_myfree(job);
#else
	if (!job->fixed)
		acl_myfree(job);
#endif

	worker_fn(worker_arg);

	/* lock again */

	status = acl_pthread_mutex_lock(thr->mutex);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_fatal("%s(%d), %s: lock error: %s, sid: %lu",
			__FILE__, __LINE__, myname, acl_last_serror(),
			(unsigned long) acl_pthread_self());
	}
}

static void *worker_thread(void* arg)
{
	const char *myname = "worker_thread";
	acl_pthread_pool_t *thr_pool = (acl_pthread_pool_t*) arg;
	acl_pthread_job_t *job;
	thread_worker *thr = NULL;
	int   status;

#undef	RETURN
#define	RETURN(_x_) {  \
	if (thr_pool->worker_free_fn != NULL)  \
		thr_pool->worker_free_fn(thr_pool->worker_free_arg);  \
	if (thr != NULL)  \
		worker_free(thr);  \
	return (_x_);  \
}
	
	if (thr_pool->worker_init_fn != NULL) {
		if (thr_pool->worker_init_fn(thr_pool->worker_init_arg) < 0) {
			acl_msg_error("%s(%d), %s: tid: %lu, thread init error",
				__FILE__, __LINE__, myname,
				(unsigned long) acl_pthread_self());
			acl_pthread_mutex_lock(&thr_pool->worker_mutex);
			thr_pool->count--;
			acl_pthread_mutex_unlock(&thr_pool->worker_mutex);
			return NULL;
		}
	}

	thr = worker_create(thr_pool);
	acl_assert(thr->mutex == &thr_pool->worker_mutex);

	/* lock the thread pool's global mutex at first */

	status = acl_pthread_mutex_lock(thr->mutex);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_fatal("%s(%d), %s: lock failed: %s", __FILE__,
			__LINE__, myname, acl_last_serror());
	}

	for (;;) {
		/* handle thread self's job first */
		if (thr->job_first != NULL) {
			job = thr->job_first;
			thr->job_first = job->next;
			if (thr->job_last == job)
				thr->job_last = NULL;
			thr->qlen--;

			worker_run(thr_pool, thr, job);
		}

		/* then handle thread pool's job */
		if (thr_pool->job_first != NULL) {
			job = thr_pool->job_first;
			thr_pool->job_first = job->next;
			if (thr_pool->job_last == job)
				thr_pool->job_last = NULL;
			thr_pool->qlen--;

			worker_run(thr_pool, thr, job);
		}

		 /* at last, idle thread wait for job and unlock mutex,
		  * lock again if it wakeup when signaled by main thread
		  * or wait for job timeout; return below:
		  * 1 : got one job to handle
		  * 0 : have no job and thread pool want to quit
		  * -1: have no job and thread wait timeout or error happened
		  */
		if (worker_wait(thr_pool, thr) > 0)
			continue;

		/* when thread pool need to quit, idle thread should exit */
		if (thr_pool->quit) {
			thr_pool->count--;
			if (thr_pool->count == 0)
				acl_pthread_cond_signal(&thr_pool->cond);
			break;
		}

		/* when wait timeout or error happened, should exit now */
		if (thr->quit) {
			thr_pool->count--;
			break;
		}
	}

	status = acl_pthread_mutex_unlock(thr->mutex);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_error("%s, %s(%d): unlock error(%s)",
			__FILE__, myname, __LINE__, acl_last_serror());
	}

	acl_debug(ACL_DEBUG_THR_POOL, 2) ("%s(%d): thread(%lu) exit now",
		myname, __LINE__, (unsigned long) acl_pthread_self());

	RETURN (NULL);
}

static void job_add(acl_pthread_pool_t *thr_pool, acl_pthread_job_t *job)
{
	const char *myname = "job_add";
	int   status;

	/* must reset the job's next to NULL */
	job->next = NULL;

	/* at first, select one idle thread which qlen is 0 */

	if (thr_pool->thr_idle != NULL && thr_pool->thr_idle->qlen == 0) {
		thr_pool->thr_idle->job_first = job;
		thr_pool->thr_idle->job_last = job;
		thr_pool->thr_idle->qlen++;

		status = acl_pthread_cond_signal(&thr_pool->thr_idle->cond);
		if (status == 0)
			return;

		SET_ERRNO(status);
		acl_msg_fatal("%s(%d),> %s: pthread_cond_signal: %s",
			__FILE__, __LINE__, myname, acl_last_serror());

		/* not reached */
	}

	/* then, add job to the pool's queue and anyone can handle it */

	if (thr_pool->job_first == NULL)
		thr_pool->job_first = job;
	else
		thr_pool->job_last->next = job;
	thr_pool->job_last = job;
	thr_pool->qlen++;

	/* if not reach the max threads limit, create one thread */

	if (thr_pool->count < thr_pool->parallelism) {
		acl_pthread_t id;

		status = acl_pthread_create(&id, &thr_pool->attr,
				worker_thread, (void*) thr_pool);
		if (status == 0) {
			thr_pool->count++;
			return;
		}

		SET_ERRNO(status);
		acl_msg_error("%s(%d), %s: pthread_create: %s",
			__FILE__, __LINE__, myname, acl_last_serror());
		return;
	}

	/* if qlen is too long, should warning, event sleep a while */

	if (thr_pool->qlen > thr_pool->qlen_warn) {
		time_t now = time(NULL);

		if (now - thr_pool->last_warn >= 2) {
			thr_pool->last_warn = now;
			acl_msg_warn("%s(%d), %s: OVERLOADED! max_thread: %d,"
				" qlen: %d, idle: %d", __FILE__, __LINE__,
				myname, thr_pool->parallelism, thr_pool->qlen,
				thr_pool->idle);
		}
		if (thr_pool->overload_timewait > 0) {
			acl_msg_warn("%s(%d), %s: sleep %d seconds", __FILE__,
				__LINE__, myname, thr_pool->overload_timewait);
			sleep(thr_pool->overload_timewait);
		}
	}
}

void acl_pthread_pool_add_one(acl_pthread_pool_t *thr_pool,
	void (*run_fn)(void *), void *run_arg)
{
	const char *myname = "acl_pthread_pool_add";
	acl_pthread_job_t *job;
	int   status;

	if (thr_pool->valid != ACL_PTHREAD_POOL_VALID)
		acl_msg_fatal("%s(%d), %s: thr_pool invalid",
			__FILE__, __LINE__, myname);
	if (run_fn == NULL)
		acl_msg_fatal("%s(%d), %s: run_fn null",
			__FILE__, __LINE__, myname);

#ifdef	USE_SLOT
	status = acl_pthread_mutex_trylock(&thr_pool->slot_mutex);
	if (status == 0) {
		if (thr_pool->job_slot_first != NULL) {
			job = thr_pool->job_slot_first;
			thr_pool->job_slot_first = job->next;
			if (thr_pool->job_slot_last == job)
				thr_pool->job_slot_last = NULL;
			thr_pool->job_nslot--;

			job->worker_fn  = run_fn;
			job->worker_arg = run_arg;
			job->fixed      = 0;
		} else
			job = NULL;

		status = acl_pthread_mutex_unlock(&thr_pool->slot_mutex);
		if (status != 0) {
			SET_ERRNO(status);
			acl_msg_fatal("%s(%d), %s: pthread_mutex_unlock: %s",
				__FILE__, __LINE__, myname, acl_last_serror());
		}

		if (job == NULL)
			job = acl_pthread_pool_alloc_job(run_fn, run_arg, 0);
	} else
#endif
		job = acl_pthread_pool_alloc_job(run_fn, run_arg, 0);

	status = acl_pthread_mutex_lock(&thr_pool->worker_mutex);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_fatal("%s(%d), %s: pthread_mutex_lock: %s",
			__FILE__, __LINE__, myname, acl_last_serror());
	}

	job_add(thr_pool, job);

	status = acl_pthread_mutex_unlock(&thr_pool->worker_mutex);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_fatal("%s(%d), %s: pthread_mutex_unlock: %s",
			__FILE__, __LINE__, myname, acl_last_serror());
	}
}

void acl_pthread_pool_add_job(acl_pthread_pool_t *thr_pool,
	acl_pthread_job_t *job)
{
	const char *myname = "acl_pthread_pool_add_job";
	int   status;

	if (thr_pool->valid != ACL_PTHREAD_POOL_VALID)
		acl_msg_fatal("%s(%d), %s: thr_pool invalid",
			__FILE__, __LINE__, myname);
	if (job == NULL)
		acl_msg_fatal("%s(%d), %s: job null",
			__FILE__, __LINE__, myname);

	status = acl_pthread_mutex_lock(&thr_pool->worker_mutex);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_fatal("%s(%d), %s: pthread_mutex_lock: %s",
			__FILE__, __LINE__, myname, acl_last_serror());
	}

	job_add(thr_pool, job);

	status = acl_pthread_mutex_unlock(&thr_pool->worker_mutex);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_fatal("%s(%d), %s: pthread_mutex_unlock: %s",
			__FILE__, __LINE__, myname, acl_last_serror());
	}
}

void acl_pthread_pool_bat_add_begin(acl_pthread_pool_t *thr_pool)
{
	const char *myname = "acl_pthread_pool_bat_add_begin";
	int   status;

	if (thr_pool->valid != ACL_PTHREAD_POOL_VALID)
		acl_msg_fatal("%s(%d), %s: invalid thr_pool->valid",
			__FILE__, __LINE__, myname);

	thr_pool->thr_iter = thr_pool->thr_first;

	status = acl_pthread_mutex_lock(&thr_pool->worker_mutex);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_fatal("%s(%d), %s: pthread_mutex_lock, serr = %s",
			__FILE__, __LINE__, myname, acl_last_serror());
	}
}

static void job_append(acl_pthread_pool_t *thr_pool, acl_pthread_job_t *job)
{
	const char *myname = "job_append";
	int   status;

	/* must reset the job's next to NULL */
	job->next = NULL;

	if (thr_pool->thr_iter != NULL) {

		/* if the idle thread has no job append, just it */

		if (thr_pool->thr_iter->qlen == 0) {
			thr_pool->thr_iter->job_first = job;
			thr_pool->thr_iter->job_last = job;
			thr_pool->thr_iter->qlen++;
			thr_pool->thr_iter = thr_pool->thr_iter->next;
			return;
		}

		/* iterator the left idle threads */

		for (thr_pool->thr_iter = thr_pool->thr_iter->next;
			thr_pool->thr_iter != NULL;
			thr_pool->thr_iter = thr_pool->thr_iter->next)
		{
			/* skip busy thread */
			if (thr_pool->thr_iter->qlen > 0)
				continue;

			thr_pool->thr_iter->job_first = job;
			thr_pool->thr_iter->job_last = job;
			thr_pool->thr_iter->qlen++;
			thr_pool->thr_iter = thr_pool->thr_iter->next;
			return;
		}
	}

	/* add the job to the thread pool's queue, anyone can handle it */

	if (thr_pool->job_first == NULL)
		thr_pool->job_first = job;
	else
		thr_pool->job_last->next = job;
	thr_pool->job_last = job;
	thr_pool->qlen++;

	/* if not reach the max threads limit, create one thread */

	if (thr_pool->count < thr_pool->parallelism) {
		acl_pthread_t id;

		status = acl_pthread_create(&id, &thr_pool->attr,
				worker_thread, (void*) thr_pool);
		if (status == 0) {
			thr_pool->count++;
			return;
		}

		SET_ERRNO(status);
		acl_msg_error("%s(%d), %s: pthread_create: %s",
			__FILE__, __LINE__, myname, acl_last_serror());
		return;
	}

	/* if there are too many jobs in thread pool's queue, do warning */

	if (thr_pool->qlen > thr_pool->qlen_warn) {
		time_t now = time(NULL);

		if (now - thr_pool->last_warn >= 2) {
			thr_pool->last_warn = now;
			acl_msg_warn("%s(%d), %s: OVERLOADED! max_thread: %d"
				", qlen: %d, idle: %d", __FILE__, __LINE__,
				myname, thr_pool->parallelism, thr_pool->qlen,
				thr_pool->idle);
		}
		if (thr_pool->overload_timewait > 0) {
			acl_msg_warn("%s(%d), %s: sleep %d seconds", __FILE__,
			       	__LINE__, myname, thr_pool->overload_timewait);
			sleep(thr_pool->overload_timewait);
		}
	}
}

void acl_pthread_pool_bat_add_one(acl_pthread_pool_t *thr_pool,
	void (*run_fn)(void *), void *run_arg)
{
	const char *myname = "acl_pthread_pool_bat_add_one";
	acl_pthread_job_t *job;
#ifdef	USE_SLOT
	int   status;
#endif

	if (thr_pool->valid != ACL_PTHREAD_POOL_VALID || run_fn == NULL)
		acl_msg_fatal("%s(%d), %s: invalid thr_pool or run_fn",
			__FILE__, __LINE__, myname);

#ifdef	USE_SLOT
	status = acl_pthread_mutex_trylock(&thr_pool->slot_mutex);
	if (status == 0) {
		/* if there are some slot of job, reuse it */
		if (thr_pool->job_slot_first != NULL) {
			job = thr_pool->job_slot_first;
			thr_pool->job_slot_first = job->next;
			if (thr_pool->job_slot_last == job)
				thr_pool->job_slot_last = NULL;
			thr_pool->job_nslot--;

			job->worker_fn  = run_fn;
			job->worker_arg = run_arg;
			job->fixed      = 0;
		} else
			job = NULL;

		status = acl_pthread_mutex_unlock(&thr_pool->slot_mutex);
		if (status != 0) {
			SET_ERRNO(status);
			acl_msg_fatal("%s(%d), %s: pthread_mutex_unlock: %s",
				__FILE__, __LINE__, myname, acl_last_serror());
		}

		if (job == NULL)
			job = acl_pthread_pool_alloc_job(run_fn, run_arg, 0);
	} else
#endif
		job = acl_pthread_pool_alloc_job(run_fn, run_arg, 0);

	job_append(thr_pool, job);
}

void acl_pthread_pool_bat_add_job(acl_pthread_pool_t *thr_pool,
	acl_pthread_job_t *job)
{
	const char *myname = "acl_pthread_pool_bat_add_job";

	if (thr_pool->valid != ACL_PTHREAD_POOL_VALID)
		acl_msg_fatal("%s(%d), %s: invalid thr_pool->valid",
			__FILE__, __LINE__, myname);

	job_append(thr_pool, job);
}

void acl_pthread_pool_bat_add_end(acl_pthread_pool_t *thr_pool)
{
	const char *myname = "acl_pthread_pool_bat_add_end";
	int   status, qlen;
	thread_worker *thr_iter;

	if (thr_pool->valid != ACL_PTHREAD_POOL_VALID)
		acl_msg_fatal("%s(%d), %s: invalid thr_pool->valid",
			__FILE__, __LINE__, myname);

	qlen = thr_pool->qlen;
	thr_iter = thr_pool->thr_first;

	/* iterator all the idle threads, signal one if it has job */

	for (; thr_iter != NULL ; thr_iter = thr_iter->next) {

		/* handle thread self's job first */

		if (thr_iter->qlen > 0) {
			status = acl_pthread_cond_signal(&thr_iter->cond);
			if (status == 0)
				continue;

			SET_ERRNO(status);
			acl_msg_fatal("%s(%d), %s: pthread_cond_signal: %s",
				__FILE__, __LINE__, myname, acl_last_serror());
		}

		/* if thread pool's job not empty , let idle thread to handle */

		else if (qlen > 0) {
			status = acl_pthread_cond_signal(&thr_iter->cond);
			if (status == 0) {
				qlen--;
				continue;
			}

			SET_ERRNO(status);
			acl_msg_fatal("%s(%d), %s: pthread_cond_signal: %s",
				__FILE__, __LINE__, myname, acl_last_serror());
		}
	}

	status = acl_pthread_mutex_unlock(&thr_pool->worker_mutex);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_fatal("%s(%d), %s: pthread_mutex_unlock: %s",
			__FILE__, __LINE__, myname, acl_last_serror());
	}

	thr_pool->thr_iter = NULL;
}

static void init_thread_pool(acl_pthread_pool_t *thr_pool)
{
	thr_pool->quit              = 0;
	thr_pool->poller_quit       = 0;
	thr_pool->poller_running    = 0;
	thr_pool->job_first         = NULL;
	thr_pool->job_last          = NULL;
	thr_pool->job_slot_first    = NULL;
	thr_pool->job_slot_last     = NULL;
	thr_pool->job_nslot         = 0;
	thr_pool->thr_first         = NULL;
	thr_pool->thr_idle          = NULL;
	thr_pool->thr_iter          = NULL;
	thr_pool->qlen              = 0;
	thr_pool->overload_timewait = 0;
	thr_pool->count             = 0;
	thr_pool->idle              = 0;
}

/* create work queue */

acl_pthread_pool_t *acl_thread_pool_create(int threads_limit, int idle_timeout)
{
	acl_pthread_pool_t *thr_pool;
	acl_pthread_pool_attr_t attr;

	acl_pthread_pool_attr_init(&attr);
	acl_pthread_pool_attr_set_threads_limit(&attr, threads_limit);
	acl_pthread_pool_attr_set_idle_timeout(&attr, idle_timeout);

	thr_pool = acl_pthread_pool_create(&attr);
	return thr_pool;
}

acl_pthread_pool_t *acl_pthread_pool_create(const acl_pthread_pool_attr_t *attr)
{
	const char *myname = "acl_pthread_pool_create";
	int   status;
	acl_pthread_pool_t *thr_pool;

	thr_pool = acl_mycalloc(1, sizeof(*thr_pool));
	status = acl_pthread_attr_init(&thr_pool->attr);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_fatal("%s(%d), %s: pthread_attr_init, serr: %s",
			__FILE__, __LINE__, myname, acl_last_serror());
	}

	if (attr && attr->stack_size > 0)
		acl_pthread_attr_setstacksize(&thr_pool->attr, attr->stack_size);

#ifdef	ACL_UNIX
	status = pthread_attr_setdetachstate(&thr_pool->attr,
			PTHREAD_CREATE_DETACHED);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_fatal("%s(%d), %s: pthread_attr_setdetachstate err: %s",
			__FILE__, __LINE__, myname, acl_last_serror());
	}
# if     !defined(__FreeBSD__)
	status = pthread_attr_setscope(&thr_pool->attr, PTHREAD_SCOPE_SYSTEM);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_fatal("%s(%d), %s: pthread_attr_setscope err: %s",
			__FILE__, __LINE__, myname, acl_last_serror());
	}
# endif
#elif defined(WIN32)
	(void) acl_pthread_attr_setdetachstate(&thr_pool->attr, 1);
#endif
	status = acl_pthread_mutex_init(&thr_pool->worker_mutex, NULL);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_fatal("%s(%d), %s: pthread_mutex_init err: %s",
			__FILE__, __LINE__, myname, acl_last_serror());
	}
	status = acl_pthread_cond_init(&thr_pool->cond, NULL);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_fatal("%s(%d), %s: pthread_cond_init err: %s",
			__FILE__, __LINE__, myname, acl_last_serror());
	}

#ifdef	USE_SLOT
	status = acl_pthread_mutex_init(&thr_pool->slot_mutex, NULL);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_fatal("%s(%d), %s: pthread_mutex_init err: %s",
			__FILE__, __LINE__, myname, acl_last_serror());
	}
#endif

	status = acl_pthread_mutex_init(&thr_pool->poller_mutex, NULL);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_fatal("%s(%d), %s: pthread_mutex_init err: %s",
			__FILE__, __LINE__, myname, acl_last_serror());
	}
	status = acl_pthread_cond_init(&thr_pool->poller_cond, NULL);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_fatal("%s(%d), %s: pthread_cond_init err: %s",
			__FILE__, __LINE__, myname, acl_last_serror());
	}

	init_thread_pool(thr_pool);

	thr_pool->parallelism = (attr && attr->threads_limit > 0) ?
		attr->threads_limit : ACL_PTHREAD_POOL_DEF_THREADS;
	thr_pool->qlen_warn = thr_pool->parallelism * 10;
	thr_pool->idle_timeout = (attr && attr->idle_timeout > 0) ?
		attr->idle_timeout : ACL_PTHREAD_POOL_DEF_IDLE;
	thr_pool->poller_fn = NULL;
	thr_pool->poller_arg = NULL;
	
	thr_pool->worker_init_fn = NULL;
	thr_pool->worker_init_arg = NULL;
	thr_pool->worker_free_fn = NULL;
	thr_pool->worker_free_arg = NULL;

	thr_pool->valid = ACL_PTHREAD_POOL_VALID;

	return thr_pool;
}

int acl_pthread_pool_set_timewait(acl_pthread_pool_t *thr_pool, int timewait)
{
	const char *myname = "acl_pthread_pool_set_timewait";

	if (thr_pool == NULL || thr_pool->valid != ACL_PTHREAD_POOL_VALID
		|| timewait < 0)
	{
		acl_msg_error("%s(%d), %s: invalid input",
			__FILE__, __LINE__, myname);
		return -1;
	}

	thr_pool->overload_timewait = timewait;
	return 0;
}

int acl_pthread_pool_atinit(acl_pthread_pool_t *thr_pool,
	int (*init_fn)(void *), void *init_arg)
{
	const char *myname = "acl_pthread_pool_atinit";

	if (thr_pool == NULL || thr_pool->valid != ACL_PTHREAD_POOL_VALID) {
		acl_msg_error("%s(%d), %s: input invalid",
			__FILE__, __LINE__, myname);
		return ACL_EINVAL;
	}

	thr_pool->worker_init_fn = init_fn;
	thr_pool->worker_init_arg = init_arg;

	return 0;
}

int acl_pthread_pool_atfree(acl_pthread_pool_t *thr_pool,
	void (*free_fn)(void *), void *free_arg)
{
	const char *myname = "acl_pthread_pool_atfree";

	if (thr_pool == NULL || thr_pool->valid != ACL_PTHREAD_POOL_VALID) {
		acl_msg_error("%s(%d), %s: input invalid",
			__FILE__, __LINE__, myname);
		return ACL_EINVAL;
	}

	thr_pool->worker_free_fn = free_fn;
	thr_pool->worker_free_arg = free_arg;

	return 0;
}

static int wait_poller_exit(acl_pthread_pool_t *thr_pool)
{
	const char *myname = "wait_poller_exit";
	int   status, nwait = 0;
	struct  timeval   tv;
	struct	timespec timeout;

	acl_debug(ACL_DEBUG_THR_POOL, 3) ("%s: begin to lock", myname);

	thr_pool->poller_quit = 1;

	status = acl_pthread_mutex_lock(&thr_pool->poller_mutex);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_error("%s, %s(%d): pthread_mutex_lock, serr = %s",
			__FILE__, myname, __LINE__, acl_last_serror());
		return status;
	}

	acl_debug(ACL_DEBUG_THR_POOL, 3) ("%s: begin to wait cond", myname);

	while (thr_pool->poller_running != 0) {
		gettimeofday(&tv, NULL);
		timeout.tv_sec = tv.tv_sec + 1;
		timeout.tv_nsec = tv.tv_usec * 1000;

		nwait++;

		status = acl_pthread_cond_timedwait(&thr_pool->poller_cond,
				&thr_pool->poller_mutex, &timeout);
		if (status == ACL_ETIMEDOUT) {
			acl_debug(ACL_DEBUG_THR_POOL, 3)
				("%s: nwait=%d", myname, nwait);
		} else if (status != 0) {
			SET_ERRNO(status);
			acl_pthread_mutex_unlock(&thr_pool->poller_mutex);
			acl_msg_error("%s, %s(%d): pthread_cond_wait: %s",
				__FILE__, myname, __LINE__, acl_last_serror());
			return status;
		}
	}

	acl_debug(ACL_DEBUG_THR_POOL, 3) ("%s: begin to unlock", myname);

	status = acl_pthread_mutex_unlock(&thr_pool->poller_mutex);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_error("%s, %s(%d): pthread_mutex_unlock error(%s)",
			__FILE__, myname, __LINE__, acl_last_serror());
	}

	return status;
}

static int wait_worker_exit(acl_pthread_pool_t *thr_pool)
{
	const char *myname = "wait_worker_exit";
	int   status, nwait = 0;

	status = acl_pthread_mutex_lock(&thr_pool->worker_mutex);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_error("%s(%d), %s: pthread_mutex_lock: %s",
			__FILE__, __LINE__, myname, acl_last_serror());
		return status;
	}

	thr_pool->quit = 1;

	if (thr_pool->count < 0) {
		acl_msg_error("%s(%d), %s: count: %d",
			__FILE__, __LINE__, myname, thr_pool->count);
		(void) acl_pthread_mutex_unlock(&thr_pool->worker_mutex);
		return -1;
	} else if (thr_pool->count == 0) {
		acl_debug(ACL_DEBUG_THR_POOL, 2) ("%s: count: 0", myname);
		(void) acl_pthread_mutex_unlock(&thr_pool->worker_mutex);
		return 0;
	}

	/* 1. set quit flag
	 * 2. broadcast to wakeup any sleeping
	 * 4. wait till all quit
	 */
	/* then: thr_pool->count > 0 */
	
	if (thr_pool->thr_first != NULL) {
		thread_worker *thr;

		acl_debug(ACL_DEBUG_THR_POOL, 2) ("%s: idle: %d, notify thread",
			myname, thr_pool->idle);

		for (thr = thr_pool->thr_first; thr != NULL; thr = thr->next)
			acl_pthread_cond_signal(&thr->cond);
	}

	while (thr_pool->count > 0) {
		nwait++;

		acl_debug(ACL_DEBUG_THR_POOL, 2)
			("debug(2): count = %d, nwait=%d, idle=%d",
			thr_pool->count, nwait, thr_pool->idle);

		/* status = pthread_cond_timedwait(&thr_pool->cond,
		 * 		&thr_pool->worker_mutex, &timeout);
		 */
		status = acl_pthread_cond_wait(&thr_pool->cond,
				&thr_pool->worker_mutex);
		if (status == ACL_ETIMEDOUT) {
			acl_debug(ACL_DEBUG_THR_POOL, 2)
				("%s: timeout nwait=%d", myname, nwait);
		} else if (status != 0) {
			SET_ERRNO(status);
			acl_pthread_mutex_unlock(&thr_pool->worker_mutex);
			acl_msg_error("%s(%d), %s: pthread_cond_timedwait"
				" err: %s", __FILE__, __LINE__, myname,
				acl_last_serror());
			return status;
		}
	}

	status = acl_pthread_mutex_unlock(&thr_pool->worker_mutex);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_error("%s(%d), %s: pthread_mutex_unlock err: %s",
			__FILE__, __LINE__, myname, acl_last_serror());
	}

	return status;
}

int acl_pthread_pool_destroy(acl_pthread_pool_t *thr_pool)
{
	const char *myname = "acl_pthread_pool_destroy";
	int   status, s1, s2, s3, s4, s5;
#ifdef	USE_SLOT
	int   s6;
#endif
	acl_pthread_job_t *job;

	if (thr_pool == NULL || thr_pool->valid != ACL_PTHREAD_POOL_VALID) {
		acl_msg_error("%s(%d), %s: input invalid",
			__FILE__, __LINE__, myname);
		return ACL_EINVAL;
	}

	thr_pool->valid = 0;  /* prevent any other operations */

	status = wait_poller_exit(thr_pool);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_error("%s, %s(%d): wait_poller_exit error(%s), ret: %d",
			__FILE__, myname, __LINE__, acl_last_serror(), status);
		return status;
	}

	acl_debug(ACL_DEBUG_THR_POOL, 2)
		("%s(%d): poller thread exits ok, worker count: %d",
		 myname, __LINE__, thr_pool->count);

	status = wait_worker_exit(thr_pool);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_error("%s, %s(%d): wait_worker_exit error(%s), ret: %d",
			__FILE__, myname, __LINE__, acl_last_serror(), status);
		return status;
	}

	for (job = thr_pool->job_slot_first; job != NULL;) {
		acl_pthread_job_t *tmp = job;
		job = job->next;
		acl_pthread_pool_free_job(tmp);
	}
	thr_pool->job_nslot = 0;

	acl_debug(ACL_DEBUG_THR_POOL, 2)
		("%s(%d): worker threads exit ok, conter: %d",
		 myname, __LINE__, thr_pool->count);

	sleep(1);
	s1 = acl_pthread_mutex_destroy(&thr_pool->poller_mutex);
	s2 = acl_pthread_cond_destroy(&thr_pool->poller_cond);

	s3 = acl_pthread_mutex_destroy(&thr_pool->worker_mutex);
	s4 = acl_pthread_cond_destroy(&thr_pool->cond);
	s5 = acl_pthread_attr_destroy(&thr_pool->attr);
#ifdef	USE_SLOT
	s6 = acl_pthread_mutex_destroy(&thr_pool->slot_mutex);
#endif

	acl_myfree(thr_pool);

#ifdef	USE_SLOT
	status = s1 ? s1 : (s2 ? s2 : (s3 ? s3 : (s4 ? s4 : (s5 ? s5 : s6))));
#else
	status = s1 ? s1 : (s2 ? s2 : (s3 ? s3 : (s4 ? s4 : s5)));
#endif

	return status;
}

int acl_pthread_pool_stop(acl_pthread_pool_t *thr_pool)
{
	const char *myname = "acl_pthread_pool_stop";
	int   status;

	if (thr_pool == NULL || thr_pool->valid != ACL_PTHREAD_POOL_VALID) {
		acl_msg_error("%s(%d), %s: input invalid",
			__FILE__, __LINE__, myname);
		return ACL_EINVAL;
	}

	thr_pool->valid = 0;  /* prevent any other operations */

	status = wait_poller_exit(thr_pool);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_error("%s, %s(%d): wait_poller_exit error(%s), ret: %d",
			__FILE__, myname, __LINE__, acl_last_serror(), status);
		return status;
	}

	acl_debug(ACL_DEBUG_THR_POOL, 2)
		("%s(%d): poller thread exits ok, worker count: %d",
		 myname, __LINE__, thr_pool->count);

	status = wait_worker_exit(thr_pool);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_error("%s, %s(%d): wait_worker_exit error(%s), ret: %d",
			__FILE__, myname, __LINE__, acl_last_serror(), status);
		return status;
	}

	/* restore the valid status */
	thr_pool->valid = ACL_PTHREAD_POOL_VALID;

	acl_debug(ACL_DEBUG_THR_POOL, 2)
		("%s(%d): worker threads exit ok, conter: %d",
		 myname, __LINE__, thr_pool->count);

	return 0;
}

void acl_pthread_pool_set_poller(acl_pthread_pool_t *thr_pool,
	int (*poller_fn)(void *), void *poller_arg)
{
	const char *myname = "acl_pthread_pool_set_poller";

	if (thr_pool->valid != ACL_PTHREAD_POOL_VALID || poller_fn == NULL) {
		acl_msg_error("%s(%d), %s: input invalid",
			__FILE__, __LINE__, myname);
		return;
	}

	thr_pool->poller_fn = poller_fn;
	thr_pool->poller_arg = poller_arg;
}

int acl_pthread_pool_start_poller(acl_pthread_pool_t *thr_pool)
{
	const char *myname = "acl_pthread_pool_start_poller";
	acl_pthread_t id;
	int   status;

	if (thr_pool->valid != ACL_PTHREAD_POOL_VALID) {
		acl_msg_error("%s(%d), %s: input invalid",
			__FILE__, __LINE__, myname);
		return -1;
	}

	if (thr_pool->poller_fn == NULL) {
		acl_msg_warn("%s, %s(%d): poller_fn null, needn't call %s",
			__FILE__, myname, __LINE__, myname);
		return -1;
	}

	status = acl_pthread_mutex_lock(&thr_pool->poller_mutex);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_error("%s, %s(%d): lock poller_mutex error(%s)",
			__FILE__, myname, __LINE__, acl_last_serror());
		return -1;
	}

	if (thr_pool->poller_running) {
		acl_msg_error("%s, %s(%d): server is running",
			__FILE__, myname, __LINE__);
		return -1;
	}

	status = acl_pthread_mutex_unlock(&thr_pool->poller_mutex);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_error("%s, %s(%d): unlock poller_mutex error(%s)",
			__FILE__, myname, __LINE__, acl_last_serror());
		return -1;
	}

	init_thread_pool(thr_pool);

	status = acl_pthread_create(&id, &thr_pool->attr,
				poller_thread, (void*) thr_pool);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_error("%s(%d), %s: pthread_create poller: %s",
			__FILE__, __LINE__, myname, acl_last_serror());
		return status;
	}

	return 0;
}

int acl_pthread_pool_add_dispatch(void *dispatch_arg,
	void (*run_fn)(void *), void *run_arg)
{
	const char *myname = "acl_pthread_pool_add_dispatch";
	acl_pthread_pool_t *thr_pool;

	if (dispatch_arg == NULL || run_fn == NULL)
		acl_msg_fatal("%s(%d), %s: invalid input",
			__FILE__, __LINE__, myname);

	thr_pool = (acl_pthread_pool_t *) dispatch_arg;
	acl_pthread_pool_bat_add_one(thr_pool, run_fn, run_arg);

	return 0;
}

int acl_pthread_pool_dispatch(void *dispatch_arg,
	void (*run_fn)(void *), void *run_arg)
{
	const char *myname = "acl_pthread_pool_dispatch";
	acl_pthread_pool_t *thr_pool;

	if (dispatch_arg == NULL || run_fn == NULL)
		acl_msg_fatal("%s(%d), %s: invalid input",
			__FILE__, __LINE__, myname);

	thr_pool = (acl_pthread_pool_t *) dispatch_arg;

	acl_pthread_pool_add(thr_pool, run_fn, run_arg);
	return 0;
}

int acl_pthread_pool_size(acl_pthread_pool_t *thr_pool)
{
	const char *myname = "acl_pthread_pool_size";
	int   status, n;

	status = acl_pthread_mutex_lock(&thr_pool->worker_mutex);
	if (status) {
		acl_msg_error("%s(%d), %s: pthread_mutex_lock error(%s)",
			__FILE__, __LINE__, myname, strerror(status));
		return -1;
	}

	n = thr_pool->count;
	status = acl_pthread_mutex_unlock(&thr_pool->worker_mutex);
	if (status) {
		acl_msg_error("%s(%d), %s: pthread_mutex_unlock error(%s)",
			__FILE__, __LINE__, myname, strerror(status));
		return -1;
	}

	return n;
}

int acl_pthread_pool_qlen(acl_pthread_pool_t *thr_pool)
{
	return thr_pool->qlen;
}

void acl_pthread_pool_set_stacksize(acl_pthread_pool_t *thr_pool, size_t size)
{
	if (thr_pool && size > 0)
		acl_pthread_attr_setstacksize(&thr_pool->attr, size);
}

void acl_pthread_pool_attr_init(acl_pthread_pool_attr_t *attr)
{
	if (attr)
		memset(attr, 0, sizeof(acl_pthread_pool_attr_t));
}

void acl_pthread_pool_attr_set_stacksize(
	acl_pthread_pool_attr_t *attr, size_t size)
{
	if (attr && size > 0)
		attr->stack_size = size;
}

void acl_pthread_pool_attr_set_threads_limit(
	acl_pthread_pool_attr_t *attr, int threads_limit)
{
	if (attr && threads_limit > 0)
		attr->threads_limit = threads_limit;
}

void acl_pthread_pool_attr_set_idle_timeout(
	acl_pthread_pool_attr_t *attr, int idle_timeout)
{
	if (attr && idle_timeout > 0)
		attr->idle_timeout = idle_timeout;
}

acl_pthread_job_t *acl_pthread_pool_alloc_job(void (*run_fn)(void*),
	void *run_arg, int fixed)
{
	acl_pthread_job_t *job = (acl_pthread_job_t*)
		acl_mymalloc(sizeof(acl_pthread_job_t));

	job->worker_fn  = run_fn;
	job->worker_arg = run_arg;
	job->next       = NULL;
	job->fixed      = fixed;
	return job;
}

void acl_pthread_pool_free_job(acl_pthread_job_t *job)
{
	acl_myfree(job);
}
