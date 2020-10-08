#include "stdafx.h"
#include "common.h"

#include "fiber.h"

#ifdef SYS_UNIX

# if defined(USE_BOOST_JMP)
#  include "boost_jmp.h"
# elif defined(USE_JMP_DEF)
#  define USE_JMP
#  include "unix_jmp.h"
# elif defined(USE_JMP_EXP)
#  define USE_JMP
#  include "exp_jmp.h"
# else
#  define SETJMP(ctx) sigsetjmp(ctx, 0)
#  define LONGJMP(ctx) siglongjmp(ctx, 1)
# endif

# ifdef USE_VALGRIND
#  include <valgrind/valgrind.h>
# endif

typedef struct FIBER_UNIX {
	ACL_FIBER fiber;
# ifdef USE_VALGRIND
	unsigned int vid;
# endif

# if	defined(USE_BOOST_JMP)
	fcontext_t fcontext;
	char      *stack;
# else
#  if	defined(USE_JMP_DEF)
#   if defined(__x86_64__)
	unsigned long long env[10];
#   else
	sigjmp_buf env;
#   endif
#  elif	defined(USE_JMP_EXP)
	label_t env;
#  endif
	ucontext_t *context;
# endif
	size_t size;
	char  *buff;
} FIBER_UNIX;

#if	defined(USE_BOOST_JMP)
typedef struct {
	FIBER_UNIX *from;
	FIBER_UNIX *to;
} s_jump_t;

static void swap_fcontext(FIBER_UNIX *from, FIBER_UNIX *to)
{
	s_jump_t jump, *jmp;
	transfer_t trans;
	
	jump.from  = from;
	jump.to    = to;
	trans      = jump_fcontext(to->fcontext, &jump);
	jmp        = (s_jump_t*) trans.data;
	jmp->from->fcontext = trans.fctx;
}
#endif

static void fiber_unix_swap(FIBER_UNIX *from, FIBER_UNIX *to)
{
#if	defined(USE_BOOST_JMP)
	swap_fcontext(from, to);
#elif	defined(USE_JMP)
	/* use setcontext() for the initial jump, as it allows us to set up
	 * a stack, but continue with longjmp() as it's much faster.
	 */
	if (SETJMP(&from->env) == 0) {
		/* context just be used once for set up a stack, which will
		 * be freed in fiber_start. The context in __thread_fiber
		 * was set NULL.
		 */
		if (to->context != NULL) {
			setcontext(to->context);
		} else {
			LONGJMP(&to->env);
		}
	}
#else
	if (swapcontext(from->context, to->context) < 0) {
		msg_fatal("%s(%d), %s: swapcontext error %s",
			__FILE__, __LINE__, __FUNCTION__, last_serror());
	}
#endif
}

static void fiber_unix_free(ACL_FIBER *fiber)
{
	FIBER_UNIX *fb = (FIBER_UNIX *) fiber;

#ifdef USE_VALGRIND
	VALGRIND_STACK_DEREGISTER(fb->vid);
#endif

#if	!defined(USE_BOOST_JMP)
	if (fb->context) {
		stack_free(fb->context);
	}
#endif
	stack_free(fb->buff);
	mem_free(fb);
}

#if	defined(USE_BOOST_JMP)
static void fiber_unix_start(transfer_t arg)
{
	s_jump_t *jmp = (s_jump_t*) arg.data;
	jmp->from->fcontext = arg.fctx;
	jmp->to->fiber.start_fn(&jmp->to->fiber);
}
#else
union cc_arg
{
	void *p;
	int   i[2];
};

static void fiber_unix_start(unsigned int x, unsigned int y)
{
	union  cc_arg arg;
	FIBER_UNIX *fb;

	arg.i[0] = x;
	arg.i[1] = y;

	fb = (FIBER_UNIX *)arg.p;

#ifdef	USE_JMP
	/* when using setjmp/longjmp, the context just be used only once */
	if (fb->context != NULL) {
		stack_free(fb->context);
		fb->context = NULL;
	}
#endif
	fb->fiber.start_fn(&fb->fiber);
}
#endif // USE_BOOST_JMP

