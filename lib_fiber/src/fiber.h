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
	ACL_RING me;
	size_t id;
#ifdef USE_VALGRIND
	unsigned int vid;
#endif
	size_t slot;
	acl_int64 when;
	int sys;
	fiber_status_t status;
	ucontext_t uctx;
	void (*fn)(FIBER *, void *);
	void *arg;
	char *stack;
	size_t size;
	char  buf[1];
};

/* in fiber_schedule.c */
void   fiber_init(void);
void   fiber_ready(FIBER *fiber);
FIBER *fiber_running(void);
void   fiber_exit(int exit_code);
void   fiber_free(FIBER *fiber);
void   fiber_system(void);
void   fiber_count_inc(void);
void   fiber_count_dec(void);

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
