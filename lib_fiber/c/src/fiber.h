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

	int      event_in;
	int      event_out;
	RING     event_waiter;
} FIBER_BASE;

struct ACL_FIBER {
	FIBER_BASE     base;
	fiber_status_t status;
	RING           me;
	unsigned       id;
	unsigned       slot;
	long long      when;
	int            errnum;
	int            sys;
	int            signum;
	unsigned int   flag;

	RING           holding;
	ACL_FIBER_MUTEX *waiting;

#define FIBER_F_SAVE_ERRNO	(unsigned) 1 << 0
#define	FIBER_F_KILLED		(unsigned) 1 << 1

	FIBER_LOCAL  **locals;
	int            nlocal;

	void (*init_fn)(ACL_FIBER *, size_t);
	void (*free_fn)(ACL_FIBER *);
	void (*swap_fn)(ACL_FIBER *, ACL_FIBER *);
	void (*start_fn)(ACL_FIBER *);

	void (*fn)(ACL_FIBER *, void *);
	void  *arg;
	void (*timer_fn)(ACL_FIBER *, void *);
};

/* in fiber.c */
extern __thread int var_hook_sys_api;
FIBER_BASE *fbase_alloc(void);
void fbase_free(FIBER_BASE *fbase);
void fiber_free(ACL_FIBER *fiber);

/* in fbase.c */
void fbase_event_open(FIBER_BASE *fbase);
void fbase_event_close(FIBER_BASE *fbase);
int fbase_event_wait(FIBER_BASE *fbase);
int fbase_event_wakeup(FIBER_BASE *fbase);

/* in fiber_schedule.c */
void fiber_save_errno(int errnum);
void fiber_exit(int exit_code);
void fiber_system(void);
void fiber_count_inc(void);
void fiber_count_dec(void);

/* in fiber_io.c */
extern int var_maxfd;

void fiber_io_check(void);
void fiber_io_clear(void);
void fiber_wait_read(FILE_EVENT *fe);
void fiber_wait_write(FILE_EVENT *fe);
void fiber_io_dec(void);
void fiber_io_inc(void);
EVENT *fiber_io_event(void);

FILE_EVENT *fiber_file_open(socket_t fd);
int fiber_file_close(socket_t fd, int *closed);

/* in hook/epoll.c */
int  epoll_event_close(int epfd);

/* in fiber/fiber_unix.c */
#ifdef SYS_UNIX
ACL_FIBER *fiber_unix_origin(void);
ACL_FIBER *fiber_unix_alloc(void (*start_fn)(ACL_FIBER *), size_t size);
#endif

/* in fiber/fiber_win.c */
#ifdef SYS_WIN
ACL_FIBER *fiber_win_origin(void);
ACL_FIBER *fiber_win_alloc(void (*start_fn)(ACL_FIBER *), size_t size);
#endif

#endif
