
#ifndef	__EVENTS_DEFINE_INCLUDE_H__
#define	__EVENTS_DEFINE_INCLUDE_H__

#ifdef	__cplusplus
extern "C" {
#endif

#include "stdlib/acl_define.h"
#include "stdlib/acl_mymalloc.h"
#include <string.h>

#define ACL_EVENT_ALLOC_INCR	10

#define	ACL_EVENTS_STYLE_SELECT		1
#define	ACL_EVENTS_STYLE_EPOLL		2
#define	ACL_EVENTS_STYLE_DEVPOLL	3
#define	ACL_EVENTS_STYLE_KQUEUE		4

#if defined(_WIN32) || defined(_WIN64)
# if(_MSC_VER >= 1300)
#  define ACL_EVENTS_STYLE_IOCP		5
# endif
# define ACL_EVENTS_STYLE_WMSG		6
#endif

#if	defined(ACL_LINUX) && !defined(MINGW)
# define	ACL_EVENTS_KERNEL_STYLE	ACL_EVENTS_STYLE_EPOLL
#elif	defined(SUNOS5)
# define	ACL_EVENTS_KERNEL_STYLE	ACL_EVENTS_STYLE_DEVPOLL
# define	USE_FDMAP
#elif	defined(ACL_FREEBSD) || defined(ACL_MACOSX)
# define	ACL_EVENTS_KERNEL_STYLE	ACL_EVENTS_STYLE_KQUEUE
#else
# undef	ACL_EVENTS_KERNEL_STYLE
#endif

#if	defined(ACL_UNIX)
# define	ACL_EVENTS_POLL_STYLE	5
#elif defined(_WIN32) || defined(_WIN64)
# if _MSC_VER >= 1600
#  define	ACL_EVENTS_POLL_STYLE	5
# else
#  undef	ACL_EVENTS_POLL_STYLE
# endif
#else
# undef		ACL_EVENTS_POLL_STYLE
#endif

#ifdef	__cplusplus
}
#endif

#endif

