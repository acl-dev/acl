#include "stdafx.h"
#define __USE_GNU
#include <dlfcn.h>

#ifdef USE_VALGRIND
#include <valgrind/valgrind.h>
#endif

#include "fiber/lib_fiber.h"
#include "event_epoll.h"  /* just for hook_epoll */
#include "fiber.h"

#define	MAX_CACHE	1000

typedef int  *(*errno_fn)(void);
typedef int   (*fcntl_fn)(int, int, ...);

static errno_fn __sys_errno     = NULL;
static fcntl_fn __sys_fcntl     = NULL;

typedef struct {
	ACL_RING       ready;		/* ready fiber queue */
	ACL_FIBER    **fibers;
	unsigned       size;
	unsigned       slot;
	int            exitcode;
	ACL_FIBER     *running;
	ACL_FIBER      original;
	int            errnum;
	unsigned       idgen;
	int            count;
	int            switched;
	int            nlocal;
} FIBER_TLS;

static void fiber_init(void) __attribute__ ((constructor));

static FIBER_TLS *__main_fiber = NULL;
static __thread FIBER_TLS *__thread_fiber = NULL;
__thread int acl_var_hook_sys_api = 0;

static acl_pthread_key_t __fiber_key;

/* forward declare */
static ACL_FIBER *fiber_alloc(void (*fn)(ACL_FIBER *, void *),
	void *arg, size_t size);

void acl_fiber_hook_api(int onoff)
{
	acl_var_hook_sys_api = onoff;
}

static void thread_free(void *ctx)
{
	FIBER_TLS *tf = (FIBER_TLS *) ctx;

	if (__thread_fiber == NULL)
		return;

	if (tf->fibers)
		acl_myfree(tf->fibers);
	if (tf->original.context)
		acl_myfree(tf->original.context);
	acl_myfree(tf);

	if (__main_fiber == __thread_fiber)
		__main_fiber = NULL;
	__thread_fiber = NULL;
}

static void fiber_schedule_main_free(void)
{
	if (__main_fiber) {
		thread_free(__main_fiber);
		if (__thread_fiber == __main_fiber)
			__thread_fiber = NULL;
		__main_fiber = NULL;
	}
}

