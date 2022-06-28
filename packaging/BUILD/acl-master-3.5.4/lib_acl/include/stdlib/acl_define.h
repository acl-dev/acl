#ifndef	ACL_DEFINE_INCLUDE_H
#define	ACL_DEFINE_INCLUDE_H

#ifdef MINGW
# undef _WIN32
# undef _WIN64
#else
# include "acl_define_win32.h"
#endif
#include "acl_define_unix.h"

typedef	acl_int64	acl_off_t;
typedef struct acl_stat acl_stat_t;

/*
 * Making the ctype.h macros not more expensive than necessary. On some
 * systems, ctype.h misbehaves with non-ASCII and/or negative characters.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define _ACL_UCHAR_(c)      ((unsigned char)(c))

#ifdef UNSAFE_CTYPE
#define ACL_ISASCII(c)	isascii(_ACL_UCHAR_(c))
#define ACL_ISALNUM(c)	(ISASCII(c) && isalnum(c))
#define ACL_ISALPHA(c)	(ISASCII(c) && isalpha(c))
#define ACL_ISCNTRL(c)	(ISASCII(c) && iscntrl(c))
#define ACL_ISDIGIT(c)	(ISASCII(c) && isdigit(c))
#define ACL_ISGRAPH(c)	(ISASCII(c) && isgraph(c))
#define ACL_ISLOWER(c)	(ISASCII(c) && islower(c))
#define ACL_ISPRINT(c)	(ISASCII(c) && isprint(c))
#define ACL_ISPUNCT(c)	(ISASCII(c) && ispunct(c))
#define ACL_ISSPACE(c)	(ISASCII(c) && isspace(c))
#define ACL_ISUPPER(c)	(ISASCII(c) && isupper(c))
#define ACL_TOLOWER(c)	(ISUPPER(c) ? tolower(c) : (c))
#define ACL_TOUPPER(c)	(ISLOWER(c) ? toupper(c) : (c))
#else
#define ACL_ISASCII(c)	isascii(_ACL_UCHAR_(c))
#define ACL_ISALNUM(c)	isalnum(_ACL_UCHAR_(c))
#define ACL_ISALPHA(c)	isalpha(_ACL_UCHAR_(c))
#define ACL_ISCNTRL(c)	iscntrl(_ACL_UCHAR_(c))
#define ACL_ISDIGIT(c)	isdigit(_ACL_UCHAR_(c))
#define ACL_ISGRAPH(c)	isgraph(_ACL_UCHAR_(c))
#define ACL_ISLOWER(c)	islower(_ACL_UCHAR_(c))
#define ACL_ISPRINT(c)	isprint(_ACL_UCHAR_(c))
#define ACL_ISPUNCT(c)	ispunct(_ACL_UCHAR_(c))
#define ACL_ISSPACE(c)	isspace(_ACL_UCHAR_(c))
#define ACL_ISUPPER(c)	isupper(_ACL_UCHAR_(c))
#define ACL_TOLOWER(c)	tolower(_ACL_UCHAR_(c))
#define ACL_TOUPPER(c)	toupper(_ACL_UCHAR_(c))
#endif

#ifndef acl_unused
# ifdef	__GNUC__
#  define	acl_unused	__attribute__ ((__unused__))
# else
#  define  acl_unused  /* Ignore */
# endif
#endif

#if	__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define	ACL_PRINTF(format_idx, arg_idx) \
	__attribute__((__format__ (__printf__, (format_idx), (arg_idx))))
#define	ACL_SCANF(format_idx, arg_idx) \
	__attribute__((__format__ (__scanf__, (format_idx), (arg_idx))))
#define	ACL_NORETURN __attribute__((__noreturn__))
#define	ACL_UNUSED __attribute__((__unused__))
#else
#define	ACL_PRINTF(format_idx, arg_idx)
#define	ACL_SCANF
#define	ACL_NORETURN
#define	ACL_UNUSED
#endif	/* __GNUC__ */

#if	__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
#define	ACL_DEPRECATED __attribute__((__deprecated__))
#elif	defined(_MSC_VER) && (_MSC_VER >= 1300)
#define	ACL_DEPRECATED __declspec(deprecated)
#else
#define	ACL_DEPRECATED
#endif	/* __GNUC__ */

#if	__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)
#define	ACL_DEPRECATED_FOR(f) __attribute__((deprecated("Use " #f " instead")))
#elif	defined(_MSC_FULL_VER) && (_MSC_FULL_VER > 140050320)
#define	ACL_DEPRECATED_FOR(f) __declspec(deprecated("is deprecated. Use '" #f "' instead"))
#else
#define	ACL_DEPRECATED_FOR(f)	ACL_DEPRECATED
#endif	/* __GNUC__ */

#ifndef ACL_ADDR_SEP
#define ACL_ADDR_SEP	'|'
#endif

#endif /* __ACL_DEFINE_INCLUDE_H__ */

