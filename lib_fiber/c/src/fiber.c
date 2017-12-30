#include "stdafx.h"
#include <pthread.h>
#include "common.h"

#ifdef USE_VALGRIND
#include <valgrind/valgrind.h>
#endif

//#define FIBER_STACK_GUARD
#ifdef	FIBER_STACK_GUARD
#include <sys/mman.h>
#endif

#include "fiber/lib_fiber.h"
#include "fiber.h"

#define	MAX_CACHE	1000

typedef int  *(*errno_fn)(void);
typedef int   (*fcntl_fn)(int, int, ...);

static errno_fn __sys_errno     = NULL;
static fcntl_fn __sys_fcntl     = NULL;

typedef struct THREAD {
	RING       ready;		/* ready fiber queue */
	RING       dead;		/* dead fiber queue */
	ACL_FIBER **fibers;
	unsigned   size;
	unsigned   slot;
	int        exitcode;
	ACL_FIBER *running;
	ACL_FIBER *original;
	int        errnum;
	unsigned   idgen;
	int        count;
	size_t     switched;
	int        nlocal;
} THREAD;

static THREAD          *__main_fiber   = NULL;
static __thread THREAD *__thread_fiber = NULL;
static __thread int __scheduled = 0;
__thread int var_hook_sys_api   = 0;

static pthread_key_t __fiber_key;
static void fiber_init(void);

void acl_fiber_hook_api(int onoff)
{
	var_hook_sys_api = onoff;
}

int acl_fiber_scheduled(void)
{
	return __scheduled;
}

static void thread_free(void *ctx)
{
	THREAD *tf = (THREAD *) ctx;

	if (__thread_fiber == NULL) {
		return;
	}

	if (tf->fibers) {
		free(tf->fibers);
	}

	tf->original->free_fn(tf->original);
	free(tf);

	if (__main_fiber == __thread_fiber) {
		__main_fiber = NULL;
	}
	__thread_fiber = NULL;
}

static void fiber_schedule_main_free(void)
{
	if (__main_fiber) {
		thread_free(__main_fiber);
		if (__thread_fiber == __main_fiber) {
			__thread_fiber = NULL;
		}
		__main_fiber = NULL;
	}
}

static void thread_init(void)
{
	if (pthread_key_create(&__fiber_key, thread_free) != 0) {
		msg_fatal("%s(%d), %s: pthread_key_create error %s",
			__FILE__, __LINE__, __FUNCTION__, last_serror());
	}
}

static pthread_once_t __once_control = PTHREAD_ONCE_INIT;

static void fiber_check(void)
{
	if (__thread_fiber != NULL) {
		return;
	}

	if (pthread_once(&__once_control, thread_init) != 0) {
		msg_fatal("%s(%d), %s: pthread_once error %s",
			__FILE__, __LINE__, __FUNCTION__, last_serror());
	}

	__thread_fiber = (THREAD *) calloc(1, sizeof(THREAD));

#ifdef SYS_UNIX
	__thread_fiber->original = fiber_unix_origin();
#endif
	__thread_fiber->fibers   = NULL;
	__thread_fiber->size     = 0;
	__thread_fiber->slot     = 0;
	__thread_fiber->idgen    = 0;
	__thread_fiber->count    = 0;
	__thread_fiber->nlocal   = 0;

	ring_init(&__thread_fiber->ready);
	ring_init(&__thread_fiber->dead);

	if ((unsigned long) pthread_self() == main_thread_self()) {
		__main_fiber = __thread_fiber;
		atexit(fiber_schedule_main_free);
	} else if (pthread_setspecific(__fiber_key, __thread_fiber) != 0) {
		msg_fatal("pthread_setspecific error!");
	}
}

#ifdef	HOOK_ERRNO

/* see /usr/include/bits/errno.h for __errno_location */
#ifdef ACL_ARM_LINUX
volatile int*   __errno(void)
#else
int *__errno_location(void)
#endif
{
	if (!var_hook_sys_api) {
		if (__sys_errno == NULL) {
			fiber_init();
		}

		return __sys_errno();
	}

	if (__thread_fiber == NULL) {
		fiber_check();
	}

	if (__thread_fiber->running) {
		return &__thread_fiber->running->errnum;
	} else {
		return &__thread_fiber->original.errnum;
	}
}

#endif

int acl_fiber_sys_errno(void)
{
	if (__sys_errno == NULL) {
		fiber_init();
	}
	return *__sys_errno();
}

