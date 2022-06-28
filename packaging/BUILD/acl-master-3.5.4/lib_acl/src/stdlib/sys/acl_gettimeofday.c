#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef  ACL_WINDOWS
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# include <time.h>
#endif

#ifdef ACL_BCB_COMPILER
# pragma hdrstop
#endif

#include "stdlib/acl_msg.h"
#include "stdlib/acl_sys_patch.h"

#endif

#ifdef ACL_WINDOWS

# ifndef __GNUC__
#   define EPOCHFILETIME (116444736000000000i64)
# else
#  define EPOCHFILETIME (116444736000000000LL)
# endif

/* FILETIME of Jan 1 1970 00:00:00. */
# if 0
#define UINT64CONST	(x)	((__int64) x)
static const unsigned __int64 epoch = UINT64CONST(116444736000000000);
# endif
static const unsigned __int64 epoch = 116444736000000000LL;

/*
# ifndef __GNUC__
#   define EPOCHFILETIME (116444736000000000i64)
# else
#  define EPOCHFILETIME (116444736000000000LL)
# endif
*/

/*
#ifndef ACL_DLL
int _daylight;
long _timezone;
#endif
*/
static void dummy(void *ptr acl_unused)
{

}

static void free_tls(void *ptr)
{
	acl_myfree(ptr);
}

static void *__tls = NULL;
#ifndef HAVE_NO_ATEXIT
static void main_free_tls(void)
{
	if (__tls) {
		acl_myfree(__tls);
		__tls = NULL;
	}
}
#endif

static acl_pthread_key_t  once_key;
static void once_init(void)
{
	if ((unsigned long) acl_pthread_self() == acl_main_thread_self()) {
		acl_pthread_key_create(&once_key, dummy);
#ifndef HAVE_NO_ATEXIT
		atexit(main_free_tls);
#endif
	} else
		acl_pthread_key_create(&once_key, free_tls);
}

static acl_pthread_once_t once_control = ACL_PTHREAD_ONCE_INIT;
static void *tls_calloc(size_t len)
{
	void *ptr;

	(void) acl_pthread_once(&once_control, once_init);
	ptr = (void*) acl_pthread_getspecific(once_key);
	if (ptr == NULL) {
		ptr = acl_mycalloc(1, len);
		acl_pthread_setspecific(once_key, ptr);
		if ((unsigned long) acl_pthread_self() == acl_main_thread_self())
			__tls = ptr;
	}
	return ptr;
}

typedef struct {
	time_t last_init;
	struct timeval tvbase;
	LARGE_INTEGER frequency;
	LARGE_INTEGER stamp;
	int tzflag;
} TIME_CTX_T;

int gettimeofday1(struct timeval *tv, struct timezone *tz)
{
	FILETIME        ft;
	LARGE_INTEGER   li;
	__int64         t;
	int             nnested = 0;
	LARGE_INTEGER stamp;
	time_t now;
	TIME_CTX_T *ctx = tls_calloc(sizeof(TIME_CTX_T));

	/* 每个线程调用此函数时都需要进行初始化，但为了防止开机时间太长
	 * 而造成时钟计数归零溢出，所以每隔 1 天校对一次基准时间
	 */
#define DAY_SEC	(3600 * 24)

	time(&now);
	if (now - ctx->last_init > DAY_SEC) {
		ctx->last_init = now;

		/* 获得CPU的时钟频率 */
		if (!QueryPerformanceFrequency(&ctx->frequency))
			acl_msg_fatal("%s(%d): Unable to get System Frequency(%s)",
				__FILE__, __LINE__, acl_last_serror());
		/* 获得系统时间(自 1970 至今) */
		GetSystemTimeAsFileTime(&ft);
		li.LowPart  = ft.dwLowDateTime;
		li.HighPart = ft.dwHighDateTime;
		t  = li.QuadPart;       /* In 100-nanosecond intervals */
		t -= EPOCHFILETIME;     /* Offset to the Epoch time */
		t /= 10;                /* In microseconds */

		/* 转换成本次开机后的基准时间 */
		ctx->tvbase.tv_sec  = (long)(t / 1000000);
		ctx->tvbase.tv_usec = (long)(t % 1000000);

		/* 获得本次开机后到现在的时钟计数 */
		if (!QueryPerformanceCounter(&ctx->stamp))
			acl_msg_fatal("%s(%d): unable to get System time(%s)",
				__FILE__, __LINE__, acl_last_serror());
	}

	/* 开始获得现在的时间截 */

	if (tv) {
		/* 获得本次开机后至现在的时钟计数  */
		if (!QueryPerformanceCounter(&stamp))
			acl_msg_fatal("%s(%d): unable to get System time(%s)",
				__FILE__, __LINE__, acl_last_serror());

		/* 计算当前精确时间截 */
		t = (stamp.QuadPart - ctx->stamp.QuadPart) * 1000000 / ctx->frequency.QuadPart;
		tv->tv_sec = ctx->tvbase.tv_sec + (long)(t / 1000000);
		tv->tv_usec = ctx->tvbase.tv_usec + (long)(t % 1000000);
	}

	if (tz) {
		if (!ctx->tzflag) {
			_tzset();
			ctx->tzflag++;
		}
		tz->tz_minuteswest = _timezone / 60;
		tz->tz_dsttime = _daylight;
	}

	return (0);
}

