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
#define	SEC_TO_NS			1000000000
#define	SEC_TO_MS			1000
#define	MS_TO_NS			1000000

#define SET_TIME(x) do { \
	struct timeval t; \
	gettimeofday(&t, NULL); \
	(x) = ((acl_int64) t.tv_sec) * 1000 + ((acl_int64) t.tv_usec / 1000); \
} while (0)

struct acl_pthread_job_t {
	struct acl_pthread_job_t *next;
	void (*worker_fn)(void *arg);         /* user function              */
	void *worker_arg;                     /* user function's arg        */
	int   fixed;                          /* if can be freed ?          */
	acl_int64  start;                     /* start time stamp           */
};

typedef struct thread_cond {
	struct thread_cond *next;
	acl_pthread_cond_t  cond;
} thread_cond;

typedef struct thread_worker {
	struct thread_worker *next;
	struct thread_worker *prev;
	unsigned long id;
	int   quit;                           /* if thread need quit ?      */
	int   idle;                           /* thread wait timeout        */
	acl_int64 wait_base;                  /* once wait: nanosecond      */
	acl_int64 wait_sec;                   /* once wait: second          */
	acl_int64 wait_nsec;                  /* once wait: nanosecond      */
	acl_int64 wait_count;                 /* timeout of total wait      */
	acl_pthread_job_t    *job_first;      /* thread's work queue first  */
	acl_pthread_job_t    *job_last;       /* thread's work queue last   */
	int   qlen;                           /* the work queue's length    */
	thread_cond          *cond;
	acl_pthread_mutex_t  *mutex;
} thread_worker;

#undef	USE_SLOT                              /* it's just for experiment   */

struct acl_pthread_pool_t {
	acl_pthread_mutex_t   worker_mutex;   /* control access to queue    */
	acl_pthread_cond_t    cond;           /* wait for worker quit       */
	acl_pthread_mutex_t   poller_mutex;   /* just for wait poller exit  */
	acl_pthread_cond_t    poller_cond;    /* just for wait poller exit  */
	acl_pthread_attr_t    attr;           /* create detached            */
	acl_pthread_job_t    *job_first;      /* work queue first           */
	acl_pthread_job_t    *job_last;       /* work queue last            */
	acl_pthread_job_t    *job_slot_first; /* work queue first           */
	acl_pthread_job_t    *job_slot_last;  /* work queue last            */
#ifdef	USE_SLOT
	acl_pthread_mutex_t   slot_mutex;
#endif
	thread_worker        *thr_first;      /* first idle thread          */
	thread_worker        *thr_iter;       /* for bat operation          */
	thread_cond          *cond_first;
	int   poller_running;                 /* is poller thread running ? */
	int   qlen;                           /* the work queue's length    */
	int   job_nslot;
	int   qlen_warn;                      /* the work queue's length    */
	int   valid;                          /* valid                      */
	int   quit;                           /* worker should quit         */
	int   poller_quit;                    /* poller should quit         */
	int   parallelism;                    /* maximum threads            */
	int   count;                          /* current threads            */
	int   idle;                           /* idle threads               */
	int   idle_timeout;                   /* idle timeout second        */
	acl_int64 schedule_warn;              /* schedule warn: millisecond */
	acl_int64 schedule_wait;              /* schedule wait: millisecond */
	int   overload_wait;                  /* when too busy, sleep time  */
	time_t last_warn;                     /* last warn time             */
	int  (*poller_fn)(void *arg);         /* worker poll function       */
	void *poller_arg;                     /* the arg of poller_fn       */
	int  (*worker_init_fn)(void *arg);    /* the arg is worker_init_arg */
	void *worker_init_arg;
	void (*worker_free_fn)(void *arg);    /* the arg is worker_free_arg */
	void *worker_free_arg;
};

#undef	SET_ERRNO
#ifdef	ACL_WINDOWS
# define	SET_ERRNO(_x_) (void) 0
#elif	defined(ACL_UNIX)
# define	SET_ERRNO(_x_) (acl_set_error(_x_))
#else
# error "unknown OS type"
#endif

#ifdef	ACL_WINDOWS
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
#elif	defined(ACL_WINDOWS)
	unsigned long id = acl_pthread_self();
#else
        # error "unknown OS"
