#ifndef	__EVENTS_KQUEUE_INCLUDE_H__ 
#define	__EVENTS_KQUEUE_INCLUDE_H__ 

#ifdef	__cplusplus
extern "C" {
#endif

/*
  * FreeBSD kqueue supports no system call to find out what descriptors are
  * registered in the kernel-based filter. To implement our own sanity checks
  * we maintain our own descriptor bitmask.
  * 
  * FreeBSD kqueue does support application context pointers. Unfortunately,
  * changing that information would cost a system call, and some of the
  * competitors don't support application context. To keep the implementation
  * simple we maintain our own table with call-back information.
  * 
  * FreeBSD kqueue silently unregisters a descriptor from its filter when the
  * descriptor is closed, so our information could get out of sync with the
  * kernel. But that will never happen, because we have to meticulously
  * unregister a file descriptor before it is closed, to avoid errors on
  * systems that are built with EVENTS_STYLE == EVENTS_STYLE_SELECT.
  */
#if (ACL_EVENTS_KERNEL_STYLE == ACL_EVENTS_STYLE_KQUEUE)
#include <sys/event.h>

#define	EVENT_NAME	"kqueue"

 /*
  * Some early FreeBSD implementations don't have the EV_SET macro.
  */
#ifndef EV_SET
#define EV_SET(kp, id, fi, fl, ffl, da, ud) do { \
        (kp)->ident = (id); \
        (kp)->filter = (fi); \
        (kp)->flags = (fl); \
        (kp)->fflags = (ffl); \
        (kp)->data = (da); \
        (kp)->udata = (ud); \
    } while(0)
#endif

 /*
  * Macros to initialize the kernel-based filter; see event_init().
  */

#define EVENT_REG_INIT_HANDLE(eh, n) do { \
	eh = kqueue(); \
    } while (0)
#define EVENT_REG_INIT_TEXT	"kqueue"

 /*
  * Macros to update the kernel-based filter; see event_enable_read(),
  * event_enable_write() and event_disable_readwrite().
  */
#define EVENT_REG_FD_OP(er, eh, fh, ctx, ev, op) do { \
	struct kevent dummy; \
	EV_SET(&dummy, (fh), (ev), (op), 0, 0, ctx); \
	(er) = kevent(eh, &dummy, 1, 0, 0, 0); \
    } while (0)

#define EVENT_REG_ADD_OP(er, eh, fh, ctx, ev) \
	EVENT_REG_FD_OP((er), (eh), (fh), ctx, (ev), EV_ADD)
#define EVENT_REG_ADD_READ(er, eh, fh, ctx)  \
	EVENT_REG_ADD_OP((er), (eh), (fh), ctx, EVFILT_READ)
#define EVENT_REG_ADD_WRITE(er, eh, fh, ctx) \
	EVENT_REG_ADD_OP((er), (eh), (fh), ctx, EVFILT_WRITE)
#define EVENT_REG_ADD_TEXT	"kevent EV_ADD"

#define EVENT_REG_DEL_OP(er, eh, fh, ev) \
	EVENT_REG_FD_OP((er), (eh), (fh), NULL, (ev), EV_DELETE)
#define EVENT_REG_DEL_READ(er, eh, fh) \
	EVENT_REG_DEL_OP((er), (eh), (fh), EVFILT_READ)
#define EVENT_REG_DEL_WRITE(er, eh, fh) \
	EVENT_REG_DEL_OP((er), (eh), (fh), EVFILT_WRITE)
#define EVENT_REG_DEL_TEXT         "kevent EV_DELETE"

 /*
  * Macros to retrieve event buffers from the kernel; see event_loop().
  */
typedef struct kevent EVENT_BUFFER;

#define EVENT_BUFFER_READ(ev_cnt, eh, ev_buf, buflen, delay) do { \
	struct timespec ts; \
	struct timespec *tsp; \
	if ((delay) < 0) { \
	    tsp = 0; \
	} else { \
	    tsp = &ts; \
	    ts.tv_nsec = ((delay) % 1000) * 1000000; \
	    ts.tv_sec = (delay) / 1000; \
	} \
	(ev_cnt) = kevent(eh, (struct kevent *) 0, 0, (ev_buf), (buflen), (tsp)); \
} while (0)
#define EVENT_BUFFER_READ_TEXT	"kevent"

 /*
  * Macros to process event buffers from the kernel; see event_loop().
  */
#define	EVENT_GET_FD(fp)	((bp)->ident)
#define EVENT_GET_CTX(bp)	((bp)->udata)
#define EVENT_GET_TYPE(bp)	((bp)->filter)
#define EVENT_TEST_READ(bp)	(EVENT_GET_TYPE(bp) == EVFILT_READ)
#define EVENT_TEST_WRITE(bp)	(EVENT_GET_TYPE(bp) == EVFILT_WRITE)

#endif /* (ACL_EVENTS_KERNEL_STYLE == ACL_EVENTS_STYLE_KQUEUE) */

#ifdef	__cplusplus
}
#endif

#endif
