#ifndef FIBER_INCLUDE_H
#define FIBER_INCLUDE_H

#include "common/ring.h"
#include "event.h"

/*
#ifdef ANDROID
extern int getcontext(ucontext_t *ucp);
extern int setcontext(const ucontext_t *ucp);
extern int swapcontext(struct ucontext *old_ctx, struct ucontext *new_ctx);
extern void makecontext(ucontext_t *ucp, void (*func)(), int argc, ...);
#endif
*/

typedef struct {
	void  *ctx;
	void (*free_fn)(void *);
} FIBER_LOCAL;

typedef struct FIBER_BASE {
#define	FBASE_F_BASE	(1 << 0)
#define FBASE_F_FIBER	(1 << 1)
	unsigned flag;

	socket_t event_in;
	socket_t event_out;
	FILE_EVENT *in;
	FILE_EVENT *out;
	RING     event_waiter;
} FIBER_BASE;

struct SYNC_WAITER;

struct ACL_FIBER {
	FIBER_BASE    *base;
	RING           me;
	long           tid;
	unsigned       fid;
	unsigned       slot;
	long long      when;
	int            errnum;	// The fiber's current errno.
	int            signum;	// The signed number to the fiber.
	unsigned short status;	// The fiber's status as FIBER_STATUS_XXX.
	unsigned short wstatus;	// The fiber's waiting status as FIBER_WAIT_XXX.
	unsigned int   oflag;	// The flags for creating fiber.
	unsigned int   flag;	// The flags for the fiber's running status.

#define	FIBER_F_STARTED		(unsigned) (1 << 0)
#define	FIBER_F_SAVE_ERRNO	(unsigned) (1 << 1)
#define	FIBER_F_KILLED		(unsigned) (1 << 2)
#define	FIBER_F_CLOSED		(unsigned) (1 << 3)
#define	FIBER_F_SIGNALED	(unsigned) (1 << 4)
#define	FIBER_F_CANCELED	(FIBER_F_KILLED | FIBER_F_CLOSED | FIBER_F_SIGNALED)
#define	FIBER_F_TIMER		(unsigned) (1 << 5)

#ifdef	DEBUG_LOCK
	RING           holding;
	ACL_FIBER_LOCK *waiting;
#endif

	struct SYNC_WAITER *sync;

	FIBER_LOCAL  **locals;
	int            nlocal;

#ifdef DEBUG_READY
	int            cline;
	int            lline;
	char           ctag[32];
	char           ltag[32];
	long long      curr;
	long long      last;
	unsigned short lstatus;
	unsigned short lwstatus;
	unsigned int   lflag;
	time_t         cost;
	FILE_EVENT    *fe;
#endif
};

#ifdef DEBUG_READY
# define FIBER_READY(f) {                                                     \
	if ((f)->ctag[0] != 0) {                                              \
		SAFE_STRNCPY((f)->ltag, (f)->ctag, sizeof((f)->ltag));        \
		(f)->lline    = (f)->cline;                                   \
		(f)->last     = (f)->curr;                                    \
		(f)->lstatus  = (f)->status;                                  \
		(f)->lwstatus = (f)->wstatus;                                 \
		(f)->lflag    = (f)->flag;                                    \
	}                                                                     \
	SAFE_STRNCPY((f)->ctag, __FUNCTION__, sizeof((f)->ctag));             \
	(f)->cline = __LINE__;                                                \
	SET_TIME((f)->curr);                                                  \
	acl_fiber_ready((f));                                                 \
}
#else
# define FIBER_READY(f) acl_fiber_ready((f))
#endif

/* in fiber.c */
extern __thread int var_hook_sys_api;

FIBER_BASE *fbase_alloc(unsigned flag);
void fbase_free(FIBER_BASE *fbase);
void fiber_free(ACL_FIBER *fiber);
void fiber_start(ACL_FIBER *fiber, void (*fn)(ACL_FIBER *, void *), void *arg);
ACL_FIBER *fiber_origin(void);

#ifdef	SHARE_STACK
char *fiber_share_stack_addr(void);
char *fiber_share_stack_bottom(void);
size_t fiber_share_stack_size(void);
size_t fiber_share_stack_dlen(void);
void fiber_share_stack_set_dlen(size_t dlen);
#endif

/* in fbase.c */
void fbase_event_open(FIBER_BASE *fbase);
void fbase_event_close(FIBER_BASE *fbase);
int fbase_event_wait(FIBER_BASE *fbase);
int fbase_event_wakeup(FIBER_BASE *fbase);

/* in fiber_schedule.c */
void fiber_save_errno(int errnum);
void fiber_exit(int exit_code);

/* in fiber_io.c */
extern int var_maxfd;

void fiber_io_check(void);
void fiber_io_clear(void);

// fiber_wait_read and fiber_wait_write will check if the given fd holding
// in fe is a valid socket, if fd is a valid socket, it will be added to the
// event loop until it's ready for  reading or writing, and the current fiber
// will be suspended; if the given fd in fe isn't a valid socket, the function
// will return immediatly, users can check fe->type.
// the return value is same as which is from event_add_read or event_add_write.
int fiber_wait_read(FILE_EVENT *fe);
int fiber_wait_write(FILE_EVENT *fe);

EVENT *fiber_io_event(void);
void fiber_timer_add(ACL_FIBER *fiber, size_t milliseconds);
int fiber_timer_del(ACL_FIBER *fiber);
int fiber_timer_exist(ACL_FIBER *fiber);

FILE_EVENT *fiber_file_open(socket_t fd);
void fiber_file_set(FILE_EVENT *fe);
FILE_EVENT *fiber_file_get(socket_t fd);
void fiber_file_free(FILE_EVENT *fe);
int fiber_file_close(FILE_EVENT *fe);
FILE_EVENT *fiber_file_cache_get(socket_t fd);
void fiber_file_cache_put(FILE_EVENT *fe);

/* in fiber/fiber_unix.c, fiber/fiber_win.c */
ACL_FIBER *fiber_real_origin(void);
ACL_FIBER *fiber_real_alloc(const ACL_FIBER_ATTR *attr);
void fiber_real_init(ACL_FIBER *fiber, size_t size,
	void (*fn)(ACL_FIBER *, void *), void *arg);
void fiber_real_swap(ACL_FIBER *from, ACL_FIBER *to);
void fiber_real_free(ACL_FIBER *fiber);

#endif