#endif

	if (thr_pool->poller_fn == NULL)
		acl_msg_fatal("%s, %s(%d): poller_fn is null!",
			__FILE__, myname, __LINE__);

	acl_debug(ACL_DEBUG_THR_POOL, 2) ("%s(%d): poller(tid=%lu) started.",
		myname, __LINE__, (unsigned long) id);
	loop_count = 0;
	pre_loop_t = time(NULL);

	if (acl_pthread_mutex_lock(&thr_pool->poller_mutex) != 0)
		abort();

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

	acl_debug(ACL_DEBUG_THR_POOL, 2) ("%s(%d): poller(%lu) thread quit.",
		myname, __LINE__, (unsigned long) id);

	thr_pool->poller_running = 0;
		
	if (acl_pthread_cond_broadcast(&thr_pool->poller_cond) != 0)
		abort();

	acl_debug(ACL_DEBUG_THR_POOL, 3) ("poller broadcast ok");
	if (acl_pthread_mutex_unlock(&thr_pool->poller_mutex) != 0)
		abort();
	acl_debug(ACL_DEBUG_THR_POOL, 3) ("poller unlock ok");

	return NULL;
}

static thread_cond *thread_cond_create(void)
{
	thread_cond *cond = (thread_cond*)
		acl_mycalloc(1, sizeof(thread_cond));

	if (acl_pthread_cond_init(&cond->cond, NULL) != 0)
		abort();
	return cond;
}

static void thread_cond_free(thread_cond *cond)
{
	acl_pthread_cond_destroy(&cond->cond);
	acl_myfree(cond);
}

static thread_worker *worker_create(acl_pthread_pool_t *thr_pool)
{
	thread_worker *thr = (thread_worker*) acl_mycalloc(1,
			sizeof(thread_worker));

	thr->id = (unsigned long) acl_pthread_self();
	thr->idle = thr_pool->idle_timeout;
	if (thr->idle > 0 && thr_pool->schedule_wait > 0) {
		thr->wait_sec = thr_pool->schedule_wait / SEC_TO_MS;
		thr->wait_nsec = (thr_pool->schedule_wait * MS_TO_NS)
			% SEC_TO_NS;
		thr->wait_count = (SEC_TO_MS * thr->idle)
			/ thr_pool->schedule_wait;
		if (thr->wait_count == 0)
			thr->idle = 0;
	} else
		thr->idle = 0;

	if (thr_pool->cond_first != NULL) {
		thr->cond = thr_pool->cond_first;
		thr_pool->cond_first = thr_pool->cond_first->next;
	} else
		thr->cond = thread_cond_create();

	thr->mutex = &thr_pool->worker_mutex;
	return thr;
}

static void worker_free(acl_pthread_pool_t *thr_pool, thread_worker *thr)
{
	thr->cond->next = thr_pool->cond_first;
	thr_pool->cond_first = thr->cond;
	acl_myfree(thr);
}

