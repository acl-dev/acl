#include "stdafx.h"
#include "fiber_io.h"
#include "fiber_schedule.h"

static ACL_RING __fibers_queue;
static FIBER  **__fibers = NULL;
static size_t   __fibers_size = 0;
static int      __fiber_exitcode = 0;
static FIBER   *__fiber_running = NULL;
static FIBER    __fiber_schedule;
static size_t   __fiber_idgen = 0;
static int      __fiber_count = 0;
static int      __fiber_switched = 0;

static void fiber_swap(FIBER *from, FIBER *to)
{
	if (swapcontext(&from->uctx, &to->uctx) < 0)
		acl_msg_fatal("%s(%d), %s: swapcontext error %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
}

FIBER *fiber_running(void)
{
	return __fiber_running;
}

void fiber_exit(int exit_code)
{
	__fiber_exitcode = exit_code;
	__fiber_running->status = FIBER_STATUS_EXITING;
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

	fiber->fun(fiber->arg);
	fiber_exit(0);
}

static FIBER *fiber_alloc(void (*fun)(void *), void *arg, size_t size)
{
	FIBER *fiber = (FIBER *) acl_mycalloc(1, sizeof(FIBER) + size);
	sigset_t zero;
	unsigned long z;
	unsigned int x, y;

	fiber->fun   = fun;
	fiber->arg   = arg;
	fiber->stack = fiber->buf;
	fiber->size  = size;
	fiber->id    = ++__fiber_idgen;

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

int fiber_create(void (*fun)(void *), void *arg, size_t size)
{
	FIBER *fiber = fiber_alloc(fun, arg, size);

	__fiber_count++;
	if (__fibers_size % 64 == 0)
		__fibers = (FIBER **) acl_myrealloc(__fibers, 
				(__fibers_size + 64) * sizeof(FIBER *));

	fiber->slot = __fibers_size;
	__fibers[__fibers_size++] = fiber;
	fiber_ready(fiber);

	return fiber->id;
}

void fiber_free(FIBER *fiber)
{
	acl_myfree(fiber);
}

//void fiber_init(void) __attribute__ ((constructor));

void fiber_init(void)
{
	printf("hook ok\n");
	acl_ring_init(&__fibers_queue);
	fiber_io_hook();
}

void fiber_schedule(void)
{
	FIBER *fiber;
	ACL_RING *head;

	for (;;) {
		printf("fibers's size: %d\r\n", (int) acl_ring_size(&__fibers_queue));
		head = acl_ring_pop_head(&__fibers_queue);
		if (head == NULL) {
			printf("no fiber now\r\n");
			break;
		}

		printf(">>>one fiber ok\r\n");
		fiber = ACL_RING_TO_APPL(head, FIBER, me);
		fiber->status = FIBER_STATUS_READY;

		__fiber_running = fiber;
		__fiber_switched++;

		fiber_swap(&__fiber_schedule, fiber);
		__fiber_running = NULL;

		if (fiber->status == FIBER_STATUS_EXITING) {
			size_t slot = fiber->slot;

			if (!fiber->sys)
				__fiber_count--;

			__fibers[slot] = __fibers[--__fibers_size];
			__fibers[slot]->slot = slot;
			fiber_free(fiber);
		}
	}
}

static void fiber_add(FIBER *fiber)
{
	acl_ring_prepend(&__fibers_queue, &fiber->me);
}

void fiber_ready(FIBER *fiber)
{
	fiber->status = FIBER_STATUS_READY;
	fiber_add(fiber);
}

int fiber_yield(void)
{
	int  n = __fiber_switched;

	fiber_ready(__fiber_running);
	fiber_switch();

	return __fiber_switched - n - 1;
}

void fiber_system(void)
{
	if (!__fiber_running->sys) {
		__fiber_running->sys = 1;
		__fiber_count--;
	}
}

void fiber_switch(void)
{
	fiber_swap(__fiber_running, &__fiber_schedule);
}
