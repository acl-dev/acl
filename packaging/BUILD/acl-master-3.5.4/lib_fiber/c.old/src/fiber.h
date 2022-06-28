#ifndef FIBER_INCLUDE_H
#define FIBER_INCLUDE_H

#include <ucontext.h>
#include <setjmp.h>
#include "event.h"

#ifdef ACL_ARM_LINUX
extern int getcontext(ucontext_t *ucp);
extern int setcontext(const ucontext_t *ucp);
extern int swapcontext(struct ucontext *old_ctx, struct ucontext *new_ctx);
extern void makecontext(ucontext_t *ucp, void (*func)(), int argc, ...);
#endif

typedef enum {
	FIBER_STATUS_READY,
	FIBER_STATUS_RUNNING,
	FIBER_STATUS_EXITING,
} fiber_status_t;

typedef struct {
	void  *ctx;
	void (*free_fn)(void *);
} FIBER_LOCAL;

typedef struct FIBER_BASE {
#define	FBASE_F_BASE	(1 << 0)
#define FBASE_F_FIBER	(1 << 1)
	unsigned flag;

	ACL_ATOMIC *atomic;
	long long   atomic_value;
	int      mutex_in;
	int      mutex_out;
	ACL_RING mutex_waiter;
} FIBER_BASE;

struct ACL_FIBER {
	FIBER_BASE     base;
#ifdef USE_VALGRIND
	unsigned int   vid;
#endif
	fiber_status_t status;
	ACL_RING       me;
	unsigned       id;
	unsigned       slot;
	acl_int64      when;
	int            errnum;
	int            sys;
	int            signum;
	unsigned int   flag;

	ACL_RING         holding;
	ACL_FIBER_MUTEX *waiting;

#define FIBER_F_SAVE_ERRNO	(unsigned) 1 << 0
#define	FIBER_F_KILLED		(unsigned) 1 << 1

	FIBER_LOCAL  **locals;
	int            nlocal;

#ifdef	USE_JMP
# if defined(__x86_64__)
	unsigned long long env[10];
# else
	sigjmp_buf     env;
# endif
#endif
	ucontext_t    *context;
	void         (*fn)(ACL_FIBER *, void *);
	void          *arg;
	void         (*timer_fn)(ACL_FIBER *, void *);
	size_t         size;
	char          *buff;
};

/* in fiber.c */
extern __thread int acl_var_hook_sys_api;
FIBER_BASE *fbase_alloc(void);
void fbase_free(FIBER_BASE *fbase);
void fiber_free(ACL_FIBER *fiber);

/* in fiber_event.c */
int fbase_event_wait(FIBER_BASE *fbase);
int fbase_event_wakeup(FIBER_BASE *fbase);
void fbase_event_close(FIBER_BASE *fbase);

/* in fiber_schedule.c */
void fiber_save_errno(void);
void fiber_exit(int exit_code);
void fiber_system(void);
void fiber_count_inc(void);
void fiber_count_dec(void);

/* in fiber_io.c */
void fiber_io_check(void);
void fiber_io_close(int fd);
void fiber_wait_read(int fd);
void fiber_wait_write(int fd);
void fiber_io_dec(void);
void fiber_io_inc(void);
EVENT *fiber_io_event(void);
void fiber_io_fibers_free(void);

/* in hook_io.c */
void hook_io(void);

/* in fiber_net.c */
void hook_net(void);
int  epoll_event_close(int epfd);
void poll_fibers_free(void);
void epoll_fibers_free(void);

#endif
