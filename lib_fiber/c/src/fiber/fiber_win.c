#include "stdafx.h"
#include "common.h"
#include "fiber.h"

#ifdef SYS_WIN

// see <sdkddkver.h>
// see https://docs.microsoft.com/en-us/cpp/porting/modifying-winver-and-win32-winnt?redirectedfrom=MSDN&view=msvc-170

#ifdef _WIN32_WINNT
# if defined(_WIN32_WINNT_WS08)
#  if _WIN32_WINNT >= _WIN32_WINNT_WS08
#   define HAS_FIBER_EX
#  endif
# elif defined(_WIN32_WINNT_VISTA)
#  if _WIN32_WINNT >= _WIN32_WINNT_VISTA
#   define HAS_FIBER_EX
#  endif
# endif
#endif

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

#ifdef HAS_FIBER_EX
	fb->context = CreateFiberEx(0, size, FIBER_FLAG_FLOAT_SWITCH,
		fiber_win_start, fb);
#else
	fb->context = CreateFiber(size, fiber_win_start, fb);
#endif

	if (fb->context == NULL) {
		int e = acl_fiber_last_error();
		msg_fatal("%s: CreateFiberEx error=%s, %d", last_serror(), e);
	}
}

ACL_FIBER *fiber_win_alloc(void (*start_fn)(ACL_FIBER *),
		const ACL_FIBER_ATTR *attr)
{
	FIBER_WIN *fb = (FIBER_WIN *) mem_calloc(1, sizeof(*fb));
	size_t size = attr ? attr->stack_size : 128000;

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

#ifdef HAS_FIBER_EX
	fb->context = ConvertThreadToFiberEx(NULL, FIBER_FLAG_FLOAT_SWITCH);
#else
	fb->context = ConvertThreadToFiber(NULL);
#endif

	fb->fiber.free_fn = fiber_win_free;
	fb->fiber.swap_fn = (void(*)(ACL_FIBER*, ACL_FIBER*)) fiber_win_swap;

	return (ACL_FIBER *) fb;
}

#endif
