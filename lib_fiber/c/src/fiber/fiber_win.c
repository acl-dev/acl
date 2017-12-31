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

static void fiber_win_init(ACL_FIBER *fiber, size_t size)
{

}

static void fiber_win_swap(ACL_FIBER *from, ACL_FIBER *to)
{
	FIBER_WIN *fb_to = (FIBER_WIN *) to;
	SwitchToFiber(fb_to->context);
}

static void __stdcall fiber_win_start(LPVOID ctx)
{
	FIBER_WIN *fb = (FIBER_WIN *) ctx;
	fb->fiber.start_fn(&fb->fiber);
}

ACL_FIBER *fiber_win_alloc(void(*start_fn)(ACL_FIBER *), size_t size)
{
	FIBER_WIN *fb = (FIBER_WIN *) calloc(1, sizeof(*fb));

	fb->fiber.init_fn = fiber_win_init;
	fb->fiber.free_fn = fiber_win_free;
	fb->fiber.swap_fn = (void(*)(ACL_FIBER*, ACL_FIBER*)) fiber_win_swap;
	fb->fiber.start_fn = start_fn;

	fb->context = CreateFiberEx(size, 0, FIBER_FLAG_FLOAT_SWITCH,
		fiber_win_start, fb);
	return (ACL_FIBER *) fb;
}

ACL_FIBER *fiber_win_origin(void)
{
	FIBER_WIN *fb = (FIBER_WIN *) calloc(1, sizeof(*fb));

	fb->context = ConvertThreadToFiberEx(NULL, FIBER_FLAG_FLOAT_SWITCH);
	fb->fiber.free_fn = fiber_win_free;
	fb->fiber.swap_fn = (void(*)(ACL_FIBER*, ACL_FIBER*)) fiber_win_swap;
	return (ACL_FIBER *) fb;
}

#endif