static void fiber_unix_init(ACL_FIBER *fiber, size_t size)
{
	FIBER_UNIX *fb = (FIBER_UNIX *) fiber;
#if	!defined(USE_BOOST_JMP)
	union cc_arg carg;
	sigset_t zero;
#endif
	
	if (fb->size < size) {
		/* if using realloc, real memory will be used, when we first
		 * free and malloc again, then we'll just use virtual memory,
		 * because memcpy will be called in realloc.
		 */
		stack_free(fb->buff);
		fb->buff = (char *) stack_alloc(size);
		fb->size = size;
	}

#if	defined(USE_BOOST_JMP)
	fb->stack = fb->buff + fb->size;
	fb->fcontext = make_fcontext(fb->stack, fb->size,
		(void(*)(transfer_t)) fiber_unix_start);
#else
	carg.p = fiber;

	if (fb->context == NULL) {
		fb->context = (ucontext_t *) stack_alloc(sizeof(ucontext_t));
	}

	memset(fb->context, 0, sizeof(ucontext_t));

	sigemptyset(&zero);
	sigaddset(&zero, SIGPIPE);
	sigaddset(&zero, SIGSYS);
	sigaddset(&zero, SIGALRM);
	sigaddset(&zero, SIGURG);
	sigaddset(&zero, SIGWINCH);
	sigprocmask(SIG_BLOCK, &zero, &fb->context->uc_sigmask);

	if (getcontext(fb->context) < 0) {
		msg_fatal("%s(%d), %s: getcontext error: %s",
			__FILE__, __LINE__, __FUNCTION__, last_serror());
	}

	fb->context->uc_stack.ss_sp   = fb->buff + 8;
	fb->context->uc_stack.ss_size = fb->size - 64;
	fb->context->uc_link = NULL;
	makecontext(fb->context, (void(*)(void)) fiber_unix_start,
		2, carg.i[0], carg.i[1]);
#endif

#ifdef USE_VALGRIND
	/* avoid the valgrind warning */
# if	defined(USE_BOOST_JMP)
	fb->vid = VALGRIND_STACK_REGISTER(fb->buff, fb->stack);
# else
	fb->vid = VALGRIND_STACK_REGISTER(fb->context->uc_stack.ss_sp,
		(char*)fb->context->uc_stack.ss_sp
		+ fb->context->uc_stack.ss_size);
# endif
#endif
}

ACL_FIBER *fiber_unix_alloc(void (*start_fn)(ACL_FIBER *), size_t size)
{
	FIBER_UNIX *fb = (FIBER_UNIX *) mem_calloc(1, sizeof(*fb));

	/* no using calloc just avoiding using real memory */
	fb->buff           = (char *) stack_alloc(size);
	fb->size           = size;
	fb->fiber.init_fn  = fiber_unix_init;
	fb->fiber.free_fn  = fiber_unix_free;
	fb->fiber.swap_fn  = (void (*)(ACL_FIBER*, ACL_FIBER*))fiber_unix_swap;
	fb->fiber.start_fn = start_fn;

	return (ACL_FIBER *) fb;
}

ACL_FIBER *fiber_unix_origin(void)
{
	FIBER_UNIX *fb = (FIBER_UNIX *) mem_calloc(1, sizeof(*fb));

#ifdef	USE_BOOST_JMP
	fb->size           = 32 * 1024;
	fb->buff           = (char *) stack_alloc(fb->size);
	fb->fiber.init_fn  = fiber_unix_init;
	fb->fiber.free_fn  = fiber_unix_free;
	fb->fiber.swap_fn  = (void (*)(ACL_FIBER*, ACL_FIBER*))fiber_unix_swap;
	fb->fiber.start_fn = NULL;

#elif	defined(USE_JMP)
	/* set context NULL when using setjmp that setcontext will not be
	 * called in fiber_swap.
	 */
	fb->context = NULL;
#else
	fb->context = (ucontext_t *) stack_calloc(sizeof(ucontext_t));
#endif
	fb->fiber.free_fn = fiber_unix_free;
	fb->fiber.swap_fn = (void (*)(ACL_FIBER*, ACL_FIBER*)) fiber_unix_swap;

	return &fb->fiber;
}

#endif
