#include "stdafx.h"
#include "common.h"
#include "fiber.h"

#ifdef SYS_WIN

typedef struct FIBER_WIN {
	ACL_FIBER fiber;
	LPVOID context;
} FIBER_WIN;

static void fiber_win_free(ACL_FIBER *fiber)
{
	FIBER_WIN *fb = (FIBER_WIN *) fiber;
	DeleteFiber(fb->context);
	stack_free(fb);
}

static void fiber_win_swap(ACL_FIBER *from, ACL_FIBER *to)
{
	FIBER_WIN *fb_to = (FIBER_WIN *) to;
	SwitchToFiber(fb_to->context);
}

static void WINAPI fiber_win_start(LPVOID ctx)
{
	FIBER_WIN *fb = (FIBER_WIN *) ctx;
	fb->fiber.start_fn(&fb->fiber);
}

static void fiber_win_init(FIBER_WIN *fb, size_t size)
{
	if (fb->context) {
		DeleteFiber(fb->context);
	}
	fb->context = CreateFiberEx(0, size, FIBER_FLAG_FLOAT_SWITCH,
		fiber_win_start, fb);
	if (fb->context == NULL) {
		int e = acl_fiber_last_error();
		msg_fatal("%s: CreateFiberEx error=%s, %d", last_serror(), e);
	}
}

ACL_FIBER *fiber_win_alloc(void (*start_fn)(ACL_FIBER *), size_t size)
{
	FIBER_WIN *fb = (FIBER_WIN *) mem_calloc(1, sizeof(*fb));

	fb->fiber.init_fn  = (void (*)(ACL_FIBER*, size_t)) fiber_win_init;
	fb->fiber.free_fn  = fiber_win_free;
	fb->fiber.swap_fn  = (void (*)(ACL_FIBER*, ACL_FIBER*)) fiber_win_swap;
	fb->fiber.start_fn = start_fn;
	fb->context        = NULL;

	return (ACL_FIBER *) fb;
}

ACL_FIBER *fiber_win_origin(void)
{
	FIBER_WIN *fb = (FIBER_WIN *) mem_calloc(1, sizeof(*fb));

	fb->context = ConvertThreadToFiberEx(NULL, FIBER_FLAG_FLOAT_SWITCH);
	fb->fiber.free_fn = fiber_win_free;
	fb->fiber.swap_fn = (void(*)(ACL_FIBER*, ACL_FIBER*)) fiber_win_swap;

	return (ACL_FIBER *) fb;
}

#endif