static void worker_run(acl_pthread_pool_t *thr_pool,
	thread_worker *thr, acl_pthread_job_t *job)
{
	const char *myname = "worker_run";
	void (*worker_fn)(void*) = job->worker_fn;
	void *worker_arg = job->worker_arg;
	int   status;

	/* shuld unlock before enter working process */

	status = acl_pthread_mutex_unlock(thr->mutex);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_fatal("%s(%d), %s: unlock error: %s, tid: %lu",
			__FILE__, __LINE__, myname, acl_last_serror(),
			(unsigned long) acl_pthread_self());
	}

	if (job->start > 0) {
		acl_int64 now;

		SET_TIME(now);
		now -= job->start;
		if (now >= thr_pool->schedule_warn) {
			acl_msg_warn("%s(%d), %s: schedule: %lld >= %lld",
				__FILE__, __LINE__, myname,
				now, thr_pool->schedule_warn);
		}
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

static int worker_wait(acl_pthread_pool_t *thr_pool, thread_worker *thr)
{
	const char *myname = "worker_wait";
	int   status, idle_count = 0, got_job = 0;
	struct timespec  timeout;
	struct timeval   tv;
	acl_int64 n;

	/* add the thread to the idle threads pool */

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

	thr_pool->idle++;

	while (1) {

		if (thr->idle > 0) {
			gettimeofday(&tv, NULL);
			timeout.tv_sec = tv.tv_sec + (time_t) thr->wait_sec;
			n = tv.tv_usec * 1000 + thr->wait_nsec;
			if (n >= SEC_TO_NS) {
				timeout.tv_sec += 1;
				timeout.tv_nsec = (long) n - SEC_TO_NS;
			} else
				timeout.tv_nsec = (long) n;

			status = acl_pthread_cond_timedwait(&thr->cond->cond,
					thr->mutex, &timeout);
		} else
			status = acl_pthread_cond_wait(&thr->cond->cond,
					thr->mutex);

		/* if thr->job_first not null, the thread had been remove
		 * from idle threads pool by the main thread in job_deliver(),
		 * so just return 1 here.
		 */
		if (thr->job_first)
			return 1;

		/* else if threads pool's job not empty, the thread should
		 * handle it and remove itself from the idle threads pool
		 */
		if (thr_pool->job_first) {
			got_job = 1;
			break;
		}

		if (thr_pool->quit)
			break;

		if (status == ACL_ETIMEDOUT) {
			idle_count++;
			if (idle_count < thr->wait_count)
				continue;
			break;
		} else if (status == 0) {
			idle_count = 0;
			continue;
		}

		/* xxx */
		SET_ERRNO(status);
		acl_msg_warn("%s(%d), %s: tid: %lu, cond timewait error: %s",
			__FILE__, __LINE__, myname, (unsigned long)
			acl_pthread_self(), acl_last_serror());
		break;
	}

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

	thr_pool->idle--;

	/* if none job got, this must because the thread need to quit */
	if (!got_job)
		thr->quit = 1;

	return got_job;
}

static void *worker_thread(void* arg)
{
	const char *myname = "worker_thread";
	acl_pthread_pool_t *thr_pool = (acl_pthread_pool_t*) arg;
	acl_pthread_job_t *job;
	acl_pthread_mutex_t *mutex;
	thread_worker *thr;
	int   status;

	if (thr_pool->worker_init_fn != NULL) {
		if (thr_pool->worker_init_fn(thr_pool->worker_init_arg) < 0) {
			acl_msg_error("%s(%d), %s: thread(%lu) init error",
				__FILE__, __LINE__, myname,
				(unsigned long) acl_pthread_self());
			acl_pthread_mutex_lock(&thr_pool->worker_mutex);
			thr_pool->count--;
			acl_pthread_mutex_unlock(&thr_pool->worker_mutex);
			return NULL;
		}
	}

	/* lock the thread pool's global mutex at first */

	status = acl_pthread_mutex_lock(&thr_pool->worker_mutex);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_fatal("%s(%d), %s: lock failed: %s", __FILE__,
			__LINE__, myname, acl_last_serror());
	}

	thr = worker_create(thr_pool);
	mutex = thr->mutex;

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
		else if (thr_pool->job_first != NULL) {
			job = thr_pool->job_first;
			thr_pool->job_first = job->next;
			if (thr_pool->job_last == job)
				thr_pool->job_last = NULL;
			thr_pool->qlen--;

			worker_run(thr_pool, thr, job);
		}

		if (thr->job_first != NULL || thr_pool->job_first != NULL)
			continue;

		else if (thr_pool->quit)
			break;

		else if (worker_wait(thr_pool, thr) > 0)
			continue;

		/* when wait timeout, wait error or thread pool is quiting */
		if (thr->quit)
			break;
	}

	acl_debug(ACL_DEBUG_THR_POOL, 2) ("%s(%d): thread(%lu) exit now",
		myname, __LINE__, (unsigned long) acl_pthread_self());

	if (thr_pool->worker_free_fn != NULL)
		thr_pool->worker_free_fn(thr_pool->worker_free_arg);

	worker_free(thr_pool, thr);

	thr_pool->count--;

	if (thr_pool->quit) /* && thr_pool->count == 0) */
		acl_pthread_cond_signal(&thr_pool->cond);

	status = acl_pthread_mutex_unlock(mutex);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_fatal("%s, %s(%d): unlock error(%s)",
			__FILE__, myname, __LINE__, acl_last_serror());
	}

	return NULL;
}

