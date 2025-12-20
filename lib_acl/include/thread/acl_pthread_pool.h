#ifndef ACL_PTHREAD_POOL_INCLUDE_H
#define ACL_PTHREAD_POOL_INCLUDE_H

#include "../stdlib/acl_define.h"
#include <time.h>

#ifdef	ACL_UNIX
#include <pthread.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct acl_pthread_job_t acl_pthread_job_t;

/**
 * Allocate a worker job object for the thread pool.
 * @param run_fn {void (*)(void*)} Callback function executed by worker threads
 * @param run_arg {void*} Argument passed to run_fn
 * @param fixed {int} Whether the job is fixed (implementation specific)
 * @return {acl_pthread_job_t*} Newly allocated job object
 */
ACL_API acl_pthread_job_t *acl_pthread_pool_alloc_job(void (*run_fn)(void*),
		void *run_arg, int fixed);

/**
 * Free a job object allocated by acl_pthread_pool_alloc_job.
 * @param job {acl_pthread_job_t*}
 */
ACL_API void acl_pthread_pool_free_job(acl_pthread_job_t *job);

/**
 * Thread pool object type.
 */
typedef struct acl_pthread_pool_t acl_pthread_pool_t;

/**
 * Thread pool attribute structure.
 */
typedef struct acl_pthread_pool_attr_t {
	int   threads_limit;	/**< Maximum number of threads in the pool */
#define ACL_PTHREAD_POOL_DEF_THREADS   100  /**< Default maximum of 100 threads */
	int   idle_timeout;                 /**< Idle timeout of worker
					 *   threads (seconds) */
#define ACL_PTHREAD_POOL_DEF_IDLE      0    /**< Default idle timeout is 0 seconds */
	size_t stack_size;                  /**< Stack size of worker
					 *   threads (bytes) */
} acl_pthread_pool_attr_t;

/**
 * Simple helper to create a thread pool object.
 * @param threads_limit {int} Maximum number of concurrent worker threads
 * @param idle_timeout {int} Idle timeout for threads before exit (seconds)
 * @return {acl_pthread_pool_t*} Non-NULL on success; NULL on failure
 */
ACL_API acl_pthread_pool_t *acl_thread_pool_create(
		int threads_limit, int idle_timeout);

/**
 * Create a thread pool object.
 * @param attr {acl_pthread_pool_attr_t*} Optional attributes; if NULL,
 *  default values ACL_PTHREAD_POOL_DEF_XXX are used
 * @return {acl_pthread_pool_t*} Non-NULL on success; NULL on failure
 */
ACL_API acl_pthread_pool_t *acl_pthread_pool_create(
		const acl_pthread_pool_attr_t *attr);

/**
 * Adjust the waiting time for queued jobs when there are fewer
 * than two idle threads.
 * This controls how long worker threads wait before taking new jobs.
 * @param thr_pool {acl_pthread_pool_t*} Non-NULL thread pool
 * @param timewait_sec {int} Wait time in seconds, usually in the range 1-5
 * @return {int} 0 on success; -1 on failure
 */
ACL_API int acl_pthread_pool_set_timewait(
		acl_pthread_pool_t *thr_pool, int timewait_sec);

/**
 * Register an initialization callback that is executed when a
 * worker thread starts.
 * @param thr_pool {acl_pthread_pool_t*} Non-NULL thread pool
 * @param init_fn {int (*)(void*)} Initialization function; if it returns < 0,
 *  the worker thread will exit automatically
 * @param init_arg {void*} Argument passed to init_fn
 * @return {int} 0: OK; != 0: Error
 */
ACL_API int acl_pthread_pool_atinit(acl_pthread_pool_t *thr_pool,
		int (*init_fn)(void *), void *init_arg);

/**
 * Register a cleanup callback that is executed when a worker thread exits.
 * @param thr_pool {acl_pthread_pool_t*} Non-NULL thread pool
 * @param free_fn {void (*)(void*)} Cleanup function executed
 *  before a worker exits
 * @param free_arg {void*} Argument passed to free_fn
 * @return {int} 0: OK; != 0: Error
 */
ACL_API int acl_pthread_pool_atfree(acl_pthread_pool_t *thr_pool,
		void (*free_fn)(void *), void *free_arg);

/**
 * Destroy a thread pool and release all its resources.
 * @param thr_pool {acl_pthread_pool_t*} Non-NULL thread pool
 * @return {int} 0 on success; non-zero on failure
 */
