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
	void (*fn)(ACL_FIBER *, void *);
	void *arg;
} FIBER_WIN;

ACL_FIBER_STACK *acl_fiber_stacktrace(const ACL_FIBER *fiber, size_t max)
{
	printf("%s(%d): Not supported, fiber-%d, max=%zd\r\n",
		__FUNCTION__, __LINE__, acl_fiber_id(fiber), max);
	return NULL;
}

void acl_fiber_stackfree(ACL_FIBER_STACK *stack)
{
	size_t i;

	if (stack) {
		for (i = 0; i < stack->count; i++) {
			mem_free(stack->frames[i].func);
		}

		mem_free(stack->frames);
		mem_free(stack);
	}
}

void fiber_real_free(ACL_FIBER *fiber)
{
	FIBER_WIN *fb = (FIBER_WIN *) fiber;
	DeleteFiber(fb->context);
	stack_free(fb);
}

void fiber_real_swap(ACL_FIBER *from, ACL_FIBER *to)
{
	FIBER_WIN *fb_to = (FIBER_WIN *) to;
	SwitchToFiber(fb_to->context);
}

static void WINAPI fiber_win_start(LPVOID ctx)
{
	FIBER_WIN *fb = (FIBER_WIN *) ctx;
	fiber_start(&fb->fiber, fb->fn, fb->arg);
}

void fiber_real_init(ACL_FIBER *fiber, size_t size,
	void (*fn)(ACL_FIBER *, void *), void *arg)
{
	FIBER_WIN *fb = (FIBER_WIN*) fiber;

	if (fb->context) {
		DeleteFiber(fb->context);
	}

	fb->fn  = fn;
	fb->arg = arg;

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

ACL_FIBER *fiber_real_alloc(const ACL_FIBER_ATTR *attr)
{
	FIBER_WIN *fb = (FIBER_WIN *) mem_calloc(1, sizeof(*fb));
	size_t size = attr ? attr->stack_size : 128000;

	fb->context        = NULL;

	return (ACL_FIBER *) fb;
}

ACL_FIBER *fiber_real_origin(void)
{
	FIBER_WIN *fb = (FIBER_WIN *) mem_calloc(1, sizeof(*fb));

#ifdef HAS_FIBER_EX
	fb->context = ConvertThreadToFiberEx(NULL, FIBER_FLAG_FLOAT_SWITCH);
#else
	fb->context = ConvertThreadToFiber(NULL);
#endif

	fb->fiber.flag = FIBER_F_STARTED;

	return (ACL_FIBER *) fb;
}

#endif