//int _daylight;
//long _timezone;

int gettimeofday2(struct timeval *tv, struct timezone *tz)
{
	FILETIME        ft;
	LARGE_INTEGER   li;
	__int64         t;
	static int      tzflag;

	if (tv) {
		GetSystemTimeAsFileTime(&ft);
		li.LowPart  = ft.dwLowDateTime;
		li.HighPart = ft.dwHighDateTime;
		t  = li.QuadPart;       /* In 100-nanosecond intervals */
		t -= EPOCHFILETIME;     /* Offset to the Epoch time */
		t /= 10;                /* In microseconds */
		tv->tv_sec  = (long)(t / 1000000);
		tv->tv_usec = (long)(t % 1000000);
	}

	if (tz) {
		if (!tzflag) {
			_tzset();
			tzflag++;
		}
		tz->tz_minuteswest = _timezone / 60;
		tz->tz_dsttime = _daylight;
	}

	return (0);
}

/* Postgresql's implement */

 /*
 * timezone information is stored outside the kernel so tzp isn't used anymore.
 *
 * Note: this function is not for Win32 high precision timing purpose. See
 * elapsed_time().
 */
int gettimeofday3(struct timeval * tp, struct timezone * tzp)
 {
	FILETIME    file_time;
	SYSTEMTIME  system_time;
	ULARGE_INTEGER ularge;
 
	GetSystemTime(&system_time);
	SystemTimeToFileTime(&system_time, &file_time);
	ularge.LowPart = file_time.dwLowDateTime;
	ularge.HighPart = file_time.dwHighDateTime;
 
	tp->tv_sec = (long) ((ularge.QuadPart - epoch) / 10000000L);
	tp->tv_usec = (long) (system_time.wMilliseconds * 1000);
 
    return 0;
}

int gettimeofday4(struct timeval *tp, struct timezone *tzp)
{
	time_t clock;
	struct tm tm;
	SYSTEMTIME wtm;
	GetLocalTime(&wtm);
	tm.tm_year   = wtm.wYear - 1900;
	tm.tm_mon    = wtm.wMonth - 1;
	tm.tm_mday   = wtm.wDay;
	tm.tm_hour   = wtm.wHour;
	tm.tm_min    = wtm.wMinute;
	tm.tm_sec    = wtm.wSecond;
	tm. tm_isdst = -1;
	clock = mktime(&tm);
	tp->tv_sec = (long) clock;
	tp->tv_usec = wtm.wMilliseconds * 1000;
	return (0);
}

int gettimeofday(struct timeval * tp, struct timezone * tzp)
{
	return gettimeofday3(tp, tzp);
}

#endif  /* ACL_WINDOWS */
