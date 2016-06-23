#ifndef EVENT_INCLUDE_H
#define EVENT_INCLUDE_H

#include "fiber/lib_fiber.h"

#define	TYPE_NONE	0
#define	TYPE_SOCK	1
#define	TYPE_NOSOCK	2

#define	EVENT_NONE	0
#define	EVENT_READABLE	1 << 0
#define	EVENT_WRITABLE	1 << 1
#define	EVENT_ERROR	1 << 2

typedef struct FILE_EVENT   FILE_EVENT;
typedef struct POLL_EVENTS  POLL_EVENTS;
typedef struct FIRED_EVENT  FIRED_EVENT;
typedef struct DEFER_DELETE DEFER_DELETE;
typedef struct EVENT EVENT;

typedef void event_proc(EVENT *ev, int fd, void *ctx, int mask);
typedef void events_proc(EVENT *ev, POLL_EVENTS *pe);

struct FILE_EVENT {
	int type;
	int mask;
	event_proc  *r_proc;
	event_proc  *w_proc;
	POLL_EVENTS *pevents;
	struct pollfd *pfd;
	void *ctx;
	DEFER_DELETE *defer;
};

struct POLL_EVENTS {
	ACL_RING me;
	struct pollfd *fds;
	int    nfds;
	int    nready;
	FIBER *curr;
	events_proc *proc;
};

struct FIRED_EVENT {
	int fd;
	int mask;
};

struct DEFER_DELETE {
	int fd;
	int mask;
	int pos;
};

struct EVENT {
	int   setsize;
	int   maxfd;
	FILE_EVENT   *events;
	FIRED_EVENT  *fired;
	DEFER_DELETE *defers;
	int   ndefer;
	int   timeout;
	ACL_RING pevents_list;
	ACL_RING_ITER iter;

	const char *(*name)(void);
	int  (*loop)(EVENT *, struct timeval *);
	int  (*add)(EVENT *, int, int);
	void (*del)(EVENT *, int, int);
	void (*free)(EVENT *);
};

EVENT *event_create(int size);
int  event_size(EVENT *ev);
void event_free(EVENT *ev);
int  event_add(EVENT *ev, int fd, int mask, event_proc *proc, void *ctx);
void event_poll(EVENT *ev, POLL_EVENTS *pe, int timeout);
void event_del(EVENT *ev, int fd, int mask);
int  event_mask(EVENT *ev, int fd);
int  event_process(EVENT *ev, int left);

#endif