static int job_deliver(acl_pthread_pool_t *thr_pool, thread_worker *thr,
	acl_pthread_job_t *job)
{
	const char *myname = "job_deliver";
	thread_cond *cond = thr->cond;
	int   status;

	thr->job_first = job;
	thr->job_last = job;
	thr->qlen++;

	if (thr_pool->thr_first == thr) {
		if (thr->next)
			thr->next->prev = NULL;
		thr_pool->thr_first = thr->next;
	} else {
		if (thr->next)
			thr->next->prev = thr->prev;
		thr->prev->next = thr->next;
	}

	thr_pool->idle--;

	status = acl_pthread_mutex_unlock(&thr_pool->worker_mutex);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_fatal("%s(%d), %s: pthread_mutex_unlock: %s",
			__FILE__, __LINE__, myname, acl_last_serror());
	}

	status = acl_pthread_cond_signal(&cond->cond);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_error("%s(%d), %s: pthread_cond_signal: %s",
			__FILE__, __LINE__, myname, acl_last_serror());
	}

	return 1;
}

static void job_add(acl_pthread_pool_t *thr_pool, acl_pthread_job_t *job)
{
	static const char *myname = "job_add";
	thread_worker *thr;
	int   status;

	/* must reset the job's next to NULL */
	job->next = NULL;

	if (thr_pool->schedule_warn > 0)
		SET_TIME(job->start);
	else
		job->start = 0;

	status = acl_pthread_mutex_lock(&thr_pool->worker_mutex);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_fatal("%s(%d), %s: pthread_mutex_lock: %s",
			__FILE__, __LINE__, myname, acl_last_serror());
	}

	/* at first, select one idle thread which qlen is 0 */

	thr = thr_pool->thr_first;
	if (thr && thr->qlen == 0 && job_deliver(thr_pool, thr, job) > 0)
		return;

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
		if (status == 0)
			thr_pool->count++;
		else {
			SET_ERRNO(status);
			acl_msg_error("%s(%d), %s: pthread_create: %s",
				__FILE__, __LINE__, myname, acl_last_serror());
		}

		status = acl_pthread_mutex_unlock(&thr_pool->worker_mutex);
		if (status != 0) {
			SET_ERRNO(status);
			acl_msg_fatal("%s(%d), %s: pthread_mutex_unlock: %s",
				__FILE__, __LINE__, myname, acl_last_serror());
		}
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
		if (thr_pool->overload_wait > 0) {
			acl_msg_warn("%s(%d), %s: sleep %d seconds", __FILE__,
				__LINE__, myname, thr_pool->overload_wait);
			sleep(thr_pool->overload_wait);
		}
	}

	status = acl_pthread_mutex_unlock(&thr_pool->worker_mutex);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_error("%s(%d), %s: pthread_mutex_unlock: %s",
			__FILE__, __LINE__, myname, acl_last_serror());
	}
}

void acl_pthread_pool_add_one(acl_pthread_pool_t *thr_pool,
	void (*run_fn)(void *), void *run_arg)
{
	const char *myname = "acl_pthread_pool_add";
	acl_pthread_job_t *job;
#ifdef	USE_SLOT
	int   status;
#endif

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

	job_add(thr_pool, job);
}

void acl_pthread_pool_add_job(acl_pthread_pool_t *thr_pool,
	acl_pthread_job_t *job)
{
	const char *myname = "acl_pthread_pool_add_job";

	if (thr_pool->valid != ACL_PTHREAD_POOL_VALID)
		acl_msg_fatal("%s(%d), %s: thr_pool invalid",
			__FILE__, __LINE__, myname);
	if (job == NULL)
		acl_msg_fatal("%s(%d), %s: job null",
			__FILE__, __LINE__, myname);

	job_add(thr_pool, job);
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
		if (thr_pool->overload_wait > 0) {
			acl_msg_warn("%s(%d), %s: sleep %d seconds", __FILE__,
			       	__LINE__, myname, thr_pool->overload_wait);
			sleep(thr_pool->overload_wait);
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
	thread_worker *thr_iter, *next;

	if (thr_pool->valid != ACL_PTHREAD_POOL_VALID)
		acl_msg_fatal("%s(%d), %s: invalid thr_pool->valid",
			__FILE__, __LINE__, myname);

	qlen = thr_pool->qlen;
	thr_iter = thr_pool->thr_first;

	/* iterator all the idle threads, signal one if it has job */

	for (; thr_iter != NULL ; thr_iter = next) {

		/* handle thread self's job first */

		if (thr_iter->qlen > 0) {
			next = thr_iter->next;
			if (thr_pool->thr_first == thr_iter) {
				if (thr_iter->next)
					thr_iter->prev = NULL;
				thr_pool->thr_first = thr_iter->next;
			} else {
				if (thr_iter->next)
					thr_iter->next->prev = thr_iter->prev;
				thr_iter->prev->next = thr_iter->next;
			}

			status = acl_pthread_cond_signal(&thr_iter->cond->cond);
			if (status == 0)
				continue;

			SET_ERRNO(status);
			acl_msg_fatal("%s(%d), %s: pthread_cond_signal: %s",
				__FILE__, __LINE__, myname, acl_last_serror());
		}

		/* if thread pool's job not empty , let idle thread handle */

		else if (qlen > 0) {
			next = thr_iter->next;
			status = acl_pthread_cond_signal(&thr_iter->cond->cond);
			if (status == 0) {
				qlen--;
				continue;
			}

			SET_ERRNO(status);
			acl_msg_fatal("%s(%d), %s: pthread_cond_signal: %s",
				__FILE__, __LINE__, myname, acl_last_serror());
		} else
			next = thr_iter->next;
	}

	status = acl_pthread_mutex_unlock(&thr_pool->worker_mutex);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_fatal("%s(%d), %s: pthread_mutex_unlock: %s",
			__FILE__, __LINE__, myname, acl_last_serror());
	}

	thr_pool->thr_iter = NULL;
}

