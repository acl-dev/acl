#ifndef FIBER_DEFINE_INCLUDE_H
#define FIBER_DEFINE_INCLUDE_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef intptr_t acl_handle_t;

#if defined(_WIN32) || defined (_WIN64)
# include <winsock2.h>

/* typedef intptr_t ssize_t; */
# ifndef	HAS_SSIZE_T
#  define	HAS_SSIZE_T
/* typedef intptr_t ssize_t; */
#  if defined(_WIN64)
typedef __int64 ssize_t;
#  elif defined(_WIN32)
typedef int ssize_t;
#  else
typedef long ssize_t;
#  endif
# endif
typedef SOCKET socket_t;
typedef int socklen_t;

# define	FIBER_ETIMEDOUT		WSAETIMEDOUT
# define	FIBER_ENOMEM		WSAENOBUFS
# define	FIBER_EINVAL		WSAEINVAL
# define	FIBER_ECONNREFUSED	WSAECONNREFUSED
# define	FIBER_ECONNRESET	WSAECONNRESET
# define	FIBER_EHOSTDOWN		WSAEHOSTDOWN
# define	FIBER_EHOSTUNREACH	WSAEHOSTUNREACH
# define	FIBER_EINTR		WSAEINTR
# define	FIBER_ENETDOWN		WSAENETDOWN
# define	FIBER_ENETUNREACH	WSAENETUNREACH
# define	FIBER_ENOTCONN		WSAENOTCONN
# define	FIBER_EISCONN		WSAEISCONN
# define	FIBER_EWOULDBLOCK	WSAEWOULDBLOCK
# define	FIBER_EAGAIN		FIBER_EWOULDBLOCK	/* xxx */
# define	FIBER_ENOBUFS		WSAENOBUFS
# define	FIBER_ECONNABORTED	WSAECONNABORTED
# define	FIBER_EINPROGRESS	WSAEINPROGRESS

#else

# include <errno.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <sys/time.h>
# include <sys/select.h>
# include <poll.h>
# include <unistd.h>
# include <netdb.h>

# define INVALID_SOCKET	-1
typedef int socket_t;

# define	FIBER_ETIMEDOUT		ETIMEDOUT
# define	FIBER_ENOMEM		ENOMEM
# define	FIBER_EINVAL		EINVAL
# define	FIBER_ECONNREFUSED	ECONNREFUSED
# define	FIBER_ECONNRESET	ECONNRESET
# define	FIBER_EHOSTDOWN		EHOSTDOWN
# define	FIBER_EHOSTUNREACH	EHOSTUNREACH
# define	FIBER_EINTR		EINTR
# define	FIBER_EAGAIN		EAGAIN
# define	FIBER_ENETDOWN		ENETDOWN
# define	FIBER_ENETUNREACH	ENETUNREACH
# define	FIBER_ENOTCONN		ENOTCONN
# define	FIBER_EISCONN		EISCONN
# define	FIBER_EWOULDBLOCK	EWOULDBLOCK
# define	FIBER_ENOBUFS		ENOBUFS
# define	FIBER_ECONNABORTED	ECONNABORTED
# define	FIBER_EINPROGRESS	EINPROGRESS

#endif

#ifdef FIBER_LIB
# ifndef FIBER_API
#  define FIBER_API
# endif
#elif defined(FIBER_DLL) // || defined(_WINDLL)
# if defined(FIBER_EXPORTS) || defined(fiber_EXPORTS)
#  ifndef FIBER_API
#   define FIBER_API __declspec(dllexport)
#  endif
# elif !defined(FIBER_API)
#  define FIBER_API __declspec(dllimport)
# endif
#elif !defined(FIBER_API)
# define FIBER_API
#endif

/**
 * the fiber struct type definition
 */
typedef struct ACL_FIBER ACL_FIBER;

#ifdef __cplusplus
}
#endif

#endif
