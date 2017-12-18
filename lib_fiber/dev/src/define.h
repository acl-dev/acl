#ifndef	__DEFINE_INCLUDE_H__
#define	__DEFINE_INCLUDE_H__

#ifndef unused
# ifdef	__GNUC__
#  define	unused	__attribute__ ((__unused__))
# else
#  define  unused  /* Ignore */
# endif
#endif

#if	__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define	PRINTF(format_idx, arg_idx) \
	__attribute__((__format__ (__printf__, (format_idx), (arg_idx))))
#define	SCANF(format_idx, arg_idx) \
	__attribute__((__format__ (__scanf__, (format_idx), (arg_idx))))
#define	NORETURN __attribute__((__noreturn__))
#define	UNUSED __attribute__((__unused__))
#else
#define	PRINTF(format_idx, arg_idx)
#define	SCANF
#define	NORETURN
#define	UNUSED
#endif	/* __GNUC__ */

#if	__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
#define	DEPRECATED __attribute__((__deprecated__))
#elif	defined(_MSC_VER) && (_MSC_VER >= 1300)
#define	DEPRECATED __declspec(deprecated)
#else
#define	DEPRECATED
#endif	/* __GNUC__ */

#if	__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)
#define	DEPRECATED_FOR(f) __attribute__((deprecated("Use " #f " instead")))
#elif	defined(_MSC_FULL_VER) && (_MSC_FULL_VER > 140050320)
#define	DEPRECATED_FOR(f) __declspec(deprecated("is deprecated. Use '" #f "' instead"))
#else
#define	DEPRECATED_FOR(f)	DEPRECATED
#endif	/* __GNUC__ */

#endif /* __DEFINE_INCLUDE_H__ */
