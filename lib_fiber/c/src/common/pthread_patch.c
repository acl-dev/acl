#include "stdafx.h"
#include "fifo.h"
#include "memory.h"
#include "msg.h"
#include "iterator.h"
#include "pthread_patch.h"

#ifdef SYS_WIN

//#define USE_FLS

#ifdef USE_FLS

//#define PTHREAD_ONCE_INIT INIT_ONCE_STATIC_INIT

typedef struct OnceWrapper
{
    void (*init_routine)(void);
} OnceWrapper;

BOOL CALLBACK InitOnceCallback(
    PINIT_ONCE InitOnce,
    PVOID Parameter,
    PVOID* Context)
{
    OnceWrapper* wrapper = (OnceWrapper*)Parameter;
    wrapper->init_routine();
    return TRUE;
}

int pthread_once(pthread_once_t* once_control, void (*init_routine)(void))
{
    OnceWrapper wrapper;
	wrapper.init_routine = init_routine;

    BOOL ok = InitOnceExecuteOnce(
        once_control,
        InitOnceCallback,
        &wrapper,
        NULL);

    return ok ? 0 : -1;
}

int pthread_key_create(pthread_key_t* key, void (*destructor)(void*))
{
    if (!key)
        return -1;

    DWORD k = FlsAlloc((PFLS_KEY_FUNCTION)destructor);

    if (k == FLS_OUT_OF_INDEXES)
        return -1;

    *key = k;
    return 0;
}

int pthread_setspecific(pthread_key_t key, const void* value)
{
    return FlsSetValue(key, (PVOID)value) ? 0 : -1;
}

void* pthread_getspecific(pthread_key_t key)
{
    return FlsGetValue(key);
}

int pthread_key_delete(pthread_key_t key)
{
    return FlsFree(key) ? 0 : -1;
}

#else

static pthread_once_t  __control_once = PTHREAD_ONCE_INIT;

int pthread_once(pthread_once_t *once_control, void (*init_routine)(void))
{
	int n = 0;

	if (once_control == NULL || init_routine == NULL) {
		return EINVAL;
	}

	/* 只有第一个调用 InterlockedCompareExchange 的线程才会执行
	 * init_routine, 后续线程永远在 InterlockedCompareExchange
	 * 外运行，并且一直进入空循环直至第一个线程执行 init_routine
	 * 完毕并且将 *once_control 重新赋值, 只有在多核环境中多个线程
	 * 同时运行至此时才有可能出现短暂的后续线程空循环现象，如果
	 * 多个线程顺序至此，则因为 *once_control 已经被第一个线程重新
	 * 赋值而不会进入循环体内只所以如此处理，是为了保证所有线程在
	 * 调用 pthread_once 返回前 init_routine 必须被调用且仅能
	 * 被调用一次, 但在VC6下，InterlockedCompareExchange 接口定义
	 * 有些怪异，需要做硬性指定参数类型，参见 <Windows 高级编程指南>
	 * Jeffrey Richter, 366 页
	 */
	while (1) {
		LONG prev = InterlockedCompareExchange(
			(LONG*) once_control, 1L, PTHREAD_ONCE_INIT);
		if (prev == 2)
			return 0;
		else if (prev == 0) {
			/* 只有第一个线程才会至此 */
			init_routine();
			/* 将 *conce_control 重新赋值以使后续线程不进入 while
			 * 循环或从 while 循环中跳出
			 */
			InterlockedExchange((LONG*) once_control, 2);
			return 0;
		} else {
			assert(prev == 1);

			/* 防止空循环过多地浪费CPU */
			Sleep(1);  /** sleep 1ms */
		}
	}
	return 1;  /* 不可达代码，避免编译器报警告 */
}

///////////////////////////////////////////////////////////////////////////////

#include "common/htable.h"

typedef struct TLS_KEY {
	pthread_key_t key;
	void (*destructor)(void *);
} TLS_KEY;

typedef struct TLS_OBJ {
	pthread_key_t key;
	void *value;
} TLS_OBJ;

typedef struct FLS_KEY {
	FIFO *locals; // Hold TLS_OBJ
} FLS_KEY;

typedef struct PROC_KEYS {
	HTABLE *keys; // tkey => TLS_KEY
} PROC_KEYS;

