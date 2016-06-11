#include "stdafx.h"
#include "fiber/fiber_io.h"
#include "fiber/fiber_schedule.h"
#include "fiber.h"

typedef struct {
	ACL_RING queue;
	FIBER  **fibers;
	size_t   size;
	int      exitcode;
	FIBER   *running;
	FIBER    schedule;
	size_t   idgen;
	int      count;
	int      switched;
} FIBER_TLS;

static FIBER_TLS *__main_fiber = NULL;
static __thread FIBER_TLS *__thread_fiber = NULL;

static acl_pthread_key_t __fiber_key;

static void thread_free(void *ctx)
{
	FIBER_TLS *tf = (FIBER_TLS *) ctx;

	if (__thread_fiber == NULL)
		return;

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
	acl_ring_init(&__thread_fiber->queue);

	if ((unsigned long) acl_pthread_self() == acl_main_thread_self()) {
		__main_fiber = __thread_fiber;
		atexit(fiber_schedule_main_free);
	} else if (acl_pthread_setspecific(__fiber_key, __thread_fiber) != 0)
		acl_msg_fatal("acl_pthread_setspecific error!");
}

static void fiber_swap(FIBER *from, FIBER *to)
{
	if (swapcontext(&from->uctx, &to->uctx) < 0)
		acl_msg_fatal("%s(%d), %s: swapcontext error %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
}

FIBER *fiber_running(void)
{
	fiber_check();
	return __thread_fiber->running;
}

void fiber_exit(int exit_code)
{
	fiber_check();

	__thread_fiber->exitcode = exit_code;
	__thread_fiber->running->status = FIBER_STATUS_EXITING;

	fiber_switch();
}

static void fiber_start(unsigned int x, unsigned int y)
{
	FIBER *fiber;
	unsigned long z;

	z = x << 16;
	z <<= 16;
	z |= y;
	fiber = (FIBER *) z;

	fiber->fn(fiber, fiber->arg);
	fiber_exit(0);
}

static FIBER *fiber_alloc(void (*fn)(FIBER *, void *), void *arg, size_t size)
{
	FIBER *fiber;
	sigset_t zero;
	unsigned long z;
	unsigned int x, y;

	fiber_check();

	fiber        = (FIBER *) acl_mycalloc(1, sizeof(FIBER) + size);
	fiber->fn    = fn;
	fiber->arg   = arg;
	fiber->stack = fiber->buf;
	fiber->size  = size;
	fiber->id    = ++__thread_fiber->idgen;

	sigemptyset(&zero);
	sigprocmask(SIG_BLOCK, &zero, &fiber->uctx.uc_sigmask);

	if (getcontext(&fiber->uctx) < 0)
		acl_msg_fatal("%s(%d), %s: getcontext error: %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());

	fiber->uctx.uc_stack.ss_sp   = fiber->stack + 8;
	fiber->uctx.uc_stack.ss_size = fiber->size - 64;

	z = (unsigned long) fiber;
	y = z;
	z >>= 16;
	x = z >> 16;

	makecontext(&fiber->uctx, (void(*)(void)) fiber_start, 2, x, y);

	return fiber;
}

FIBER *fiber_create(void (*fn)(FIBER *, void *), void *arg, size_t size)
{
	FIBER *fiber = fiber_alloc(fn, arg, size);

	__thread_fiber->count++;
	if (__thread_fiber->size % 64 == 0)
		__thread_fiber->fibers = (FIBER **) acl_myrealloc(
			__thread_fiber->fibers, 
			(__thread_fiber->size + 64) * sizeof(FIBER *));

	fiber->slot = __thread_fiber->size;
	__thread_fiber->fibers[__thread_fiber->size++] = fiber;
	fiber_ready(fiber);

	return fiber;
}

void fiber_free(FIBER *fiber)
{
	acl_myfree(fiber);
}

int fiber_id(const FIBER *fiber)
{
	return fiber->id;
}

void fiber_init(void) __attribute__ ((constructor));

void fiber_init(void)
{
	static int __called = 0;

	if (__called != 0)
		return;

	__called++;
	fiber_io_hook();
	fiber_net_hook();
}

void fiber_schedule(void)
{
	FIBER *fiber;
	ACL_RING *head;

	for (;;) {
		head = acl_ring_pop_head(&__thread_fiber->queue);
		if (head == NULL) {
			printf("------- NO FIBER NOW --------\r\n");
			break;
		}

		fiber = ACL_RING_TO_APPL(head, FIBER, me);
		fiber->status = FIBER_STATUS_READY;

		__thread_fiber->running = fiber;
		__thread_fiber->switched++;

		fiber_swap(&__thread_fiber->schedule, fiber);
		__thread_fiber->running = NULL;

		if (fiber->status == FIBER_STATUS_EXITING) {
			size_t slot = fiber->slot;

			if (!fiber->sys)
				__thread_fiber->count--;

			__thread_fiber->fibers[slot] =
				__thread_fiber->fibers[--__thread_fiber->size];
			__thread_fiber->fibers[slot]->slot = slot;

			fiber_free(fiber);
		}
	}
}

void fiber_ready(FIBER *fiber)
{
	fiber->status = FIBER_STATUS_READY;
	acl_ring_prepend(&__thread_fiber->queue, &fiber->me);
}

int fiber_yield(void)
{
	int  n = __thread_fiber->switched;

	fiber_ready(__thread_fiber->running);
	fiber_switch();

	return __thread_fiber->switched - n - 1;
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

void fiber_switch(void)
{
	fiber_swap(__thread_fiber->running, &__thread_fiber->schedule);
}