static void thread_pool_init(acl_pthread_pool_t *thr_pool)
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
	thr_pool->thr_iter          = NULL;
	thr_pool->qlen              = 0;
	thr_pool->overload_wait     = 0;
	thr_pool->count             = 0;
	thr_pool->idle              = 0;
	thr_pool->schedule_warn     = 100;
	thr_pool->schedule_wait     = 100;
	thr_pool->cond_first        = NULL;
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

void acl_pthread_pool_set_schedule_warn(acl_pthread_pool_t *thr_pool,
	acl_int64 n)
{
	if (n > 0)
		thr_pool->schedule_warn = n;
}

void acl_pthread_pool_set_schedule_wait(acl_pthread_pool_t *thr_pool,
	acl_int64 n)
{
	if (n > 0)
		thr_pool->schedule_wait = n;
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
		acl_msg_fatal("%s(%d), %s: pthread_attr_init: %s",
			__FILE__, __LINE__, myname, acl_last_serror());
	}

	if (attr && attr->stack_size > 0)
		acl_pthread_attr_setstacksize(&thr_pool->attr,
			attr->stack_size);

	status = acl_pthread_attr_setdetachstate(&thr_pool->attr,
			ACL_PTHREAD_CREATE_DETACHED);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_fatal("%s(%d), %s: pthread_attr_setdetachstate: %s",
			__FILE__, __LINE__, myname, acl_last_serror());
	}

#if	defined(ACL_UNIX) && !defined(__FreeBSD__) && !defined(MINGW)
	status = pthread_attr_setscope(&thr_pool->attr, PTHREAD_SCOPE_SYSTEM);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_fatal("%s(%d), %s: pthread_attr_setscope: %s",
			__FILE__, __LINE__, myname, acl_last_serror());
	}
#endif

	status = acl_pthread_mutex_init(&thr_pool->worker_mutex, NULL);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_fatal("%s(%d), %s: pthread_mutex_init: %s",
			__FILE__, __LINE__, myname, acl_last_serror());
	}

	status = acl_pthread_cond_init(&thr_pool->cond, NULL);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_fatal("%s(%d), %s: pthread_cond_init: %s",
			__FILE__, __LINE__, myname, acl_last_serror());
	}

#ifdef	USE_SLOT
	status = acl_pthread_mutex_init(&thr_pool->slot_mutex, NULL);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_fatal("%s(%d), %s: pthread_mutex_init: %s",
			__FILE__, __LINE__, myname, acl_last_serror());
	}
#endif

	status = acl_pthread_mutex_init(&thr_pool->poller_mutex, NULL);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_fatal("%s(%d), %s: pthread_mutex_init: %s",
			__FILE__, __LINE__, myname, acl_last_serror());
	}
	status = acl_pthread_cond_init(&thr_pool->poller_cond, NULL);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_fatal("%s(%d), %s: pthread_cond_init: %s",
			__FILE__, __LINE__, myname, acl_last_serror());
	}

	thread_pool_init(thr_pool);

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