static pthread_key_t __tls_key = TLS_OUT_OF_INDEXES;
static pthread_key_t __fls_key = FLS_OUT_OF_INDEXES;
static PROC_KEYS      *__pkeys = NULL;
static pthread_mutex_t __lock;

extern int acl_fiber_scheduled(void);

static void hash_key(pthread_key_t key, char *buf, size_t n)
{
	assert(n > 10);
	_snprintf(buf, n, "%d", key);
	buf[n - 1] = '\0';
}

static void NTAPI thread_exit(void *ctx)
{
	FLS_KEY *fkey = (FLS_KEY *) ctx;
	assert(fkey && fkey->locals);

	// The first one is set in thread_init() in fiber.c, it should be called
	// in the end, because the original fiber will be freed that it means
	// the current thread will be end and the thread context will freed.
	TLS_OBJ *first = fifo_pop_front(fkey->locals), *obj;
	assert(first);

	char key[32];

	pthread_mutex_lock(&__lock);

	while ((obj = fifo_pop_front(fkey->locals))) {
		hash_key(obj->key, key, sizeof(key));
		TLS_KEY *tkey = (TLS_KEY*) htable_find(__pkeys->keys, key);
		if (tkey == NULL || tkey->destructor == NULL) {
			continue;
		}
		tkey->destructor(obj->value);
		mem_free(obj);
	}

	fifo_free(fkey->locals, NULL);
	mem_free(fkey);

	hash_key(first->key, key, sizeof(key));
	TLS_KEY *tkey = (TLS_KEY*) htable_find(__pkeys->keys, key);
	assert(tkey);

	pthread_mutex_unlock(&__lock);

	void *value = first->value;
	mem_free(first);

	// Must the __fls_key's value to NULL to avoid the thread_exit
	// to be triggered again for the current thread when the original
	// fiber is deleted.
	FlsSetValue(__fls_key, NULL);
	tkey->destructor(value);
}

static void thread_once(void)
{
	__tls_key = TlsAlloc();
	__fls_key = FlsAlloc(thread_exit);
	__pkeys   = (PROC_KEYS *) mem_calloc(1, sizeof(PROC_KEYS));
	__pkeys->keys = htable_create(10);
	pthread_mutex_init(&__lock, NULL);
}

void pthread_fkey_create(void)
{
	pthread_once(&__control_once, thread_once);

	FLS_KEY *fkey = (FLS_KEY*) TlsGetValue(__tls_key);
	if (fkey == NULL) {
		assert(!acl_fiber_scheduled());
		fkey = (FLS_KEY*) mem_calloc(sizeof(FLS_KEY), 1);
		fkey->locals = fifo_new();
		FlsSetValue(__fls_key, fkey);
		TlsSetValue(__tls_key, fkey);
	}
}

int pthread_key_create(pthread_key_t *key_ptr, void (*destructor)(void*))
{
	char kbuf[32];
	pthread_once(&__control_once, thread_once);

	assert(__tls_key != TLS_OUT_OF_INDEXES);
	assert(__fls_key != FLS_OUT_OF_INDEXES);

	if (*key_ptr <= 0 && *key_ptr != TLS_OUT_OF_INDEXES) {
		*key_ptr = TlsAlloc();
		assert(*key_ptr != TLS_OUT_OF_INDEXES);
	} else {
		hash_key(*key_ptr, kbuf, sizeof(kbuf));
		pthread_mutex_lock(&__lock);
		if (htable_find(__pkeys->keys, kbuf)) {
			pthread_mutex_unlock(&__lock);
			return 0;
		}
		*key_ptr = TlsAlloc();
		assert(*key_ptr != TLS_OUT_OF_INDEXES);
	}

	TLS_KEY *tkey = (TLS_KEY*) mem_calloc(sizeof(TLS_KEY), 1);
	tkey->key        = *key_ptr;
	tkey->destructor = destructor;

	hash_key(*key_ptr, kbuf, sizeof(kbuf));

	pthread_mutex_lock(&__lock);
	htable_enter(__pkeys->keys, kbuf, tkey);
	pthread_mutex_unlock(&__lock);
	return 0;
}