ACL_API int acl_pthread_pool_destroy(acl_pthread_pool_t *thr_pool);

/**
 * Stop a thread pool from accepting new jobs; existing jobs may
 * still be processed.
 * @param thr_pool {acl_pthread_pool_t*} Non-NULL thread pool
 * @return {int} 0 on success; non-zero on failure
 */
ACL_API int acl_pthread_pool_stop(acl_pthread_pool_t *thr_pool);

/**
 * Add one job to the thread pool.
 * @param thr_pool {acl_pthread_pool_t*} Non-NULL thread pool
 * @param run_fn {void (*)(*)} Callback executed by worker threads
 * @param run_arg {void*} Argument passed to run_fn
 */
ACL_API void acl_pthread_pool_add_one(acl_pthread_pool_t *thr_pool,
		void (*run_fn)(void *), void *run_arg);
#define	acl_pthread_pool_add	acl_pthread_pool_add_one

/**
 * Add one pre-allocated job to the thread pool.
 * @param thr_pool {acl_pthread_pool_t*} Non-NULL thread pool
 * @param job {acl_pthread_job_t*} Job created by acl_pthread_pool_alloc_job
 */
ACL_API void acl_pthread_pool_add_job(acl_pthread_pool_t *thr_pool,
		acl_pthread_job_t *job);

/**
 * Begin batch mode for adding jobs; effectively marks the start of a batch.
 * @param thr_pool {acl_pthread_pool_t*} Non-NULL thread pool
 */
ACL_API void acl_pthread_pool_bat_add_begin(acl_pthread_pool_t *thr_pool);

/**
 * Add one job in batch mode; must be preceded by a successful call to
 * acl_pthread_pool_bat_add_begin.
 * @param thr_pool {acl_pthread_pool_t*} Non-NULL thread pool
 * @param run_fn {void (*)(void*)} Callback executed by worker threads
 * @param run_arg Argument passed to run_fn
 */
ACL_API void acl_pthread_pool_bat_add_one(acl_pthread_pool_t *thr_pool,
		void (*run_fn)(void *), void *run_arg);
/**
 * Add one pre-allocated job in batch mode; must be preceded by a successful
 * call to acl_pthread_pool_bat_add_begin.
 * @param thr_pool {acl_pthread_pool_t*} Non-NULL thread pool
 * @param job {acl_pthread_job_t*} Job created by acl_pthread_pool_alloc_job
 */
ACL_API void acl_pthread_pool_bat_add_job(acl_pthread_pool_t *thr_pool,
		acl_pthread_job_t *job);

/**
 * End batch mode for adding jobs; actually submits the batch to the pool.
 * @param thr_pool {acl_pthread_pool_t*} Non-NULL thread pool
 */
ACL_API void acl_pthread_pool_bat_add_end(acl_pthread_pool_t *thr_pool);

/**
 * Set the thread pool POLLER dispatch function. To use this feature, call
 * acl_pthread_pool_create, then configure the poller with this function, and
 * finally call acl_pthread_pool_start_poller to start a background poller
 * thread. The poller repeatedly invokes poller_fn, inside which the user
 * typically calls acl_pthread_pool_add to push jobs into the pool.
 * @param thr_pool {acl_pthread_pool_t*} Non-NULL thread pool
 * @param poller_fn {int (*)(void*)} Loop callback executed by the poller thread
 * @param poller_arg {void*} Argument passed to poller_fn
 */
ACL_API void acl_pthread_pool_set_poller(acl_pthread_pool_t *thr_pool,
		int (*poller_fn)(void *), void *poller_arg);
/**
 * Start a POLLER thread for the thread pool.
 * @param thr_pool {acl_pthread_pool_t*} Non-NULL thread pool
 * @return 0 on success; non-zero on failure. strerror(ret) can
 *  be used to retrieve the error description.
 */
ACL_API int acl_pthread_pool_start_poller(acl_pthread_pool_t *thr_pool);

/**
 * Dispatcher callback for adding jobs in "internal" mode;
 * internally this simply calls acl_pthread_pool_add_one().
 * @return 0: OK; -1: error
 */
ACL_API int acl_pthread_pool_add_dispatch(void *dispatch_arg,
		void (*run_fn)(void *), void *run_arg);

/**
 * Dispatcher callback for adding jobs in "external" mode.
 * @return 0: OK; -1: error
 * Note: The second parameter of worker_fn is a pointer to an ACL_WORKER_ATTR
 *     structure, whose member init_data can be used by worker
 *     threads to maintain user-specific data such as database
 *     connections.
 */
