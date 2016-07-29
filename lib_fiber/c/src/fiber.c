#include "stdafx.h"
#define __USE_GNU
#include <dlfcn.h>

#ifdef USE_VALGRIND
#include <valgrind/valgrind.h>
#endif

#include "fiber/lib_fiber.h"
#include "event_epoll.h"  /* just for hook_epoll */
#include "fiber.h"

typedef int *(*errno_fn)(void);
typedef int  (*fcntl_fn)(int, int, ...);

static errno_fn __sys_errno = NULL;
static fcntl_fn __sys_fcntl = NULL;

typedef struct {
	ACL_RING       queue;		/* ready fiber queue */
	ACL_RING       dead;		/* dead fiber queue */
	ACL_FIBER    **fibers;
	size_t         size;
	int            exitcode;
	ACL_FIBER     *running;
	ACL_FIBER      schedule;
	int            errnum;
	size_t         idgen;
	int            count;
	int            switched;
#ifdef	USE_DBUF
	ACL_DBUF_POOL *dbuf;
#endif
} FIBER_TLS;

static FIBER_TLS *__main_fiber = NULL;
static __thread FIBER_TLS *__thread_fiber = NULL;
__thread int acl_var_hook_sys_api = 0;

static acl_pthread_key_t __fiber_key;

void acl_fiber_hook_api(int onoff)
{
	acl_var_hook_sys_api = onoff;
}

static void thread_free(void *ctx)
{
	FIBER_TLS *tf = (FIBER_TLS *) ctx;

	if (__thread_fiber == NULL)
		return;

#ifdef	USE_DBUF
	acl_dbuf_pool_destroy(tf->dbuf);
#endif

	if (tf->fibers)
		acl_myfree(tf->fibers);
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
	acl_assert(acl_pthread_key_create(&__fiber_key, thread_free) == 0);
}

static acl_pthread_once_t __once_control = ACL_PTHREAD_ONCE_INIT;

static void fiber_check(void)
{
	if (__thread_fiber != NULL)
		return;

	acl_assert(acl_pthread_once(&__once_control, thread_init) == 0);

	__thread_fiber = (FIBER_TLS *) acl_mycalloc(1, sizeof(FIBER_TLS));
	__thread_fiber->fibers = NULL;
	__thread_fiber->size   = 0;
	__thread_fiber->idgen  = 0;
	__thread_fiber->count  = 0;
#ifdef	USE_DBUF
	__thread_fiber->dbuf   = acl_dbuf_pool_create(640000);
#endif

	acl_ring_init(&__thread_fiber->queue);
	acl_ring_init(&__thread_fiber->dead);

	if ((unsigned long) acl_pthread_self() == acl_main_thread_self()) {
		__main_fiber = __thread_fiber;
		atexit(fiber_schedule_main_free);
	} else if (acl_pthread_setspecific(__fiber_key, __thread_fiber) != 0)
		acl_msg_fatal("acl_pthread_setspecific error!");
}

/* see /usr/include/bits/errno.h for __errno_location */
int *__errno_location(void)
{
	if (!acl_var_hook_sys_api)
		return __sys_errno();

	if (__thread_fiber == NULL)
		fiber_check();

	if (__thread_fiber->running)
		return &__thread_fiber->running->errnum;
	else
		return &__thread_fiber->schedule.errnum;
}

int fcntl(int fd, int cmd, ...)
{
	long arg;
	struct flock *lock;
	va_list ap;
	int ret;

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
	fiber->errnum = errnum;
}

int acl_fiber_errno(ACL_FIBER *fiber)
{
	fiber->flag |= FIBER_F_SAVE_ERRNO;
	return fiber->errnum;
}

void fiber_save_errno(void)
{
	ACL_FIBER *curr;

	if (__thread_fiber == NULL)
		fiber_check();

	if ((curr = __thread_fiber->running) == NULL)
		curr = &__thread_fiber->schedule;

	if (curr->flag & FIBER_F_SAVE_ERRNO) {
		curr->flag &= ~FIBER_F_SAVE_ERRNO;
		return;
	}

	if (__sys_errno != NULL)
		curr->errnum = *__sys_errno();
	else
		curr->errnum = errno;
}

