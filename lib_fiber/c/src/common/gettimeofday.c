#include "stdafx.h"
#include "fiber/fiber_base.h"
#include "pthread_patch.h"
#include "memory.h"
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
	mem_free(ptr);
}

static void *__tls = NULL;
static void main_free_tls(void)
{
	if (__tls) {
		mem_free(__tls);
		__tls = NULL;
	}
}

static pthread_key_t  once_key;
static void once_init(void)
{
	if (__pthread_self() == main_thread_self()) {
		pthread_key_create(&once_key, dummy);
		atexit(main_free_tls);
	} else {
		pthread_key_create(&once_key, free_tls);
	}
}

static pthread_once_t once_control = PTHREAD_ONCE_INIT;
static void *tls_calloc(size_t len)
{
	void *ptr;

	(void) pthread_once(&once_control, once_init);
	ptr = (void*) pthread_getspecific(once_key);
	if (ptr == NULL) {
		ptr = mem_calloc(1, len);
		pthread_setspecific(once_key, ptr);
		if (__pthread_self() == main_thread_self()) {
			__tls = ptr;
		}
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
		if (!QueryPerformanceFrequency(&ctx->frequency)) {
			msg_fatal("%s(%d): Unable to get System Frequency(%s)",
				__FILE__, __LINE__, last_serror());
		}
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
		if (!QueryPerformanceCounter(&ctx->stamp)) {
			msg_fatal("%s(%d): unable to get System time(%s)",
				__FILE__, __LINE__, last_serror());
		}
	}

	/* 开始获得现在的时间截 */

	if (tv) {
		/* 获得本次开机后至现在的时钟计数  */
		if (!QueryPerformanceCounter(&stamp)) {
			msg_fatal("%s(%d): unable to get System time(%s)",
				__FILE__, __LINE__, last_serror());
		}

		/* 计算当前精确时间截 */
		t = (stamp.QuadPart - ctx->stamp.QuadPart) * 1000000
			/ ctx->frequency.QuadPart;
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

#elif defined(USE_FAST_TIME)

#include "fiber.h"
#include "atomic.h"

static inline unsigned long long rte_rdtsc(void)
{
	union {
		unsigned long long tsc_64;
		struct {
			unsigned lo_32;
			unsigned hi_32;
		};
	} tsc;

	asm volatile("rdtsc" :
			"=a" (tsc.lo_32),
			"=d" (tsc.hi_32));
	return tsc.tsc_64;
}

static unsigned long long __one_msec;
static unsigned long long __one_sec;
static unsigned long long __metric_diff;

void set_time_metric(int ms)
{
	unsigned long long now, startup, end;
	unsigned long long begin = rte_rdtsc();

	usleep(ms * 1000);

	end        = rte_rdtsc();
	__one_msec = (end - begin) / ms;
	__one_sec  = __one_msec * 1000;

	startup    = rte_rdtsc();
	now        = time(NULL) * __one_sec;
	if (now > startup) {
		__metric_diff = now - startup;
	} else {
		__metric_diff = 0;
	}
}

static pthread_once_t __once_control2 = PTHREAD_ONCE_INIT;

static void time_metric_once(void)
{
	set_time_metric(1000);
}

int acl_fiber_gettimeofday(struct timeval *tv, struct timezone *tz fiber_unused)
{
	unsigned long long now;

	if (UNLIKELY(__metric_diff) == 0) {
		if (pthread_once(&__once_control2, time_metric_once) != 0) {
			abort();
		}
	}

	now = rte_rdtsc() + __metric_diff;
	tv->tv_sec  = now / __one_sec;
	tv->tv_usec = (1000 * (now % __one_sec) / __one_msec);
	return 0;
}

# ifdef HOOK_GETTIMEOFDAY
#  ifdef __APPLE__
typedef int (*gettimeofday_fn)(struct timeval *, void *);
#  else
typedef int (*gettimeofday_fn)(struct timeval *, struct timezone *);
#  endif

static gettimeofday_fn __gettimeofday = NULL;

static void hook_api(void)
{
	__gettimeofday = (gettimeofday_fn) dlsym(RTLD_NEXT, "gettimeofday");
	assert(__gettimeofday);
}

static pthread_once_t __once_control = PTHREAD_ONCE_INIT;

static void hook_init(void)
{
	if (pthread_once(&__once_control, hook_api) != 0) {
		abort();
	}
}

#  ifdef __APPLE__
int gettimeofday(struct timeval *tv, void *tz)
#  else
int gettimeofday(struct timeval *tv, struct timezone *tz)
#  endif
{
	if (!var_hook_sys_api) {
		if (__gettimeofday == NULL) {
			hook_init();
		}
		return __gettimeofday(tv, tz);
	}

	return acl_fiber_gettimeofday(tv, (struct timezone*) tz);
}
# endif /* HOOK_GETTIMEOFDAY */
#else
int acl_fiber_gettimeofday(struct timeval *tv, struct timezone *tz)
{
	return gettimeofday(tv, tz);
}
#endif /* USE_FAST_TIME */
