#include "stdafx.h"
#ifndef __USE_GNU
#define __USE_GNU
#endif
#include <dlfcn.h>
#include <signal.h>

#ifdef USE_VALGRIND
#include <valgrind/valgrind.h>
#endif

//#define FIBER_STACK_GUARD
#ifdef	FIBER_STACK_GUARD
#include <dlfcn.h>
#include <sys/mman.h>
#endif

#include "fiber/lib_fiber.h"
#include "event_epoll.h"  /* just for hook_epoll */
#include "fiber.h"

#define	MAX_CACHE	1000

typedef int  *(*errno_fn)(void);
typedef int   (*fcntl_fn)(int, int, ...);

static errno_fn __sys_errno     = NULL;
static fcntl_fn __sys_fcntl     = NULL;

typedef struct THREAD {
	ACL_RING       ready;		/* ready fiber queue */
	ACL_RING       dead;		/* dead fiber queue */
	ACL_FIBER    **fibers;
	unsigned       size;
	unsigned       slot;
	int            exitcode;
	ACL_FIBER     *running;
	ACL_FIBER      original;
	int            errnum;
	unsigned       idgen;
	int            count;
	size_t         switched;
	int            nlocal;
} THREAD;

static void fiber_init(void) __attribute__ ((constructor));

static THREAD *__main_fiber = NULL;
static __thread THREAD *__thread_fiber = NULL;
static __thread int __scheduled = 0;
__thread int acl_var_hook_sys_api = 0;

static acl_pthread_key_t __fiber_key;

#ifdef	FIBER_STACK_GUARD

static size_t page_size(void)
{
	static __thread long pgsz = 0;

	if (pgsz == 0) {
		pgsz = sysconf(_SC_PAGE_SIZE);
		assert(pgsz > 0);
	}

	return (size_t) pgsz;
}

static size_t stack_size(size_t size)
{
	size_t pgsz = page_size(), sz;
	if (size < pgsz)
		size = pgsz;
	sz = (size + pgsz - 1) & ~(pgsz - 1);
	return sz;
}