static void fiber_swap(ACL_FIBER *from, ACL_FIBER *to)
{
	if (swapcontext(&from->uctx, &to->uctx) < 0)
		acl_msg_fatal("%s(%d), %s: swapcontext error %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
}

ACL_FIBER *fiber_running(void)
{
	fiber_check();
	return __thread_fiber->running;
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
	fiber->status = FIBER_STATUS_READY;
	acl_ring_prepend(&__thread_fiber->queue, &fiber->me);
}

int acl_fiber_yield(void)
{
	int  n;

	if (acl_ring_size(&__thread_fiber->queue) == 0)
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
		if (fiber->locals[i]->free_fn)
			fiber->locals[i]->free_fn(fiber->locals[i]->ctx);
	}

	if (fiber->locals) {
		acl_myfree(fiber->locals);
		fiber->locals = NULL;
		fiber->nlocal = 0;
	}

	fiber_exit(0);
}

#define	MAX_CACHE	100

static void fiber_free(FIBER_TLS *tls, ACL_FIBER *fiber)
{
#ifdef	USE_DBUF
	acl_dbuf_pool_free(tls->dbuf, fiber);
#else
	(void) tls;
	acl_myfree(fiber);
#endif
}

static ACL_FIBER *fiber_alloc(void (*fn)(ACL_FIBER *, void *),
	void *arg, size_t size)
{
	ACL_FIBER *fiber;
	sigset_t zero;
	union cc_arg carg;
	ACL_RING *head;
	size_t n;

	fiber_check();

	n = acl_ring_size(&__thread_fiber->dead);
	if (n > MAX_CACHE) {
		n -= MAX_CACHE;
		while (n > 0) {
			head = acl_ring_pop_head(&__thread_fiber->dead);
			acl_assert(head != NULL);
			fiber = ACL_RING_TO_APPL(head, ACL_FIBER,me);
			fiber_free(__thread_fiber, fiber);
			n--;
		}
	}

	head = acl_ring_pop_head(&__thread_fiber->dead);
	if (head == NULL)
#ifdef	USE_DBUF
		fiber = (ACL_FIBER *) acl_dbuf_pool_calloc(
			__thread_fiber->dbuf, sizeof(ACL_FIBER) + size);
#else
		fiber = (ACL_FIBER *) acl_mycalloc(1, sizeof(ACL_FIBER) + size);
#endif
	else if ((fiber = ACL_RING_TO_APPL(head, ACL_FIBER, me))->size < size) {
		fiber_free(__thread_fiber, fiber);
#ifdef	USE_DBUF
		fiber = (ACL_FIBER *) acl_dbuf_pool_calloc(
			__thread_fiber->dbuf, sizeof(ACL_FIBER) + size);
#else
		fiber = (ACL_FIBER *) acl_mycalloc(1, sizeof(ACL_FIBER) + size);
#endif
	} else
		size = fiber->size;

	fiber->errnum = 0;
	fiber->fn     = fn;
	fiber->arg    = arg;
	fiber->stack  = fiber->buf;
	fiber->size   = size;
	fiber->id     = ++__thread_fiber->idgen;

	sigemptyset(&zero);
	sigprocmask(SIG_BLOCK, &zero, &fiber->uctx.uc_sigmask);

	if (getcontext(&fiber->uctx) < 0)
		acl_msg_fatal("%s(%d), %s: getcontext error: %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());

	fiber->uctx.uc_stack.ss_sp   = fiber->stack + 8;
	fiber->uctx.uc_stack.ss_size = fiber->size - 64;
	fiber->uctx.uc_link = &__thread_fiber->schedule.uctx;

#ifdef USE_VALGRIND
	fiber->vid = VALGRIND_STACK_REGISTER(fiber->uctx.uc_stack.ss_sp,
			fiber->uctx.uc_stack.ss_sp
			+ fiber->uctx.uc_stack.ss_size);
#endif

	carg.p = fiber;
	makecontext(&fiber->uctx, (void(*)(void)) fiber_start,
		2, carg.i[0], carg.i[1]);

	return fiber;
}

ACL_FIBER *acl_fiber_create(void (*fn)(ACL_FIBER *, void *), void *arg, size_t size)
{
	ACL_FIBER *fiber = fiber_alloc(fn, arg, size);

	__thread_fiber->count++;
	if (__thread_fiber->size % 64 == 0)
		__thread_fiber->fibers = (ACL_FIBER **) acl_myrealloc(
			__thread_fiber->fibers, 
			(__thread_fiber->size + 64) * sizeof(ACL_FIBER *));

	fiber->slot = __thread_fiber->size;
	__thread_fiber->fibers[__thread_fiber->size++] = fiber;
	acl_fiber_ready(fiber);

	return fiber;
}

int acl_fiber_id(const ACL_FIBER *fiber)
{
	return fiber->id;
}

int acl_fiber_self(void)
{
	ACL_FIBER *curr = fiber_running();
	return acl_fiber_id(curr);
}

int acl_fiber_status(const ACL_FIBER *fiber)
{
	return fiber->status;
}

static void fiber_init(void) __attribute__ ((constructor));

static void fiber_init(void)
{
	static int __called = 0;

	if (__called != 0)
		return;

	__called++;

	__sys_errno = (errno_fn) dlsym(RTLD_NEXT, "__errno_location");
	__sys_fcntl = (fcntl_fn) dlsym(RTLD_NEXT, "fcntl");

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
		head = acl_ring_pop_head(&__thread_fiber->queue);
		if (head == NULL) {
			acl_msg_info("------- NO ACL_FIBER NOW --------");
			break;
		}

		fiber = ACL_RING_TO_APPL(head, ACL_FIBER, me);
		fiber->status = FIBER_STATUS_READY;

		__thread_fiber->running = fiber;
		__thread_fiber->switched++;

		fiber_swap(&__thread_fiber->schedule, fiber);
		__thread_fiber->running = NULL;
	}

	/* release dead fiber */
	while ((head = acl_ring_pop_head(&__thread_fiber->dead)) != NULL) {
		fiber = ACL_RING_TO_APPL(head, ACL_FIBER, me);
		fiber_free(__thread_fiber, fiber);
	}
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
	ACL_RING *head;

#ifdef _DEBUG
	acl_assert(current);
#endif

	if (current->status == FIBER_STATUS_EXITING) {
		size_t slot = current->slot;

		if (!current->sys)
			__thread_fiber->count--;

		__thread_fiber->fibers[slot] =
			__thread_fiber->fibers[--__thread_fiber->size];
		__thread_fiber->fibers[slot]->slot = slot;
		acl_ring_append(&__thread_fiber->dead, &current->me);
	}

	head = acl_ring_pop_head(&__thread_fiber->queue);

	if (head == NULL) {
		fiber_swap(current, &__thread_fiber->schedule);
		return;
	}

	fiber = ACL_RING_TO_APPL(head, ACL_FIBER, me);
	fiber->status = FIBER_STATUS_READY;

	__thread_fiber->running = fiber;
	__thread_fiber->switched++;
	fiber_swap(current, __thread_fiber->running);
}

int acl_fiber_set_specific(void *ctx, void (*free_fn)(void *))
{
	FIBER_LOCAL *local;
	ACL_FIBER *curr;
	int key;

	if (__thread_fiber == NULL || __thread_fiber->running == NULL)
		return -1;

	curr = __thread_fiber->running;

	key = curr->nlocal;
	local = (FIBER_LOCAL *) acl_mymalloc(sizeof(FIBER_LOCAL));
	local->ctx = ctx;
	local->free_fn = free_fn;

	if (curr->nlocal % 64 == 0)
		curr->locals = (FIBER_LOCAL **) acl_myrealloc(curr->locals,
			(curr->nlocal + 64) * sizeof(FIBER_LOCAL*));
	curr->locals[curr->nlocal++] = local;

	return key;
}

void *acl_fiber_get_specific(int key)
{
	ACL_FIBER *curr;

	if (__thread_fiber == NULL || __thread_fiber->running == NULL)
		return NULL;

	curr = __thread_fiber->running;
	if (key >= curr->nlocal || key < 0)
		return NULL;

	return curr->locals[key];
}
