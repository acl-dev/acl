#ifndef FIBER_INCLUDE_H
#define FIBER_INCLUDE_H

#include <ucontext.h>
#include <setjmp.h>
#include "event.h"

typedef enum {
	FIBER_STATUS_READY,
	FIBER_STATUS_RUNNING,
	FIBER_STATUS_EXITING,
} fiber_status_t;

typedef struct {
	void  *ctx;
	void (*free_fn)(void *);
} FIBER_LOCAL;

struct ACL_FIBER {
#ifdef USE_VALGRIND
	unsigned int   vid;
#endif
	fiber_status_t status;
	ACL_RING       me;
	size_t         id;
	size_t         slot;
	acl_int64      when;
	int            errnum;
	int            sys;
	unsigned int   flag;
#define FIBER_F_SAVE_ERRNO	(unsigned) 1 << 0

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

/*
 * channel communication
 */
enum
{
	CHANEND,
	CHANSND,
	CHANRCV,
	CHANNOP,
	CHANNOBLK,
};

typedef struct FIBER_ALT FIBER_ALT;
typedef struct FIBER_ALT_ARRAY FIBER_ALT_ARRAY;

struct FIBER_ALT {
	ACL_CHANNEL   *c;
	void          *v;
	unsigned int   op;
	ACL_FIBER     *fiber;
	FIBER_ALT     *xalt;
};

struct FIBER_ALT_ARRAY {
	FIBER_ALT  **a;
	unsigned int n;
	unsigned int m;
};

struct ACL_CHANNEL {
	unsigned int    bufsize;
	unsigned int    elemsize;
	unsigned char  *buf;
	unsigned int    nbuf;
	unsigned int    off;
	FIBER_ALT_ARRAY asend;
	FIBER_ALT_ARRAY arecv;
	char           *name;
};

struct ACL_FIBER_MUTEX {
	ACL_FIBER *owner;
	ACL_RING   waiting;
};

struct ACL_FIBER_RWLOCK {
	int        readers;
	ACL_FIBER *writer;
	ACL_RING   rwaiting;
	ACL_RING   wwaiting;
};

struct ACL_FIBER_SEM {
	int num;
	ACL_RING waiting;
};

/* in fiber.c */
extern __thread int acl_var_hook_sys_api;

/* in fiber_schedule.c */
ACL_FIBER *fiber_running(void);
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

/* in hook_io.c */
void hook_io(void);

/* in fiber_net.c */
void hook_net(void);
int  epoll_event_close(int epfd);

#endif
