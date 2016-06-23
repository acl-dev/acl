#ifndef FIBER_INCLUDE_H
#define FIBER_INCLUDE_H

#include <ucontext.h>
#include "event.h"

typedef enum {
	FIBER_STATUS_READY,
	FIBER_STATUS_RUNNING,
	FIBER_STATUS_EXITING,
} fiber_status_t;

struct FIBER {
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
#define FIBER_F_SAVE_ERRNO	1 << 0

	ucontext_t     uctx;
	void         (*fn)(FIBER *, void *);
	void          *arg;
	char          *stack;
	size_t         size;
	void         (*timer_fn)(FIBER *, void *);
	char           buf[1];
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
	CHANNEL *c;
	void          *v;
	unsigned int   op;
	FIBER         *fiber;
	FIBER_ALT     *xalt;
};

struct FIBER_ALT_ARRAY {
	FIBER_ALT  **a;
	unsigned int n;
	unsigned int m;
};

struct CHANNEL {
	unsigned int    bufsize;
	unsigned int    elemsize;
	unsigned char  *buf;
	unsigned int    nbuf;
	unsigned int    off;
	FIBER_ALT_ARRAY asend;
	FIBER_ALT_ARRAY arecv;
	char           *name;
};

struct FIBER_LOCK {
	FIBER *owner;
	ACL_RING waiting;
};

struct FIBER_RWLOCK {
	int      readers;
	FIBER   *writer;
	ACL_RING rwaiting;
	ACL_RING wwaiting;
};

/* in fiber_schedule.c */
FIBER *fiber_running(void);
void fiber_save_errno(void);
void fiber_exit(int exit_code);
void fiber_free(FIBER *fiber);
void fiber_system(void);
void fiber_count_inc(void);
void fiber_count_dec(void);

/* in fiber_io.c */
void fiber_io_hook(void);
void fiber_io_check(void);
void fiber_wait_read(int fd);
void fiber_wait_write(int fd);
void fiber_io_dec(void);
void fiber_io_inc(void);
EVENT *fiber_io_event(void);

/* in fiber_net.c */
void fiber_net_hook(void);

#endif