void acl_pthread_pool_set_qlen_warn(acl_pthread_pool_t *thr_pool, int max)
{
	thr_pool->qlen_warn = max;
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

	thr_pool->overload_wait = timewait;
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
		acl_msg_error("%s, %s(%d): pthread_mutex_lock: %s",
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

		acl_debug(ACL_DEBUG_THR_POOL, 2) ("%s: idle: %d, notifying",
			myname, thr_pool->idle);

		for (thr = thr_pool->thr_first; thr != NULL; thr = thr->next)
			acl_pthread_cond_signal(&thr->cond->cond);
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
		acl_msg_error("%s, %s(%d): wait_poller_exit: %s, ret: %d",
			__FILE__, myname, __LINE__, acl_last_serror(), status);
		return status;
	}

	acl_debug(ACL_DEBUG_THR_POOL, 2)
		("%s(%d): poller thread exits ok, worker count: %d",
		 myname, __LINE__, thr_pool->count);

	status = wait_worker_exit(thr_pool);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_error("%s, %s(%d): wait_worker_exit: %s, ret: %d",
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

	for (; thr_pool->cond_first != NULL;) {
		thread_cond *cond = thr_pool->cond_first;
		thr_pool->cond_first = thr_pool->cond_first->next;

		thread_cond_free(cond);
	}

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
		acl_msg_error("%s, %s(%d): wait_poller_exit: %s, ret: %d",
			__FILE__, myname, __LINE__, acl_last_serror(), status);
		return status;
	}

	acl_debug(ACL_DEBUG_THR_POOL, 2)
		("%s(%d): poller thread exits ok, worker count: %d",
		 myname, __LINE__, thr_pool->count);

	status = wait_worker_exit(thr_pool);
	if (status != 0) {
		SET_ERRNO(status);
		acl_msg_error("%s, %s(%d): wait_worker_exit: %s, ret: %d",
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

	thread_pool_init(thr_pool);

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

int acl_pthread_pool_limit(acl_pthread_pool_t *thr_pool)
{
	return thr_pool->parallelism;
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

int acl_pthread_pool_idle(acl_pthread_pool_t *thr_pool)
{
	const char *myname = "acl_pthread_pool_idle";
	int   status, n;

	status = acl_pthread_mutex_lock(&thr_pool->worker_mutex);
	if (status) {
		acl_msg_error("%s(%d), %s: pthread_mutex_lock error(%s)",
			__FILE__, __LINE__, myname, strerror(status));
		return -1;
	}

	n = thr_pool->idle;

	status = acl_pthread_mutex_unlock(&thr_pool->worker_mutex);
	if (status) {
		acl_msg_error("%s(%d), %s: pthread_mutex_unlock error(%s)",
			__FILE__, __LINE__, myname, strerror(status));
		return -1;
	}

	return n;
}

int acl_pthread_pool_busy(acl_pthread_pool_t *thr_pool)
{
	const char *myname = "acl_pthread_pool_busy";
	int   status, n;

	status = acl_pthread_mutex_lock(&thr_pool->worker_mutex);
	if (status) {
		acl_msg_error("%s(%d), %s: pthread_mutex_lock error(%s)",
			__FILE__, __LINE__, myname, strerror(status));
		return -1;
	}

	n = thr_pool->count - thr_pool->idle;

	status = acl_pthread_mutex_unlock(&thr_pool->worker_mutex);
	if (status) {
		acl_msg_error("%s(%d), %s: pthread_mutex_unlock error(%s)",
			__FILE__, __LINE__, myname, strerror(status));
		return -1;
	}

	if (n < 0)
		acl_msg_error("%s(%d), %s: threads's count(%d) < idle(%d)",
			__FILE__, __LINE__, myname, thr_pool->count,
			thr_pool->idle);
	return n;
}

int acl_pthread_pool_qlen(acl_pthread_pool_t *thr_pool)
{
	const char *myname = "acl_pthread_pool_qlen";
	int   status, n;

	status = acl_pthread_mutex_lock(&thr_pool->worker_mutex);
	if (status) {
		acl_msg_error("%s(%d), %s: pthread_mutex_lock error(%s)",
			__FILE__, __LINE__, myname, strerror(status));
		return -1;
	}

	n = thr_pool->qlen;

	status = acl_pthread_mutex_unlock(&thr_pool->worker_mutex);
	if (status) {
		acl_msg_error("%s(%d), %s: pthread_mutex_unlock error(%s)",
			__FILE__, __LINE__, myname, strerror(status));
		return -1;
	}

	if (n < 0)
		acl_msg_error("%s(%d), %s: threads's count(%d) < idle(%d)",
			__FILE__, __LINE__, myname, thr_pool->count,
			thr_pool->idle);
	return n;
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