void acl_fiber_sys_errno_set(int errnum)
{
	if (__sys_errno == NULL) {
		fiber_init();
	}
	*__sys_errno() = errnum;
}

#ifdef SYS_UNIX

int fcntl(int fd, int cmd, ...)
{
	long arg;
	struct flock *lock;
	va_list ap;
	int ret;

	if (__sys_fcntl == NULL) {
		fiber_init();
	}

	va_start(ap, cmd);

	switch (cmd) {
	case F_GETFD:
	case F_GETFL:
		ret = __sys_fcntl(fd, cmd);
		break;
	case F_SETFD:
	case F_SETFL:
		arg = va_arg(ap, long);
		ret = __sys_fcntl(fd, cmd, arg);
		break;
	case F_GETLK:
	case F_SETLK:
	case F_SETLKW:
		lock = va_arg(ap, struct flock*);
		ret = __sys_fcntl(fd, cmd, lock);
		break;
	default:
		ret = -1;
		msg_error("%s(%d), %s: unknown cmd: %d, fd: %d",
			__FILE__, __LINE__, __FUNCTION__, cmd, fd);
		break;
	}

	va_end(ap);

	if (ret < 0) {
		fiber_save_errno();
	}

	return ret;
}

#endif

void acl_fiber_set_errno(ACL_FIBER *fiber, int errnum)
{
	if (fiber == NULL) {
		fiber = acl_fiber_running();
	}
	if (fiber) {
		fiber->errnum = errnum;
	}
}

int acl_fiber_errno(ACL_FIBER *fiber)
{
	if (fiber == NULL) {
		fiber = acl_fiber_running();
	}
	return fiber ? fiber->errnum : 0;
}

void acl_fiber_keep_errno(ACL_FIBER *fiber, int yesno)
{
	if (fiber == NULL) {
		fiber = acl_fiber_running();
	}
	if (fiber) {
		if (yesno) {
			fiber->flag |= FIBER_F_SAVE_ERRNO;
		} else {
			fiber->flag &= ~FIBER_F_SAVE_ERRNO;
		}
	}
}

void fiber_save_errno(void)
{
	ACL_FIBER *curr;

	if (__thread_fiber == NULL) {
		fiber_check();
	}

	if ((curr = __thread_fiber->running) == NULL) {
		curr = __thread_fiber->original;
	}

	if (curr->flag & FIBER_F_SAVE_ERRNO) {
		//curr->flag &= ~FIBER_F_SAVE_ERRNO;
		return;
	}

	if (__sys_errno != NULL) {
		acl_fiber_set_errno(curr, *__sys_errno());
	} else {
		acl_fiber_set_errno(curr, errno);
	}
}

static void fiber_kick(int max)
{
	RING *head;
	ACL_FIBER *fiber;

	while (max > 0) {
		head = ring_pop_head(&__thread_fiber->dead);
		if (head == NULL) {
			break;
		}
		fiber = RING_TO_APPL(head, ACL_FIBER,me);
		fiber_free(fiber);
		max--;
	}
}

static void fiber_swap(ACL_FIBER *from, ACL_FIBER *to)
{
	if (from->status == FIBER_STATUS_EXITING) {
		size_t slot = from->slot;
		int n = ring_size(&__thread_fiber->dead);

		/* if the cached dead fibers reached the limit,
		 * some will be freed
		 */
		if (n > MAX_CACHE) {
			n -= MAX_CACHE;
			fiber_kick(n);
		}

		if (!from->sys) {
			__thread_fiber->count--;
		}

		__thread_fiber->fibers[slot] =
			__thread_fiber->fibers[--__thread_fiber->slot];
		__thread_fiber->fibers[slot]->slot = slot;

		ring_prepend(&__thread_fiber->dead, &from->me);
	}

	from->swap_fn(from, to);
}

static void check_timer(ACL_FIBER *fiber fiber_unused, void *ctx)
{
	size_t *intptr = (size_t *) ctx;
	size_t  max = *intptr;

	free(intptr);
	while (1) {
#ifdef SYS_WIN
		Sleep(1000);
#else
		sleep(1);
#endif
		fiber_kick((int) max);
	}
}

void acl_fiber_check_timer(size_t max)
{
	size_t *intptr = (size_t *) malloc(sizeof(int));

	*intptr = max;
	acl_fiber_create(check_timer, intptr, 64000);
}

ACL_FIBER *acl_fiber_running(void)
{
	fiber_check();
	return __thread_fiber->running;
}