int pthread_setspecific(pthread_key_t key, void *value)
{
	if (key < 0 || key == TLS_OUT_OF_INDEXES) {
		msg_error("%s(%d): key(%d) invalid", __FUNCTION__, __LINE__, key);
		return EINVAL;
	}

	assert(__tls_key != TLS_OUT_OF_INDEXES);
	assert(__pkeys);

	char kbuf[32];
	hash_key(key, kbuf, sizeof(kbuf));

	pthread_mutex_lock(&__lock);
	TLS_KEY *tkey = (TLS_KEY*) htable_find(__pkeys->keys, kbuf);
	pthread_mutex_unlock(&__lock);

	if (tkey == NULL) {
		msg_error("%s(%d): no TLS_KEY for key=%d", __FUNCTION__, __LINE__, key);
		return EINVAL;
	}

	FLS_KEY *fkey = (FLS_KEY*) TlsGetValue(__tls_key);
	assert(fkey && fkey->locals);
	TLS_OBJ *obj = (TLS_OBJ*) mem_calloc(1, sizeof(TLS_OBJ));
	obj->key = key;
	obj->value = value;
	fkey->locals->push_back(fkey->locals, obj);

	if (!TlsSetValue(key, value)) {
		msg_error("%s(%d): TlsSetValue(key=%d) error(%s)",
			__FUNCTION__, __LINE__, key, last_serror());
		return -1;
	}

	return 0;
}

void *pthread_getspecific(pthread_key_t key)
{
	return TlsGetValue(key);
}

#endif /* USE_FLS */

/* Free the mutex */
int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
	if (mutex) {
		if (mutex->id) {
			CloseHandle(mutex->id);
			mutex->id = 0;
		}
		return 0;
	} else
		return -1;
}

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *mattr)
{
	const char *myname = "pthread_mutex_init";

	if (mutex == NULL) {
		msg_error("%s, %s(%d): input invalid",
			__FILE__, myname, __LINE__);
		return -1;
	}

	mutex->dynamic = 0;

	/* Create the mutex, with initial value signaled */
	mutex->id = CreateMutex((SECURITY_ATTRIBUTES *) mattr, FALSE, NULL);
	if (!mutex->id) {
		msg_error("%s, %s(%d): CreateMutex error(%s)",
			__FILE__, myname, __LINE__, last_serror());
		mem_free(mutex);
		return -1;
	}

	return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
	const char *myname = "pthread_mutex_lock";

	if (mutex == NULL) {
		msg_error("%s, %s(%d): input invalid",
			__FILE__, myname, __LINE__);
		return -1;
	}

	if (WaitForSingleObject(mutex->id, INFINITE) == WAIT_FAILED) {
		msg_error("%s, %s(%d): WaitForSingleObject error(%s)",
			__FILE__, myname, __LINE__, last_serror());
		return -1;
	}

	return 0;
}

int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
	const char *myname = "pthread_mutex_trylock";
	DWORD ret;

	if (mutex == NULL) {
		msg_error("%s, %s(%d): input invalid",
			__FILE__, myname, __LINE__);
		return -1;
	}

	ret = WaitForSingleObject(mutex->id, 0);
	if (ret == WAIT_TIMEOUT) {
		return FIBER_ETIME;
	} else if (ret == WAIT_FAILED) {
		msg_error("%s, %s(%d): WaitForSingleObject error(%s)",
			__FILE__, myname, __LINE__, last_serror());
		return -1;
	}

	return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	const char *myname = "pthread_mutex_unlock";

	if (mutex == NULL) {
		msg_error("%s, %s(%d): input invalid",
			__FILE__, myname, __LINE__);
		return -1;
	}

	if (ReleaseMutex(mutex->id) == FALSE) {
		msg_error("%s, %s(%d): ReleaseMutex error(%s)",
			__FILE__, myname, __LINE__, last_serror());
		return -1;
	}

	return 0;
}

long thread_self(void)
{
	return (long) GetCurrentThreadId();
}

#elif	defined(__linux__) || defined(COSMOCC)

#include <sys/syscall.h>

long thread_self(void)
{
	return (long) syscall(SYS_gettid);
}
#elif	defined(__APPLE__)
long thread_self(void)
{
	return (long) pthread_self();
}
#elif	defined(__FreeBSD__)
long thread_self(void)
{
#if defined(__FreeBSD__) && (__FreeBSD__ >= 9)
	return (long) pthread_getthreadid_np();
#else
	return (long) pthread_self();
#endif
}
#else
# error "Unknown OS"
#endif
