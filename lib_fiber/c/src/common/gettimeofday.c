#include "stdafx.h"
#include "pthread_patch.h"
#include "msg.h"
#include "init.h"
#include "gettimeofday.h"

#ifdef SYS_WIN

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <time.h>

struct timezone {
	int tz_minuteswest; /**< minutes W of Greenwich */
	int tz_dsttime;     /**< type of dst correction */
};

# ifndef __GNUC__
#   define EPOCHFILETIME (116444736000000000i64)
# else
#  define EPOCHFILETIME (116444736000000000LL)
# endif

static void dummy(void *ptr fiber_unused)
{
}

static void free_tls(void *ptr)
{
	free(ptr);
}

static void *__tls = NULL;
static void main_free_tls(void)
{
	if (__tls) {
		free(__tls);
		__tls = NULL;
	}
}

static pthread_key_t  once_key;
static void once_init(void)
{
	if (__pthread_self() == main_thread_self()) {
		pthread_key_create(&once_key, dummy);
		atexit(main_free_tls);
	} else
		pthread_key_create(&once_key, free_tls);
}

static pthread_once_t once_control = PTHREAD_ONCE_INIT;
static void *tls_calloc(size_t len)
{
	void *ptr;

	(void) pthread_once(&once_control, once_init);
	ptr = (void*) pthread_getspecific(once_key);
	if (ptr == NULL) {
		ptr = calloc(1, len);
		pthread_setspecific(once_key, ptr);
		if (__pthread_self() == main_thread_self())
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

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
	FILETIME        ft;
	LARGE_INTEGER   li;
	__int64         t;
	int             nnested = 0;
	LARGE_INTEGER   stamp;
	time_t now;
	TIME_CTX_T *ctx = (TIME_CTX_T*) tls_calloc(sizeof(TIME_CTX_T));

	/* 每个线程调用此函数时都需要进行初始化，但为了防止开机时间太长
	 * 而造成时钟计数归零溢出，所以每隔 1 天校对一次基准时间
	 */
#define DAY_SEC	(3600 * 24)

	time(&now);
	if (now - ctx->last_init > DAY_SEC) {
		ctx->last_init = now;

		/* 获得CPU的时钟频率 */
		if (!QueryPerformanceFrequency(&ctx->frequency))
			msg_fatal("%s(%d): Unable to get System Frequency(%s)",
				__FILE__, __LINE__, last_serror());
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
			msg_fatal("%s(%d): unable to get System time(%s)",
				__FILE__, __LINE__, last_serror());
	}

	/* 开始获得现在的时间截 */

	if (tv) {
		/* 获得本次开机后至现在的时钟计数  */
		if (!QueryPerformanceCounter(&stamp))
			msg_fatal("%s(%d): unable to get System time(%s)",
				__FILE__, __LINE__, last_serror());

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

#endif
