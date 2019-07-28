#ifndef	__EVENTS_EPOLL_INCLUDE_H__
#define	__EVENTS_EPOLL_INCLUDE_H__

#ifdef	__cplusplus
extern "C" {
#endif

#include "events_define.h"

 /*
  * Linux epoll supports no system call to find out what descriptors are
  * registered in the kernel-based filter. To implement our own sanity checks
  * we maintain our own descriptor bitmask.
  * 
  * Linux epoll does support application context pointers. Unfortunately,
  * changing that information would cost a system call, and some of the
  * competitors don't support application context. To keep the implementation
  * simple we maintain our own table with call-back information.
  * 
  * Linux epoll silently unregisters a descriptor from its filter when the
  * descriptor is closed, so our information could get out of sync with the
  * kernel. But that will never happen, because we have to meticulously
  * unregister a file descriptor before it is closed, to avoid errors on
  */
#if (ACL_EVENTS_KERNEL_STYLE == ACL_EVENTS_STYLE_EPOLL)
#include <stdint.h>
#include <sys/epoll.h>

#define	EVENT_NAME	"epoll"

/*
 * Macros to initalize the kernel-based filter; see acl_event_init()
 */

#define	EVENT_REG_INIT_HANDLE(eh, n) do { \
	eh = epoll_create(n); \
} while (0)
#define	EVENT_REG_INIT_TEXT	"epoll_create"

/*
 * Macros to update the kernel-based filter; see acl_event_enable_read(),
 * acl_event_enable_write(), acl_event_disable_read(),
 * acl_event_disable_write(), acl_event_disable_readwrite().
 */
#include <stdio.h>

/*
	dummy.events = (ev) | EPOLLET;
*/
#define	EVENT_REG_FD_OP(er, eh, fh, ctx, ev, op) do { \
	struct epoll_event dummy; \
	memset(&dummy, 0, sizeof(dummy)); \
	dummy.events = (ev) | EPOLLHUP | EPOLLERR; \
	dummy.data.ptr = ctx; \
	(er) = epoll_ctl(eh, (op), (fh), &dummy); \
} while (0)

#define	EVENT_REG_ADD_OP(er, eh, fh, ctx, ev) \
	EVENT_REG_FD_OP((er), (eh), (fh), (ctx), (ev), EPOLL_CTL_ADD)
#define	EVENT_REG_ADD_READ(er, eh, fh, ctx) \
	EVENT_REG_ADD_OP((er), (eh), (fh), (ctx), EPOLLIN)
#define	EVENT_REG_ADD_WRITE(er, eh, fh, ctx) \
	EVENT_REG_ADD_OP((er), (eh), (fh), (ctx), EPOLLOUT)
#define	EVENT_REG_ADD_RDWR(er, eh, fh, ctx) \
	EVENT_REG_ADD_OP((er), (eh), (fh), (ctx), EPOLLIN | EPOLLOUT)
#define	EVENT_REG_ADD_TEXT	"epoll_ctl EPOLL_CTL_ADD"

#define	EVENT_REG_MOD_OP(er, eh, fh, ctx, ev) \
	EVENT_REG_FD_OP((er), (eh), (fh), (ctx), (ev), EPOLL_CTL_MOD)
#define	EVENT_REG_MOD_READ(er, eh, fh, ctx) \
	EVENT_REG_MOD_OP((er), (eh), (fh), (ctx), EPOLLIN)
#define	EVENT_REG_MOD_WRITE(er, eh, fh, ctx) \
	EVENT_REG_MOD_OP((er), (eh), (fh), (ctx), EPOLLOUT)
#define	EVENT_REG_MOD_RDWR(er, eh, fh, ctx) \
	EVENT_REG_MOD_OP((er), (eh), (fh), (ctx), EPOLLIN | EPOLLOUT)
#define	EVENT_REG_MOD_TEXT	"epoll_ctl EPOLL_CTL_MOD"

#define	EVENT_REG_DEL_OP(er, eh, fh, ev) \
	EVENT_REG_FD_OP((er), (eh), (fh), NULL, (ev), EPOLL_CTL_DEL)
#define	EVENT_REG_DEL_BOTH(er, eh, fh) \
	EVENT_REG_DEL_OP((er), (eh), (fh), 0)
#define	EVENT_REG_DEL_TEXT	"epoll_ctl EPOLL_CTL_DEL"

/*
 * Macros to retrieve event buffers from the kernel; see acl_event_loop().
 */
typedef struct epoll_event EVENT_BUFFER;

#define	EVENT_BUFFER_READ(ev_cnt, eh, ev_buf, buflen, delay) do { \
	(ev_cnt) = epoll_wait(eh, (ev_buf), (buflen), \
			(delay) < 0 ? -1 : (delay)); \
} while (0)
#define	EVENT_BUFFER_READ_TEXT	"epoll_wait"

/*
 * Macros to process event buffers from the kernel; see acl_event_loop().
 */
#define	EVENT_GET_CTX(bp)		((bp)->data.ptr)
#define	EVENT_GET_TYPE(bp)		((bp)->events)
#define	EVENT_TEST_READ(bp)		(EVENT_GET_TYPE(bp) & EPOLLIN)
#define	EVENT_TEST_WRITE(bp)		(EVENT_GET_TYPE(bp) & EPOLLOUT)
#define	EVENT_TEST_ERROR(bp)		(EVENT_GET_TYPE(bp) & (EPOLLERR | EPOLLHUP))

#endif /* (ACL_EVENTS_KERNEL_STYLE == ACL_EVENTS_STYLE_EPOLL) */

#ifdef	__cplusplus
}
#endif

#endif

