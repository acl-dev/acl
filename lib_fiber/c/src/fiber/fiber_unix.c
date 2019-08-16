#include "stdafx.h"
#include "common.h"
#include "fiber.h"

#ifdef SYS_UNIX

#ifdef USE_VALGRIND
#include <valgrind/valgrind.h>
#endif

typedef struct FIBER_UNIX {
	ACL_FIBER fiber;
#ifdef USE_VALGRIND
	unsigned int vid;
#endif

#ifdef	USE_JMP
# if defined(__x86_64__)
	unsigned long long env[10];
# else
	sigjmp_buf env;
# endif
#endif
	ucontext_t *context;
	size_t size;
	char  *buff;
} FIBER_UNIX;

#if defined(__x86_64__)
# if defined(__AVX__)
#  define CLOBBER \
	, "ymm0", "ymm1", "ymm2", "ymm3", "ymm4", "ymm5", "ymm6", "ymm7", \
	"ymm8", "ymm9", "ymm10", "ymm11", "ymm12", "ymm13", "ymm14", "ymm15"
# else
#  define CLOBBER
# endif

//	asm(".cfi_undefined rip;\r\n")
# define SETJMP(ctx) ({\
	int ret; \
	asm("lea     LJMPRET%=(%%rip), %%rcx\n\t"\
	"xor     %%rax, %%rax\n\t"\
	"mov     %%rbx, (%%rdx)\n\t"\
	"mov     %%rbp, 8(%%rdx)\n\t"\
	"mov     %%r12, 16(%%rdx)\n\t"\
	"mov     %%rsp, 24(%%rdx)\n\t"\
	"mov     %%r13, 32(%%rdx)\n\t"\
	"mov     %%r14, 40(%%rdx)\n\t"\
	"mov     %%r15, 48(%%rdx)\n\t"\
	"mov     %%rcx, 56(%%rdx)\n\t"\
	"mov     %%rdi, 64(%%rdx)\n\t"\
	"mov     %%rsi, 72(%%rdx)\n\t"\
	"LJMPRET%=:\n\t"\
	: "=a" (ret)\
	: "d" (ctx)\
	: "memory", "rcx", "r8", "r9", "r10", "r11", \
	"xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7", \
	"xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15"\
	CLOBBER\
	); \
	ret; \
})

# define LONGJMP(ctx) \
	asm("movq   (%%rax), %%rbx\n\t"\
	"movq   8(%%rax), %%rbp\n\t"\
	"movq   16(%%rax), %%r12\n\t"\
	"movq   24(%%rax), %%rdx\n\t"\
	"movq   32(%%rax), %%r13\n\t"\
	"movq   40(%%rax), %%r14\n\t"\
	"mov    %%rdx, %%rsp\n\t"\
	"movq   48(%%rax), %%r15\n\t"\
	"movq   56(%%rax), %%rdx\n\t"\
	"movq   64(%%rax), %%rdi\n\t"\
	"movq   72(%%rax), %%rsi\n\t"\
	"jmp    *%%rdx\n\t"\
	: : "a" (ctx) : "rdx" \
	)

#elif defined(__i386__)

# define SETJMP(ctx) ({\
	int ret; \
	asm("movl   $LJMPRET%=, %%eax\n\t"\
	"movl   %%eax, (%%edx)\n\t"\
	"movl   %%ebx, 4(%%edx)\n\t"\
	"movl   %%esi, 8(%%edx)\n\t"\
	"movl   %%edi, 12(%%edx)\n\t"\
	"movl   %%ebp, 16(%%edx)\n\t"\
	"movl   %%esp, 20(%%edx)\n\t"\
	"xorl   %%eax, %%eax\n\t"\
	"LJMPRET%=:\n\t"\
	: "=a" (ret) : "d" (ctx) : "memory"); \
	ret; \
	})

# define LONGJMP(ctx) \
	asm("movl   (%%eax), %%edx\n\t"\
	"movl   4(%%eax), %%ebx\n\t"\
	"movl   8(%%eax), %%esi\n\t"\
	"movl   12(%%eax), %%edi\n\t"\
	"movl   16(%%eax), %%ebp\n\t"\
	"movl   20(%%eax), %%esp\n\t"\
	"jmp    *%%edx\n\t"\
	: : "a" (ctx) : "edx" \
	)

#else

# define SETJMP(ctx) \
	sigsetjmp(ctx, 0)
# define LONGJMP(ctx) \
	siglongjmp(ctx, 1)
#endif

static void fiber_unix_swap(FIBER_UNIX *from, FIBER_UNIX *to)
{
#ifdef	USE_JMP
	/* use setcontext() for the initial jump, as it allows us to set up
	 * a stack, but continue with longjmp() as it's much faster.
	 */
	if (SETJMP(from->env) == 0) {
		/* context just be used once for set up a stack, which will
		 * be freed in fiber_start. The context in __thread_fiber
		 * was set NULL.
		 */
		if (to->context != NULL) {
			setcontext(to->context);
		}
		else {
			LONGJMP(to->env);
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
	if (fb->context) {
		stack_free(fb->context);
	}
	stack_free(fb->buff);
	mem_free(fb);
}

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

static void fiber_unix_init(ACL_FIBER *fiber, size_t size)
{
	FIBER_UNIX *fb = (FIBER_UNIX *) fiber;
	union cc_arg carg;
	sigset_t zero;
	
	if (fb->size < size) {
		/* if using realloc, real memory will be used, when we first
		 * free and malloc again, then we'll just use virtual memory,
		 * because memcpy will be called in realloc.
		 */
		stack_free(fb->buff);
		fb->buff = (char *) stack_alloc(size);
		fb->size = size;
	}

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

#ifdef	USE_JMP
	fb->context->uc_link = NULL;
#else
	fb->context->uc_link = __thread_fiber->original->context;
#endif

#ifdef USE_VALGRIND
	/* avoding the valgrind's warning */
	fb->vid = VALGRIND_STACK_REGISTER(fb->context->uc_stack.ss_sp,
		(char*)fb->context->uc_stack.ss_sp
		+ fb->context->uc_stack.ss_size);
#endif
	makecontext(fb->context, (void(*)(void)) fiber_unix_start,
		2, carg.i[0], carg.i[1]);
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

#ifdef	USE_JMP
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
