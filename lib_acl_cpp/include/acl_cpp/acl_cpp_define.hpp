#pragma once

#ifdef ACL_CPP_LIB
# ifndef ACL_CPP_API
#  define ACL_CPP_API
# endif
#elif defined(ACL_CPP_DLL) // || defined(_WINDLL)
# if defined(ACL_CPP_EXPORTS) || defined(acl_cpp_EXPORTS)
#  ifndef ACL_CPP_API
#   define ACL_CPP_API __declspec(dllexport)
#  endif
# elif !defined(ACL_CPP_API)
#  define ACL_CPP_API __declspec(dllimport)
# endif
#elif !defined(ACL_CPP_API)
# define ACL_CPP_API
#endif

/*
#ifndef ACL_CPP_TPL
# ifdef ACL_CPP_DLL
#  ifdef ACL_CPP_EXPORTS
#   define ACL_CPP_TPL __declspec(dllexport)
#  else
#   define ACL_CPP_TPL
#  endif
# else
#  define ACL_CPP_TPL
# endif
#endif
*/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#ifdef	_MSC_VER
# pragma warning(disable:4251)
//# if !defined(VC2003) && !defined(VC6)
//extern "C" { FILE _iob[3] = {__iob_func()[0], __iob_func()[1], __iob_func()[2]}; }
//extern "C" { FILE _iob[3]; }
//# endif
# ifndef	HAS_SSIZE_T
#  define	HAS_SSIZE_T
#  if defined(_WIN64)
typedef __int64 ssize_t;
#  elif defined(_WIN32)
typedef int ssize_t;
#  else
typedef long ssize_t;
#  endif
# endif
# if(_MSC_VER >= 1300)
#  include <winsock2.h>
#  include <mswsock.h>
# else
#  include <winsock.h>
# endif
#else
# ifdef HAVE_MEMCACHED
#  undef	HAVE_MEMCACHED
# endif
#endif

#if	__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define	ACL_CPP_PRINTF(format_idx, arg_idx) \
	__attribute__((__format__ (__printf__, (format_idx), (arg_idx))))
#define	ACL_CPP_SCANF(format_idx, arg_idx) \
	__attribute__((__format__ (__scanf__, (format_idx), (arg_idx))))
#define	ACL_CPP_NORETURN __attribute__((__noreturn__))
#define	ACL_CPP_UNUSED __attribute__((__unused__))
#else
#define	ACL_CPP_PRINTF(format_idx, arg_idx)
#define	ACL_CPP_SCANF
#define	ACL_CPP_NORETURN
#define	ACL_CPP_UNUSED
#endif  // __GNUC__

#if	__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
#define	ACL_CPP_DEPRECATED __attribute__((__deprecated__))
#elif	defined(_MSC_VER) && (_MSC_VER >= 1300)
#define	ACL_CPP_DEPRECATED __declspec(deprecated)
#else
#define	ACL_CPP_DEPRECATED
#endif  // __GNUC__

#if	__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)
#define	ACL_CPP_DEPRECATED_FOR(f) __attribute__((deprecated("Use " #f " instead")))
#elif	defined(_MSC_FULL_VER) && (_MSC_FULL_VER > 140050320)
#define	ACL_CPP_DEPRECATED_FOR(f) __declspec(deprecated("is deprecated. Use '" #f "' instead"))
#else
#define	ACL_CPP_DEPRECATED_FOR(f)	ACL_CPP_DEPRECATED
#endif // __GNUC__

#if defined(__GNUC__) && (__GNUC__ > 6 ||(__GNUC__ == 6 && __GNUC_MINOR__ >= 0))
# ifndef   ACL_USE_CPP11
#  define  ACL_USE_CPP11
# endif
#elif	defined(_MSC_VER) && (_MSC_VER >= 1900)
# ifndef   ACL_USE_CPP11
#  define  ACL_USE_CPP11
# endif
#endif // __GNUC__

//#define ACL_CPP_DEBUG_MEM