void acl_fiber_kill(ACL_FIBER *fiber)
{
	acl_fiber_signal(fiber, SIGTERM);
}

int acl_fiber_killed(ACL_FIBER *fiber)
{
	if (!fiber) {
		fiber = acl_fiber_running();
	}
	return fiber && (fiber->flag & FIBER_F_KILLED);
}

void acl_fiber_signal(ACL_FIBER *fiber, int signum)
{
	ACL_FIBER *curr = __thread_fiber->running;

	if (fiber == NULL) {
		msg_error("%s(%d), %s: fiber NULL",
			__FILE__, __LINE__, __FUNCTION__);
		return;
	}

	if (curr == NULL) {
		msg_error("%s(%d), %s: current fiber NULL",
			__FILE__, __LINE__, __FUNCTION__);
		return;
	}

#ifdef SYS_WIN
	if (signum == SIGTERM) {
#else
	if (signum == SIGKILL || signum == SIGTERM || signum == SIGQUIT) {
#endif
		fiber->errnum = ECANCELED;
		fiber->flag |= FIBER_F_KILLED;
	}

	fiber->signum = signum;

	if (fiber == curr) { // just return if kill myself
		return;
	}

	ring_detach(&curr->me);
	ring_detach(&fiber->me);

	/* add the current fiber and signed fiber in the head of the ready */
#if 0
	fiber_ready(fiber);
	fiber_yield();
#else
	curr->status = FIBER_STATUS_READY;
	ring_append(&__thread_fiber->ready, &curr->me);

	fiber->status = FIBER_STATUS_READY;
	ring_append(&__thread_fiber->ready, &fiber->me);

	acl_fiber_switch();
#endif
}

int acl_fiber_signum(ACL_FIBER *fiber)
{
	if (fiber) {
		fiber = acl_fiber_running();
	}
	return fiber ? fiber->signum : 0;
}

void fiber_exit(int exit_code)
{
	fiber_check();

	__thread_fiber->exitcode = exit_code;
	__thread_fiber->running->status = FIBER_STATUS_EXITING;

	acl_fiber_switch();
}

void acl_fiber_ready(ACL_FIBER *fiber)
{
	if (fiber->status != FIBER_STATUS_EXITING) {
		fiber->status = FIBER_STATUS_READY;
		ring_prepend(&__thread_fiber->ready, &fiber->me);
	}
}

int acl_fiber_yield(void)
{
	size_t  n;

	if (ring_size(&__thread_fiber->ready) == 0) {
		return 0;
	}

	n = __thread_fiber->switched;
	acl_fiber_ready(__thread_fiber->running);
	acl_fiber_switch();

	// when switched overflows, it will be set to 0, then n saved last
	// switched's value will larger than switched, so we need to use
	// abs function to avoiding this problem
	return abs(__thread_fiber->switched - n - 1);
}

int acl_fiber_ndead(void)
{
	if (__thread_fiber == NULL) {
		return 0;
	}
	return ring_size(&__thread_fiber->dead);
}

static void fbase_init(FIBER_BASE *fbase, int flag)
{
	fbase->flag      = flag;
	fbase->mutex_in  = -1;
	fbase->mutex_out = -1;
	fbase->atomic    = NULL;
	ring_init(&fbase->mutex_waiter);

	assert(fbase->atomic == NULL);

	fbase->atomic = atomic_new();
	atomic_set(fbase->atomic, &fbase->atomic_value);
	atomic_int64_set(fbase->atomic, 0);
}

static void fbase_finish(FIBER_BASE *fbase)
{
	fbase_event_close(fbase);
	atomic_free(fbase->atomic);
}

FIBER_BASE *fbase_alloc(void)
{
	FIBER_BASE *fbase = (FIBER_BASE *) calloc(1, sizeof(FIBER_BASE));

	fbase_init(fbase, FBASE_F_BASE);
	return fbase;
}

void fbase_free(FIBER_BASE *fbase)
{
	fbase_finish(fbase);
	free(fbase);
}

void fiber_free(ACL_FIBER *fiber)
{
	fbase_finish(&fiber->base);
	fiber->free_fn(fiber);
}

static void fiber_start(ACL_FIBER *fiber)
{
	int i;

	fiber->fn(fiber, fiber->arg);

	for (i = 0; i < fiber->nlocal; i++) {
		if (fiber->locals[i] == NULL) {
			continue;
		}
		if (fiber->locals[i]->free_fn) {
			fiber->locals[i]->free_fn(fiber->locals[i]->ctx);
		}
		free(fiber->locals[i]);
	}

	if (fiber->locals) {
		free(fiber->locals);
		fiber->locals = NULL;
		fiber->nlocal = 0;
	}

	fiber_exit(0);
}

static ACL_FIBER *fiber_alloc(void (*fn)(ACL_FIBER *, void *),
	void *arg, size_t size)
{
	ACL_FIBER *fiber = NULL;
	RING *head;

	fiber_check();

#define	APPL	RING_TO_APPL

	/* try to reuse the fiber memory in dead queue */
	head = ring_pop_head(&__thread_fiber->dead);
	if (head == NULL) {
#ifdef SYS_UNIX
		fiber = fiber_unix_alloc(fiber_start);
#endif
	} else if ((fiber = APPL(head, ACL_FIBER, me))->size < size) {
		/* if using realloc, real memory will be used, when we first
		 * free and malloc again, then we'll just use virtual memory,
		 * because memcpy will be called in realloc.
		 */
		stack_free(fiber->buff);
		fiber->buff = (char *) stack_alloc(size);
	} else {
		size = fiber->size;
	}

	__thread_fiber->idgen++;
	if (__thread_fiber->idgen == 0) { /* overflow ? */
		__thread_fiber->idgen++;
	}

	fiber->id     = __thread_fiber->idgen;
	fiber->errnum = 0;
	fiber->signum = 0;
	fiber->fn     = fn;
	fiber->arg    = arg;
	fiber->size   = size;
	fiber->flag   = 0;
	fiber->status = FIBER_STATUS_READY;

	fiber->waiting = NULL;
	ring_init(&fiber->holding);

#ifdef SYS_UNIX
	fiber_unit_init(fiber);
#endif
	return fiber;
}

ACL_FIBER *acl_fiber_create(void (*fn)(ACL_FIBER *, void *),
	void *arg, size_t size)
{
	ACL_FIBER *fiber = fiber_alloc(fn, arg, size);

	__thread_fiber->count++;

	if (__thread_fiber->slot >= __thread_fiber->size) {
		__thread_fiber->size  += 128;
		__thread_fiber->fibers = (ACL_FIBER **) realloc(
			__thread_fiber->fibers, 
			__thread_fiber->size * sizeof(ACL_FIBER *));
	}

	fiber->slot = __thread_fiber->slot;
	__thread_fiber->fibers[__thread_fiber->slot++] = fiber;

	acl_fiber_ready(fiber);

	return fiber;
}

unsigned int acl_fiber_id(const ACL_FIBER *fiber)
{
	return fiber ? fiber->id : 0;
}

unsigned int acl_fiber_self(void)
{
	ACL_FIBER *curr = acl_fiber_running();
	return acl_fiber_id(curr);
}

int acl_fiber_status(const ACL_FIBER *fiber)
{
	if (fiber == NULL) {
		fiber = acl_fiber_running();
	}
	return fiber ? fiber->status : 0;
}

static void fiber_init(void)
{
#ifdef SYS_UNIX

	static pthread_mutex_t __lock = PTHREAD_MUTEX_INITIALIZER;
	static int __called = 0;

	(void) pthread_mutex_lock(&__lock);

	if (__called != 0) {
		(void) pthread_mutex_unlock(&__lock);
		return;
	}

	__called++;

#ifdef ACL_ARM_LINUX
	__sys_errno   = (errno_fn) dlsym(RTLD_NEXT, "__errno");
#else
	__sys_errno   = (errno_fn) dlsym(RTLD_NEXT, "__errno_location");
#endif
	__sys_fcntl   = (fcntl_fn) dlsym(RTLD_NEXT, "fcntl");

	(void) pthread_mutex_unlock(&__lock);
#endif
}

void acl_fiber_schedule(void)
{
	ACL_FIBER *fiber;
	RING *head;

	fiber_check();
	acl_fiber_hook_api(1);
	__scheduled = 1;

	for (;;) {
		head = ring_pop_head(&__thread_fiber->ready);
		if (head == NULL) {
#ifdef	FREEBSD
			msg_info("thread-%lu: NO FIBER NOW", (long) pthread_self());
#else
			msg_info("thread-%lu: NO FIBER NOW", pthread_self());
#endif
			break;
		}

		fiber = RING_TO_APPL(head, ACL_FIBER, me);
		fiber->status = FIBER_STATUS_READY;

		__thread_fiber->running = fiber;
		__thread_fiber->switched++;

		fiber_swap(__thread_fiber->original, fiber);
		__thread_fiber->running = NULL;
	}

	/* release dead fiber */
	while ((head = ring_pop_head(&__thread_fiber->dead)) != NULL) {
		fiber = RING_TO_APPL(head, ACL_FIBER, me);
		fiber_free(fiber);
	}

	acl_fiber_hook_api(0);
	__scheduled = 0;
}

void fiber_system(void)
{
	if (!__thread_fiber->running->sys) {
		__thread_fiber->running->sys = 1;
		__thread_fiber->count--;
	}
}

void fiber_count_inc(void)
{
	__thread_fiber->count++;
}

void fiber_count_dec(void)
{
	__thread_fiber->count--;
}

void acl_fiber_switch(void)
{
	ACL_FIBER *fiber, *current = __thread_fiber->running;
	RING *head;
	ssize_t left = (char *) &fiber - (char *) current->buff;

	// xxx: sanity checking, just for stack size checking
	if (left < 1024) {
		msg_fatal("%s(%d), %s: left=%ld, no space to save current"
			" fiber's stack, current stack=%p, start stack=%p",
			__FILE__, __LINE__, __FUNCTION__, (long) left,
			(void *) &fiber, (void *) current->buff);
	}

#ifdef _DEBUG
	assert(current);
#endif

	head = ring_pop_head(&__thread_fiber->ready);
	if (head == NULL) {
#ifdef	FREEBSD
		msg_info("thread-%lu: NO FIBER in ready", (long) pthread_self());
#else
		msg_info("thread-%lu: NO FIBER in ready", pthread_self());
#endif
		fiber_swap(current, __thread_fiber->original);
		return;
	}

	fiber = RING_TO_APPL(head, ACL_FIBER, me);
	//fiber->status = FIBER_STATUS_READY;

	__thread_fiber->running = fiber;
	__thread_fiber->switched++;

	fiber_swap(current, __thread_fiber->running);
}

int acl_fiber_set_specific(int *key, void *ctx, void (*free_fn)(void *))
{
	FIBER_LOCAL *local;
	ACL_FIBER *curr;

	if (key == NULL) {
		msg_error("%s(%d), %s: key NULL",
			__FILE__, __LINE__, __FUNCTION__);
		return -1;
	}

	if (__thread_fiber == NULL) {
		msg_error("%s(%d), %s: __thread_fiber: NULL",
			__FILE__, __LINE__, __FUNCTION__);
		return -1;
	} else if (__thread_fiber->running == NULL) {
		msg_error("%s(%d), %s: running: NULL",
			__FILE__, __LINE__, __FUNCTION__);
		return -1;
	} else
		curr = __thread_fiber->running;

	if (*key <= 0) {
		*key = ++__thread_fiber->nlocal;
	} else if (*key > __thread_fiber->nlocal) {
		msg_error("%s(%d), %s: invalid key: %d > nlocal: %d",
			__FILE__, __LINE__, __FUNCTION__,
			*key, __thread_fiber->nlocal);
		return -1;
	}

	if (curr->nlocal < __thread_fiber->nlocal) {
		int i, n = curr->nlocal;
		curr->nlocal = __thread_fiber->nlocal;
		curr->locals = (FIBER_LOCAL **) realloc(curr->locals,
			curr->nlocal * sizeof(FIBER_LOCAL*));
		for (i = n; i < curr->nlocal; i++)
			curr->locals[i] = NULL;
	}

	local = (FIBER_LOCAL *) calloc(1, sizeof(FIBER_LOCAL));
	local->ctx = ctx;
	local->free_fn = free_fn;
	curr->locals[*key - 1] = local;

	return *key;
}

void *acl_fiber_get_specific(int key)
{
	FIBER_LOCAL *local;
	ACL_FIBER *curr;

	if (key <= 0) {
		return NULL;
	}

	if (__thread_fiber == NULL) {
		msg_error("%s(%d), %s: __thread_fiber NULL",
			__FILE__, __LINE__, __FUNCTION__);
		return NULL;
	} else if (__thread_fiber->running == NULL) {
		msg_error("%s(%d), %s: running fiber NULL",
			__FILE__, __LINE__, __FUNCTION__);
		return NULL;
	} else
		curr = __thread_fiber->running;

	if (key > curr->nlocal) {
		return NULL;
	}

	local = curr->locals[key - 1];

	return local ? local->ctx : NULL;
}
