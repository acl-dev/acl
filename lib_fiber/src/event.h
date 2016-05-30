#ifndef EVENT_INCLUDE_H
#define EVENT_INCLUDE_H

#define	EVENT_NONE	0
#define	EVENT_READABLE	1
#define	EVENT_WRITABLE	2


typedef struct FILE_EVENT  FILE_EVENT;
typedef struct FIRED_EVENT FIRED_EVENT;
typedef struct EVENT EVENT;

typedef void event_proc(EVENT *ev, int fd, void *ctx, int mask);

struct FILE_EVENT {
	int mask;
	event_proc *r_proc;
	event_proc *w_proc;
	void *ctx;
};

struct FIRED_EVENT {
	int fd;
	int mask;
};

struct EVENT {
	int   setsize;
	int   stop;
	int   maxfd;
	FILE_EVENT  *events;
	FIRED_EVENT *fired;

	const char *(*name)(void);
	int  (*loop)(EVENT *, struct timeval *);
	int  (*add)(EVENT *, int, int);
	void (*del)(EVENT *, int, int);
	void (*free)(EVENT *);
};

EVENT *event_create(int size);
int event_size(EVENT *ev);
void event_free(EVENT *ev);
void event_stop(EVENT *ev);
int event_add(EVENT *ev, int fd, int mask, event_proc *proc, void *ctx);
void event_del(EVENT *ev, int fd, int mask);
int event_mask(EVENT *ev, int fd);
int event_process(EVENT *ev);
int event_wait(int fd, int mask, long long milliseconds);
void event_loop(EVENT *ev);

#endif