ACL_API int acl_pthread_pool_dispatch(void *dispatch_arg,
		void (*run_fn)(void *), void *run_arg);

/**
 * Get the maximum number of threads allowed in the thread pool.
 * @param thr_pool {acl_pthread_pool_t*} Non-NULL thread pool
 * @return {int} Maximum thread limit
 */
ACL_API int acl_pthread_pool_limit(acl_pthread_pool_t *thr_pool);

/**
 * Get the current number of threads in the thread pool.
 * @param thr_pool {acl_pthread_pool_t*} Non-NULL thread pool
 * @return {int} Current thread count; < 0 indicates error
 */
ACL_API int acl_pthread_pool_size(acl_pthread_pool_t *thr_pool);

/**
 * Get the number of idle threads currently in the thread pool.
 * @param thr_pool {acl_pthread_pool_t*} Non-NULL thread pool
 * @return {int} Number of idle threads; < 0 indicates error
 */
ACL_API int acl_pthread_pool_idle(acl_pthread_pool_t *thr_pool);

/**
 * Get the number of busy threads currently in the thread pool.
 * @param thr_pool {acl_pthread_pool_t*} Non-NULL thread pool
 * @return {int} Number of busy threads; < 0 indicates error
 */
ACL_API int acl_pthread_pool_busy(acl_pthread_pool_t *thr_pool);

/**
 * Set the warning threshold for job scheduling delay (for logging/diagnostics).
 * @param thr_pool {acl_pthread_pool_t*} Non-NULL thread pool
 * @param n {acl_int64} If > 0, log a warning when the scheduling
 *  delay exceeds this value
 */
ACL_API void acl_pthread_pool_set_schedule_warn(
		acl_pthread_pool_t *thr_pool, acl_int64 n);

/**
 * Set the warning threshold for how long threads wait for jobs
 * (for logging/diagnostics).
 * @param thr_pool {acl_pthread_pool_t*} Non-NULL thread pool
 * @param n {acl_int64} If > 0, log a warning when the wait time
 *  exceeds this value
 */
ACL_API void acl_pthread_pool_set_schedule_wait(
		acl_pthread_pool_t *thr_pool, acl_int64 n);

/**
 * Set the warning threshold for the length of the global work queue.
 * @param thr_pool {acl_pthread_pool_t*} Non-NULL thread pool
 * @param max {int} Maximum queue length before a warning is issued
 */
ACL_API void acl_pthread_pool_set_qlen_warn(
		acl_pthread_pool_t *thr_pool, int max);
/**
 * Get the number of pending jobs in the global queue.
 * @param thr_pool {acl_pthread_pool_t*} Non-NULL thread pool
 * @return {int} Number of pending jobs; < 0 indicates error
 */
ACL_API int acl_pthread_pool_qlen(acl_pthread_pool_t *thr_pool);

/**
 * Set the stack size of worker threads in the thread pool.
 * @param thr_pool {acl_pthread_pool_t*} Non-NULL thread pool
 * @param size {size_t} Stack size at thread creation time, in bytes
 */
ACL_API void acl_pthread_pool_set_stacksize(
		acl_pthread_pool_t *thr_pool, size_t size);

/**
 * Initialize a thread pool attribute structure with default values.
 * @param attr {acl_pthread_pool_attr_t*}
 */
ACL_API void acl_pthread_pool_attr_init(acl_pthread_pool_attr_t *attr);

/**
 * Set the default stack size (bytes) in the thread pool attributes.
 * @param attr {acl_pthread_pool_attr_t*}
 * @param size {size_t}
 */
ACL_API void acl_pthread_pool_attr_set_stacksize(
		acl_pthread_pool_attr_t *attr, size_t size);

/**
 * Set the maximum number of threads in the thread pool attributes.
 * @param attr {acl_pthread_pool_attr_t*}
 * @param threads_limit {int} Maximum number of threads
 */
ACL_API void acl_pthread_pool_attr_set_threads_limit(
		acl_pthread_pool_attr_t *attr, int threads_limit);

/**
 * Set the idle timeout for threads in the thread pool attributes.
 * @param attr {acl_pthread_pool_attr_t*}
 * @param idle_timeout {int} Idle timeout in seconds
 */
ACL_API void acl_pthread_pool_attr_set_idle_timeout(
		acl_pthread_pool_attr_t *attr, int idle_timeout);

#ifdef	__cplusplus
}
#endif

#endif	/* !__acl_pthread_pool_t_H_INCLUDED__ */
