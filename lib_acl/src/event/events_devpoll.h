
#ifndef	__EVENTS_DEVPOLL_INCLUDE_H__
#define	__EVENTS_DEVPOLL_INCLUDE_H__
 
#ifdef	__cplusplus
extern "C" {
#endif

#include "events_define.h"

 /*
  * Solaris /dev/poll does not support application context, so we have to
  * maintain our own. This has the benefit of avoiding an expensive system
  * call just to change a call-back function or argument.
  * 
  * Solaris /dev/poll does have a way to query if a specific descriptor is
  * registered. However, we maintain a descriptor mask anyway because a) it
  * avoids having to make an expensive system call to find out if something
  * is registered, b) some EVENTS_STYLE_MUMBLE implementations need a
  * descriptor bitmask anyway and c) we use the bitmask already to implement
  * sanity checks.
  */
#if (ACL_EVENTS_KERNEL_STYLE == ACL_EVENTS_STYLE_DEVPOLL)
#include <sys/devpoll.h>
#include <fcntl.h>

#define	EVENT_NAME	"devpoll"

 /*
  * Macros to initialize the kernel-based filter; see event_init().
  */

#define EVENT_REG_INIT_HANDLE(eh, n) do { \
	eh = open("/dev/poll", O_RDWR); \
} while (0)
#define EVENT_REG_INIT_TEXT	"open /dev/poll"

 /*
  * Macros to update the kernel-based filter; see event_enable_read(),
  * event_enable_write() and event_disable_readwrite().
  */
#define EVENT_REG_FD_OP(er, eh, fh, ev) do { \
	struct pollfd dummy; \
	dummy.fd = (fh); \
	dummy.events = (ev); \
	dummy.revents = 0; \
	(er) = write(eh, (char *) &dummy, \
	    sizeof(dummy)) != sizeof(dummy) ? -1 : 0; \
} while (0)

#define EVENT_REG_ADD_READ(er, eh, fh, ctx_dummy) \
	EVENT_REG_FD_OP((er), (eh), (fh), POLLIN)
#define EVENT_REG_ADD_WRITE(er, eh, fh, ctx_dummy) \
	EVENT_REG_FD_OP((er), (eh), (fh), POLLOUT)
#define EVENT_REG_ADD_RDWR(er, eh, fh, ctx_dummy) \
	EVENT_REG_FD_OP((er), (eh), (fh), POLLIN | POLLOUT)
#define EVENT_REG_ADD_TEXT        "write /dev/poll add"

#define	EVENT_REG_MOD_READ(er, eh, fh, ctx_dummy) \
	EVENT_REG_FD_OP((er), (eh), (fh), POLLIN)
#define	EVENT_REG_MOD_WRITE(er, eh, fh, ctx_dummy) \
	EVENT_REG_FD_OP((er), (eh), (fh), POLLOUT)
#define	EVENT_REG_MOD_RDWR(er, eh, fh, ctx_dummy) \
	EVENT_REG_FD_OP((er), (eh), (fh), POLLIN | POLLOUT)
#define	EVENT_REG_MOD_TEXT	"write /dev/poll modify"
	
#define EVENT_REG_DEL_BOTH(er, eh, fh) \
	EVENT_REG_FD_OP((er), (eh), (fh), POLLREMOVE)
#define EVENT_REG_DEL_TEXT        "write /dev/poll delete"

 /*
  * Macros to retrieve event buffers from the kernel; see event_loop().
  */
typedef struct pollfd EVENT_BUFFER;

#define EVENT_BUFFER_READ(ev_cnt, eh, ev_buf, buflen, delay) do { \
	struct dvpoll dvpoll; \
	dvpoll.dp_fds = (ev_buf); \
	dvpoll.dp_nfds = (buflen); \
	dvpoll.dp_timeout = (delay) < 0 ? -1 : (delay); \
	(ev_cnt) = ioctl(eh, DP_POLL, &dvpoll); \
} while (0)
#define EVENT_BUFFER_READ_TEXT	"ioctl DP_POLL"

 /*
  * Macros to process event buffers from the kernel; see event_loop().
  */
#define EVENT_GET_FD(bp)		((bp)->fd)
#define EVENT_GET_TYPE(bp)		((bp)->revents)
#define EVENT_TEST_READ(bp)		(EVENT_GET_TYPE(bp) & POLLIN)
#define EVENT_TEST_WRITE(bp)		(EVENT_GET_TYPE(bp) & POLLOUT)
#define	EVENT_TEST_ERROR(bp)		(EVENT_GET_TYPE(bp) & (POLLERR|POLLHUP|POLLNVAL))

#endif	 /* ACL_EVENTS_KERNEL_STYLE == ACL_EVENTS_STYLE_DEVPOLL */

#ifdef	__cplusplus
}
#endif

#endif