static void thread_init(void)
{
	if (acl_pthread_key_create(&__fiber_key, thread_free) != 0)
		acl_msg_fatal("%s(%d), %s: pthread_key_create error %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
}

static acl_pthread_once_t __once_control = ACL_PTHREAD_ONCE_INIT;

static void fiber_check(void)
{
	if (__thread_fiber != NULL)
		return;

	if (acl_pthread_once(&__once_control, thread_init) != 0)
		acl_msg_fatal("%s(%d), %s: pthread_once error %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());

	__thread_fiber = (FIBER_TLS *) acl_mycalloc(1, sizeof(FIBER_TLS));
	__thread_fiber->original.context = (ucontext_t *)
		acl_mycalloc(1, sizeof(ucontext_t));
	__thread_fiber->fibers = NULL;
	__thread_fiber->size   = 0;
	__thread_fiber->slot   = 0;
	__thread_fiber->idgen  = 0;
	__thread_fiber->count  = 0;
	__thread_fiber->nlocal = 0;

	acl_ring_init(&__thread_fiber->ready);

	if ((unsigned long) acl_pthread_self() == acl_main_thread_self()) {
		__main_fiber = __thread_fiber;
		atexit(fiber_schedule_main_free);
	} else if (acl_pthread_setspecific(__fiber_key, __thread_fiber) != 0)
		acl_msg_fatal("acl_pthread_setspecific error!");
}

/* see /usr/include/bits/errno.h for __errno_location */
#ifdef ACL_ARM_LINUX
volatile int*   __errno(void)
#else
int *__errno_location(void)
#endif
{
	if (!acl_var_hook_sys_api) {
		if (__sys_errno == NULL)
			fiber_init();

		return __sys_errno();
	}

	if (__thread_fiber == NULL)
		fiber_check();

	if (__thread_fiber->running)
		return &__thread_fiber->running->errnum;
	else
		return &__thread_fiber->original.errnum;
}

int fcntl(int fd, int cmd, ...)
{
	long arg;
	struct flock *lock;
	va_list ap;
	int ret;

	if (__sys_fcntl == NULL)
		fiber_init();

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
		acl_msg_error("%s(%d), %s: unknown cmd: %d, fd: %d",
			__FILE__, __LINE__, __FUNCTION__, cmd, fd);
		break;
	}

	va_end(ap);

	if (ret < 0)
		fiber_save_errno();

	return ret;
}

void acl_fiber_set_errno(ACL_FIBER *fiber, int errnum)
{
	if (fiber == NULL)
		fiber = acl_fiber_running();
	if (fiber)
		fiber->errnum = errnum;
}

int acl_fiber_errno(ACL_FIBER *fiber)
{
	if (fiber == NULL)
		fiber = acl_fiber_running();
	return fiber ? fiber->errnum : 0;
}

void acl_fiber_keep_errno(ACL_FIBER *fiber, int yesno)
{
	if (fiber == NULL)
		fiber = acl_fiber_running();
	if (fiber) {
		if (yesno)
			fiber->flag |= FIBER_F_SAVE_ERRNO;
		else
			fiber->flag &= ~FIBER_F_SAVE_ERRNO;
	}
}

void fiber_save_errno(void)
{
	ACL_FIBER *curr;

	if (__thread_fiber == NULL)
		fiber_check();

	if ((curr = __thread_fiber->running) == NULL)
		curr = &__thread_fiber->original;

	if (curr->flag & FIBER_F_SAVE_ERRNO) {
		//curr->flag &= ~FIBER_F_SAVE_ERRNO;
		return;
	}

	if (__sys_errno != NULL)
		acl_fiber_set_errno(curr, *__sys_errno());
	else
		acl_fiber_set_errno(curr, errno);
}

static void fiber_swap(ACL_FIBER *from, ACL_FIBER *to)
{
	if (swapcontext(from->context, to->context) < 0)
		acl_msg_fatal("%s(%d), %s: swapcontext error %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
}

ACL_FIBER *acl_fiber_running(void)
{
	fiber_check();
	return __thread_fiber->running;
}

void acl_fiber_kill(ACL_FIBER *fiber)
{
	acl_fiber_signal(fiber, SIGKILL);
}

int acl_fiber_killed(ACL_FIBER *fiber)
{
	if (!fiber)
		fiber = acl_fiber_running();
	return fiber && (fiber->flag & FIBER_F_KILLED);
}

void acl_fiber_signal(ACL_FIBER *fiber, int signum)
{
	ACL_FIBER *curr = __thread_fiber->running;

	if (fiber == NULL) {
		acl_msg_error("%s(%d), %s: fiber NULL",
			__FILE__, __LINE__, __FUNCTION__);
		return;
	}

	if (curr == NULL) {
		acl_msg_error("%s(%d), %s: current fiber NULL",
			__FILE__, __LINE__, __FUNCTION__);
		return;
	}

	if (signum == SIGKILL || signum == SIGTERM || signum == SIGQUIT) {
		fiber->errnum = ECANCELED;
		fiber->flag |= FIBER_F_KILLED;
	}

	fiber->signum = signum;

	if (fiber == curr) // just return if kill myself
		return;

	acl_ring_detach(&curr->me);
	acl_ring_detach(&fiber->me);

	/* add the current fiber and signed fiber in the head of the ready */
#if 0
	acl_fiber_ready(fiber);
	acl_fiber_yield();
#else
	curr->status = FIBER_STATUS_READY;
	acl_ring_append(&__thread_fiber->ready, &curr->me);

	fiber->status = FIBER_STATUS_READY;
	acl_ring_append(&__thread_fiber->ready, &fiber->me);

	acl_fiber_switch();
#endif
}

int acl_fiber_signum(ACL_FIBER *fiber)
{
	if (fiber)
		fiber = acl_fiber_running();
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
		acl_ring_prepend(&__thread_fiber->ready, &fiber->me);
	}
}

int acl_fiber_yield(void)
{
	int  n;

	if (acl_ring_size(&__thread_fiber->ready) == 0)
		return 0;

	n = __thread_fiber->switched;
	acl_fiber_ready(__thread_fiber->running);
	acl_fiber_switch();

	return __thread_fiber->switched - n - 1;
}

union cc_arg
{
	void *p;
	int   i[2];
};

static void fiber_start(unsigned int x, unsigned int y)
{
	union  cc_arg arg;
	ACL_FIBER *fiber;
	int i;

	arg.i[0] = x;
	arg.i[1] = y;
	
	fiber = (ACL_FIBER *) arg.p;

	fiber->fn(fiber, fiber->arg);

	for (i = 0; i < fiber->nlocal; i++) {
		if (fiber->locals[i] == NULL)
			continue;
		if (fiber->locals[i]->free_fn)
			fiber->locals[i]->free_fn(fiber->locals[i]->ctx);
		acl_myfree(fiber->locals[i]);
	}

	if (fiber->locals) {
		acl_myfree(fiber->locals);
		fiber->locals = NULL;
		fiber->nlocal = 0;
	}

	fiber_exit(0);
}

void fiber_free(ACL_FIBER *fiber)
{
#ifdef USE_VALGRIND
	VALGRIND_STACK_DEREGISTER(fiber->vid);
#endif
	if (fiber->context)
		acl_myfree(fiber->context);
	acl_myfree(fiber->buff);
	acl_myfree(fiber);
}

static ACL_FIBER *fiber_alloc(void (*fn)(ACL_FIBER *, void *),
	void *arg, size_t size)
{
	ACL_FIBER *fiber;
	sigset_t zero;
	union cc_arg carg;

	fiber_check();

#define	APPL	ACL_RING_TO_APPL

	fiber = (ACL_FIBER *) acl_mycalloc(1, sizeof(ACL_FIBER));
	/* no using calloc just avoiding using real memory */
	fiber->buff = (char *) acl_mymalloc(size);

	fiber->errnum = 0;
	fiber->signum = 0;
	fiber->fn     = fn;
	fiber->arg    = arg;
	fiber->size   = size;
	__thread_fiber->idgen++;
	if (__thread_fiber->idgen == 0)  /* overflow ? */
		__thread_fiber->idgen++;
	fiber->id     = __thread_fiber->idgen;
	fiber->flag   = 0;
	fiber->status = FIBER_STATUS_READY;

	carg.p = fiber;

	if (fiber->context == NULL)
		fiber->context = (ucontext_t *) acl_mymalloc(sizeof(ucontext_t));
	sigemptyset(&zero);
	sigprocmask(SIG_BLOCK, &zero, &fiber->context->uc_sigmask);

	if (getcontext(fiber->context) < 0)
		acl_msg_fatal("%s(%d), %s: getcontext error: %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());

	fiber->context->uc_stack.ss_sp   = fiber->buff + 8;
	fiber->context->uc_stack.ss_size = fiber->size - 64;

	fiber->context->uc_link = __thread_fiber->original.context;

#ifdef USE_VALGRIND
	/* avoding the valgrind's warning */
	fiber->vid = VALGRIND_STACK_REGISTER(fiber->context->uc_stack.ss_sp,
			(char*) fiber->context->uc_stack.ss_sp
			+ fiber->context->uc_stack.ss_size);
#endif
	makecontext(fiber->context, (void(*)(void)) fiber_start,
		2, carg.i[0], carg.i[1]);

	return fiber;
}

ACL_FIBER *acl_fiber_create(void (*fn)(ACL_FIBER *, void *),
	void *arg, size_t size)
{
	ACL_FIBER *fiber = fiber_alloc(fn, arg, size);

	__thread_fiber->count++;

	if (__thread_fiber->slot >= __thread_fiber->size) {
		__thread_fiber->size += 128;
		__thread_fiber->fibers = (ACL_FIBER **) acl_myrealloc(
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
	if (fiber == NULL)
		fiber = acl_fiber_running();
	return fiber ? fiber->status : 0;
}

static void fiber_init(void)
{
	static int __called = 0;

	if (__called != 0)
		return;

	__called++;

#ifdef ACL_ARM_LINUX
	__sys_errno   = (errno_fn) dlsym(RTLD_NEXT, "__errno");
#else
	__sys_errno   = (errno_fn) dlsym(RTLD_NEXT, "__errno_location");
#endif
	__sys_fcntl   = (fcntl_fn) dlsym(RTLD_NEXT, "fcntl");

	hook_io();
	hook_net();
	hook_epoll();
}

void acl_fiber_schedule(void)
{
	ACL_FIBER *fiber;
	ACL_RING *head;

	acl_fiber_hook_api(1);

	for (;;) {
		head = acl_ring_pop_head(&__thread_fiber->ready);
		if (head == NULL) {
			acl_msg_info("------- NO ACL_FIBER NOW --------");
			break;
		}

		fiber = ACL_RING_TO_APPL(head, ACL_FIBER, me);
		fiber->status = FIBER_STATUS_READY;

		__thread_fiber->running = fiber;
		__thread_fiber->switched++;

		fiber_swap(&__thread_fiber->original, fiber);
		__thread_fiber->running = NULL;

		if (fiber->status == FIBER_STATUS_EXITING) {
			size_t slot = fiber->slot;

			if (!fiber->sys)
				__thread_fiber->count--;

			__thread_fiber->fibers[slot] =
				__thread_fiber->fibers[--__thread_fiber->slot];
			__thread_fiber->fibers[slot]->slot = slot;

			fiber_free(fiber);
		}
	}

	acl_fiber_hook_api(0);
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
	fiber_swap(__thread_fiber->running, &__thread_fiber->original);
}

int acl_fiber_set_specific(int *key, void *ctx, void (*free_fn)(void *))
{
	FIBER_LOCAL *local;
	ACL_FIBER *curr;

	if (key == NULL) {
		acl_msg_error("%s(%d), %s: key NULL",
			__FILE__, __LINE__, __FUNCTION__);
		return -1;
	}

	if (__thread_fiber == NULL) {
		acl_msg_error("%s(%d), %s: __thread_fiber: NULL",
			__FILE__, __LINE__, __FUNCTION__);
		return -1;
	} else if (__thread_fiber->running == NULL) {
		acl_msg_error("%s(%d), %s: running: NULL",
			__FILE__, __LINE__, __FUNCTION__);
		return -1;
	} else
		curr = __thread_fiber->running;

	if (*key <= 0)
		*key = ++__thread_fiber->nlocal;
	else if (*key > __thread_fiber->nlocal) {
		acl_msg_error("%s(%d), %s: invalid key: %d > nlocal: %d",
			__FILE__, __LINE__, __FUNCTION__,
			*key, __thread_fiber->nlocal);
		return -1;
	}

	if (curr->nlocal < __thread_fiber->nlocal) {
		curr->nlocal = __thread_fiber->nlocal;
		curr->locals = (FIBER_LOCAL **) acl_myrealloc(curr->locals,
			curr->nlocal * sizeof(FIBER_LOCAL*));
	}

	local = (FIBER_LOCAL *) acl_mycalloc(1, sizeof(FIBER_LOCAL));
	local->ctx = ctx;
	local->free_fn = free_fn;
	curr->locals[*key - 1] = local;

	return *key;
}

void *acl_fiber_get_specific(int key)
{
	FIBER_LOCAL *local;
	ACL_FIBER *curr;

	if (key <= 0)
		return NULL;

	if (__thread_fiber == NULL) {
		acl_msg_error("%s(%d), %s: __thread_fiber NULL",
			__FILE__, __LINE__, __FUNCTION__);
		return NULL;
	} else if (__thread_fiber->running == NULL) {
		acl_msg_error("%s(%d), %s: running fiber NULL",
			__FILE__, __LINE__, __FUNCTION__);
		return NULL;
	} else
		curr = __thread_fiber->running;

	if (key > curr->nlocal)
		return NULL;

	local = curr->locals[key - 1];

	return local ? local->ctx : NULL;
}