static void *stack_alloc(size_t size)
{
	int    ret;
	char  *ptr = NULL;
	size_t pgsz = page_size();

	size = stack_size(size);
	size += pgsz;

	ret = posix_memalign((void *) &ptr, pgsz, size);
	if (ret != 0)
		acl_msg_fatal("%s(%d), %s: posix_memalign error %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());

	ret = mprotect(ptr, pgsz, PROT_NONE);
	if (ret != 0)
		acl_msg_fatal("%s(%d), %s: mprotect error=%s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());

	ptr += pgsz;

	return ptr;
}

static void stack_free(void *ptr)
{
	int ret;
	size_t pgsz = page_size();

	ptr = (char *) ptr - pgsz;
	ret = mprotect(ptr, page_size(), PROT_READ|PROT_WRITE);
	if (ret != 0)
		acl_msg_fatal("%s(%d), %s: mprotect error=%s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
	free(ptr);
}

#else

static void *stack_alloc(size_t size)
{
	return acl_mymalloc(size);
}

static void stack_free(void *ptr)
{
	acl_myfree(ptr);
}

#endif

#ifndef	USE_JMP
static void *stack_calloc(size_t size)
{
	void* ptr = stack_alloc(size);
	if (ptr)
		memset(ptr, 0, size);
	return ptr;
}
#endif

/****************************************************************************/

/* forward declare */
static ACL_FIBER *fiber_alloc(void (*fn)(ACL_FIBER *, void *),
	void *arg, size_t size);

void acl_fiber_hook_api(int onoff)
{
	acl_var_hook_sys_api = onoff;
}

int acl_fiber_scheduled(void)
{
	return __scheduled;
}

static void thread_free(void *ctx)
{
	THREAD *tf = (THREAD *) ctx;

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

	__thread_fiber = (THREAD *) acl_mycalloc(1, sizeof(THREAD));
#ifdef	USE_JMP
	/* set context NULL when using setjmp that setcontext will not be
	 * called in fiber_swap.
	 */
	__thread_fiber->original.context = NULL;
#else
	__thread_fiber->original.context = (ucontext_t *)
		stack_calloc(sizeof(ucontext_t));
#endif
	__thread_fiber->fibers = NULL;
	__thread_fiber->size   = 0;
	__thread_fiber->slot   = 0;
	__thread_fiber->idgen  = 0;
	__thread_fiber->count  = 0;
	__thread_fiber->nlocal = 0;

	acl_ring_init(&__thread_fiber->ready);
	acl_ring_init(&__thread_fiber->dead);

	if ((unsigned long) acl_pthread_self() == acl_main_thread_self()) {
		__main_fiber = __thread_fiber;
		atexit(fiber_schedule_main_free);
	} else if (acl_pthread_setspecific(__fiber_key, __thread_fiber) != 0)
		acl_msg_fatal("acl_pthread_setspecific error!");
}

#ifdef	HOOK_ERRNO

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

#endif

int acl_fiber_sys_errno(void)
{
	if (__sys_errno == NULL)
		fiber_init();
	return *__sys_errno();
}

void acl_fiber_sys_errno_set(int errnum)
{
	if (__sys_errno == NULL)
		fiber_init();
	*__sys_errno() = errnum;
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

#if defined(__x86_64__)

# if defined(__AVX__)
#  define CLOBBER \
        , "ymm0", "ymm1", "ymm2", "ymm3", "ymm4", "ymm5", "ymm6", "ymm7",\
        "ymm8", "ymm9", "ymm10", "ymm11", "ymm12", "ymm13", "ymm14", "ymm15"
# else
#  define CLOBBER
# endif

//	asm(".cfi_undefined rip;\r\n")

# define SETJMP(ctx) ({\
    int ret;\
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
        : "memory", "rcx", "r8", "r9", "r10", "r11",\
          "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7",\
          "xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15"\
          CLOBBER\
          );\
    ret;\
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
    int ret;\
    asm("movl   $LJMPRET%=, %%eax\n\t"\
	"movl   %%eax, (%%edx)\n\t"\
	"movl   %%ebx, 4(%%edx)\n\t"\
	"movl   %%esi, 8(%%edx)\n\t"\
	"movl   %%edi, 12(%%edx)\n\t"\
	"movl   %%ebp, 16(%%edx)\n\t"\
	"movl   %%esp, 20(%%edx)\n\t"\
	"xorl   %%eax, %%eax\n\t"\
	"LJMPRET%=:\n\t"\
	: "=a" (ret) : "d" (ctx) : "memory");\
	ret;\
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

static void fiber_kick(int max)
{
	ACL_RING *head;
	ACL_FIBER *fiber;

	while (max > 0) {
		head = acl_ring_pop_head(&__thread_fiber->dead);
		if (head == NULL)
			break;
		fiber = ACL_RING_TO_APPL(head, ACL_FIBER,me);
		fiber_free(fiber);
		max--;
	}
}

static void fiber_swap(ACL_FIBER *from, ACL_FIBER *to)
{
	if (from->status == FIBER_STATUS_EXITING) {
		size_t slot = from->slot;
		int n = acl_ring_size(&__thread_fiber->dead);

		/* if the cached dead fibers reached the limit,
		 * some will be freed
		 */
		if (n > MAX_CACHE) {
			n -= MAX_CACHE;
			fiber_kick(n);
		}

		if (!from->sys)
			__thread_fiber->count--;

		__thread_fiber->fibers[slot] =
			__thread_fiber->fibers[--__thread_fiber->slot];
		__thread_fiber->fibers[slot]->slot = slot;

		acl_ring_prepend(&__thread_fiber->dead, &from->me);
	}

#ifdef	USE_JMP
	/* use setcontext() for the initial jump, as it allows us to set up
	 * a stack, but continue with longjmp() as it's much faster.
	 */
	if (SETJMP(from->env) == 0) {
		/* context just be used once for set up a stack, which will
		 * be freed in fiber_start. The context in __thread_fiber
		 * was set NULL.
		 */
		if (to->context != NULL)
			setcontext(to->context);
		else
			LONGJMP(to->env);
	}
#else
	if (swapcontext(from->context, to->context) < 0)
		acl_msg_fatal("%s(%d), %s: swapcontext error %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
#endif
}

static void check_timer(ACL_FIBER *fiber acl_unused, void *ctx)
{
	size_t *intptr = (size_t *) ctx;
	size_t  max = *intptr;

	acl_myfree(intptr);
	while (1) {
		sleep(1);
		fiber_kick((int) max);
	}
}

void acl_fiber_check_timer(size_t max)
{
	size_t *intptr = (size_t *) acl_mymalloc(sizeof(int));

	*intptr = max;
	acl_fiber_create(check_timer, intptr, 64000);
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
	size_t  n;

	if (acl_ring_size(&__thread_fiber->ready) == 0)
		return 0;

	n = __thread_fiber->switched;
	acl_fiber_ready(__thread_fiber->running);
	acl_fiber_switch();

	// when switched overflows, it will be set to 0, then n saved last
	// switched's value will larger than switched, so we need to use
	// abs function to avoiding this problem
	return abs(__thread_fiber->switched - n - 1);
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

#ifdef	USE_JMP
	/* when using setjmp/longjmp, the context just be used only once */
	if (fiber->context != NULL) {
		stack_free(fiber->context);
		fiber->context = NULL;
	}
#endif

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

int acl_fiber_ndead(void)
{
	if (__thread_fiber == NULL)
		return 0;
	return acl_ring_size(&__thread_fiber->dead);
}

static void fbase_init(FIBER_BASE *fbase, int flag)
{
	fbase->flag      = flag;
	fbase->mutex_in  = -1;
	fbase->mutex_out = -1;
	fbase->atomic    = NULL;
	acl_ring_init(&fbase->mutex_waiter);

	assert(fbase->atomic == NULL);

	fbase->atomic = acl_atomic_new();
	acl_atomic_set(fbase->atomic, &fbase->atomic_value);
	acl_atomic_int64_set(fbase->atomic, 0);
}

static void fbase_finish(FIBER_BASE *fbase)
{
	fbase_event_close(fbase);
	acl_atomic_free(fbase->atomic);
}

FIBER_BASE *fbase_alloc(void)
{
	FIBER_BASE *fbase = (FIBER_BASE *) acl_mycalloc(1, sizeof(FIBER_BASE));

	fbase_init(fbase, FBASE_F_BASE);
	return fbase;
}

void fbase_free(FIBER_BASE *fbase)
{
	fbase_finish(fbase);
	acl_myfree(fbase);
}

void fiber_free(ACL_FIBER *fiber)
{
#ifdef USE_VALGRIND
	VALGRIND_STACK_DEREGISTER(fiber->vid);
#endif
	fbase_finish(&fiber->base);

	if (fiber->context)
		stack_free(fiber->context);
	stack_free(fiber->buff);
	acl_myfree(fiber);
}

static ACL_FIBER *fiber_alloc(void (*fn)(ACL_FIBER *, void *),
	void *arg, size_t size)
{
	ACL_FIBER *fiber;
	sigset_t zero;
	union cc_arg carg;
	ACL_RING *head;

	fiber_check();

#define	APPL	ACL_RING_TO_APPL

	/* try to reuse the fiber memory in dead queue */
	head = acl_ring_pop_head(&__thread_fiber->dead);
	if (head == NULL) {
		fiber = (ACL_FIBER *) acl_mycalloc(1, sizeof(ACL_FIBER));
		/* no using calloc just avoiding using real memory */
		fiber->buff = (char *) stack_alloc(size);
		fbase_init(&fiber->base, FBASE_F_FIBER);
	} else if ((fiber = APPL(head, ACL_FIBER, me))->size < size) {
		/* if using realloc, real memory will be used, when we first
		 * free and malloc again, then we'll just use virtual memory,
		 * because memcpy will be called in realloc.
		 */
		stack_free(fiber->buff);
		fiber->buff = (char *) stack_alloc(size);
	} else
		size = fiber->size;

	__thread_fiber->idgen++;
	if (__thread_fiber->idgen == 0)  /* overflow ? */
		__thread_fiber->idgen++;

	fiber->id     = __thread_fiber->idgen;
	fiber->errnum = 0;
	fiber->signum = 0;
	fiber->fn     = fn;
	fiber->arg    = arg;
	fiber->size   = size;
	fiber->flag   = 0;
	fiber->status = FIBER_STATUS_READY;

	fiber->waiting = NULL;
	acl_ring_init(&fiber->holding);

	carg.p = fiber;

	if (fiber->context == NULL)
		fiber->context = (ucontext_t *) stack_alloc(sizeof(ucontext_t));
	memset(fiber->context, 0, sizeof(ucontext_t));

	sigemptyset(&zero);
	sigaddset(&zero, SIGPIPE);
	sigaddset(&zero, SIGSYS);
	sigaddset(&zero, SIGALRM);
	sigaddset(&zero, SIGURG);
	sigaddset(&zero, SIGWINCH);
//	sigaddset(&zero, SIGINT);
//	sigaddset(&zero, SIGHUP);
	sigprocmask(SIG_BLOCK, &zero, &fiber->context->uc_sigmask);

	if (getcontext(fiber->context) < 0)
		acl_msg_fatal("%s(%d), %s: getcontext error: %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());

	fiber->context->uc_stack.ss_sp   = fiber->buff + 8;
	fiber->context->uc_stack.ss_size = fiber->size - 64;

#ifdef	USE_JMP
	fiber->context->uc_link = NULL;
#else
	fiber->context->uc_link = __thread_fiber->original.context;
#endif

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
	static acl_pthread_mutex_t __lock = PTHREAD_MUTEX_INITIALIZER;
	static int __called = 0;

	(void) acl_pthread_mutex_lock(&__lock);

	if (__called != 0) {
		(void) pthread_mutex_unlock(&__lock);
		return;
	}

	__called++;

#ifdef ACL_ARM_LINUX
	__sys_errno   = (errno_fn) dlsym(RTLD_NEXT, "__errno");
#else
	__sys_errno   = (errno_fn) dlsym(RTLD_NEXT, "__errno_location");
#endif
	__sys_fcntl   = (fcntl_fn) dlsym(RTLD_NEXT, "fcntl");

	(void) acl_pthread_mutex_unlock(&__lock);

	hook_io();
	hook_net();
	hook_epoll();
}

void acl_fiber_schedule(void)
{
	ACL_FIBER *fiber;
	ACL_RING *head;

	fiber_check();
	acl_fiber_hook_api(1);
	__scheduled = 1;

	for (;;) {
		head = acl_ring_pop_head(&__thread_fiber->ready);
		if (head == NULL) {
			acl_msg_info("thread-%lu: NO ACL_FIBER NOW",
				acl_pthread_self());
			break;
		}

		fiber = ACL_RING_TO_APPL(head, ACL_FIBER, me);
		fiber->status = FIBER_STATUS_READY;

		__thread_fiber->running = fiber;
		__thread_fiber->switched++;

		fiber_swap(&__thread_fiber->original, fiber);
		__thread_fiber->running = NULL;
	}

	/* release dead fiber */
	while ((head = acl_ring_pop_head(&__thread_fiber->dead)) != NULL) {
		fiber = ACL_RING_TO_APPL(head, ACL_FIBER, me);
		fiber_free(fiber);
	}

	acl_fiber_hook_api(0);
	__scheduled = 0;
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
	ssize_t left = (char *) &fiber - (char *) current->buff;

	// xxx: sanity checking, just for stack size checking
	if (left < 1024)
		acl_msg_fatal("%s(%d), %s: left=%ld, no space to save current"
			" fiber's stack, current stack=%p, start stack=%p",
			__FILE__, __LINE__, __FUNCTION__, (long) left,
			(void *) &fiber, (void *) current->buff);

#ifdef _DEBUG
	acl_assert(current);
#endif

	head = acl_ring_pop_head(&__thread_fiber->ready);
	if (head == NULL) {
		acl_msg_info("thread-%lu: NO FIBER in ready",
			acl_pthread_self());
		fiber_swap(current, &__thread_fiber->original);
		return;
	}

	fiber = ACL_RING_TO_APPL(head, ACL_FIBER, me);
	//fiber->status = FIBER_STATUS_READY;

	__thread_fiber->running = fiber;
	__thread_fiber->switched++;

	fiber_swap(current, __thread_fiber->running);
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
		int i, n = curr->nlocal;
		curr->nlocal = __thread_fiber->nlocal;
		curr->locals = (FIBER_LOCAL **) acl_myrealloc(curr->locals,
			curr->nlocal * sizeof(FIBER_LOCAL*));
		for (i = n; i < curr->nlocal; i++)
			curr->locals[i] = NULL;
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
