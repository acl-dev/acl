#include "stdafx.h"
#include "fifo.h"
#include "memory.h"
#include "msg.h"
#include "iterator.h"
#include "pthread_patch.h"

#ifdef SYS_WIN

typedef struct {
	pthread_key_t key;
	void (*destructor)(void *);
} TLS_KEY;

typedef struct {
	TLS_KEY *tls_key;
	void *value;
} TLS_VALUE;

static int     __thread_inited = 0;
static TLS_KEY __tls_key_list[PTHREAD_KEYS_MAX];
static pthread_mutex_t __thread_lock;
static pthread_key_t   __tls_value_list_key         = TLS_OUT_OF_INDEXES;
static pthread_once_t  __create_thread_control_once = PTHREAD_ONCE_INIT;

static void tls_value_list_free(void);

void pthread_end(void)
{
	static int __thread_ended = 0;
	int   i;

	tls_value_list_free();

	if (__thread_ended)
		return;

	__thread_ended = 1;
	pthread_mutex_destroy(&__thread_lock);

	for (i = 0; i < PTHREAD_KEYS_MAX; i++) {
		if (__tls_key_list[i].key >= 0
			&& __tls_key_list[i].key < PTHREAD_KEYS_MAX)
		{
			TlsFree(__tls_key_list[i].key);
			__tls_key_list[i].key = TLS_OUT_OF_INDEXES;
		}
		__tls_key_list[i].destructor = NULL;
	}
}

/* 每个进程的唯一初始化函数 */

static void pthread_init_once(void)
{
	const char *myname = "pthread_init_once";
	int   i;

	pthread_mutex_init(&__thread_lock, NULL);
	__thread_inited = 1;

	for (i = 0; i < PTHREAD_KEYS_MAX; i++) {
		__tls_key_list[i].destructor = NULL;
		__tls_key_list[i].key        = TLS_OUT_OF_INDEXES;
	}

	__tls_value_list_key = TlsAlloc();
	if (__tls_value_list_key == TLS_OUT_OF_INDEXES)
		msg_fatal("%s(%d): TlsAlloc error(%s)",
			myname, __LINE__, last_serror());
	if (__tls_value_list_key < 0
		|| __tls_value_list_key >= PTHREAD_KEYS_MAX)
	{
		msg_fatal("%s(%d): TlsAlloc error(%s), not in(%d, %d)",
			myname, __LINE__, last_serror(),
			0, PTHREAD_KEYS_MAX);
	}

	__tls_key_list[__tls_value_list_key].destructor = NULL;
	__tls_key_list[__tls_value_list_key].key = __tls_value_list_key;
}

/* 获得线程局部变量链表 */

static FIFO *tls_value_list_get(void)
{
	FIFO *tls_value_list_ptr;

	tls_value_list_ptr = (FIFO*) TlsGetValue(__tls_value_list_key);
	if (tls_value_list_ptr == NULL) {
		tls_value_list_ptr = fifo_new();
		TlsSetValue(__tls_value_list_key, tls_value_list_ptr);
	}
	return tls_value_list_ptr;
}

static void tls_value_list_on_free(void *ctx)
{
	mem_free(ctx);
}

static void tls_value_list_free(void)
{
	FIFO *tls_value_list_ptr;

	tls_value_list_ptr = (FIFO*) TlsGetValue(__tls_value_list_key);
	if (tls_value_list_ptr != NULL) {
		TlsSetValue(__tls_value_list_key, NULL);
		fifo_free(tls_value_list_ptr, tls_value_list_on_free);
	}
}

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

int pthread_key_create(pthread_key_t *key_ptr, void (*destructor)(void*))
{
	const char *myname = "pthread_key_create";

	pthread_once(&__create_thread_control_once, pthread_init_once);

	*key_ptr = TlsAlloc();
	if (*key_ptr == TLS_OUT_OF_INDEXES) {
		return ENOMEM;
	} else if (*key_ptr >= PTHREAD_KEYS_MAX) {
		msg_error("%s(%d): key(%d) > PTHREAD_KEYS_MAX(%d)",
			myname, __LINE__, *key_ptr, PTHREAD_KEYS_MAX);
		TlsFree(*key_ptr);
		*key_ptr = TLS_OUT_OF_INDEXES;
		return ENOMEM;
	}

	__tls_key_list[*key_ptr].destructor = destructor;
	__tls_key_list[*key_ptr].key = *key_ptr;
	return 0;
}

void *pthread_getspecific(pthread_key_t key)
{
	return TlsGetValue(key);
}

int pthread_setspecific(pthread_key_t key, void *value)
{
	const char *myname = "pthread_setspecific";
	FIFO *tls_value_list_ptr = tls_value_list_get();
	ITER iter;

	if (key < 0 || key >= PTHREAD_KEYS_MAX) {
		msg_error("%s(%d): key(%d) invalid", myname, __LINE__, key);
		return EINVAL;
	}
	if (__tls_key_list[key].key != key) {
		msg_error("%s(%d): __tls_key_list[%d].key(%d) != key(%d)",
			myname, __LINE__, key, __tls_key_list[key].key, key);
		return EINVAL;
	}

	foreach(iter, tls_value_list_ptr) {
		TLS_VALUE *tls_value = (TLS_VALUE*) iter.data;
		if (tls_value->tls_key != NULL
			&& tls_value->tls_key->key == key) {

			/* 如果相同的键存在则需要先释放旧数据 */
			if (tls_value->tls_key->destructor && tls_value->value)
				tls_value->tls_key->destructor(tls_value->value);
			tls_value->tls_key = NULL;
			tls_value->value = NULL;
			break;
		}
	}

	if (TlsSetValue(key, value)) {
		TLS_VALUE *tls_value = (TLS_VALUE*) mem_malloc(sizeof(TLS_VALUE));
		tls_value->tls_key = &__tls_key_list[key];
		tls_value->value = value;
		fifo_push(tls_value_list_ptr, tls_value);
		return 0;
	} else {
		msg_error("%s(%d): TlsSetValue(key=%d) error(%s)",
			myname, __LINE__, key, last_serror());
		return -1;
	}
}

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

unsigned long __pthread_self(void)
{
	return GetCurrentThreadId();
}

#elif	defined(__linux__) || defined(__APPLE__)
unsigned long __pthread_self(void)
{
	return (unsigned long) pthread_self();
}
#elif	defined(__FreeBSD__)
unsigned long __pthread_self(void)
{
#if defined(__FreeBSD__) && (__FreeBSD__ >= 9)
	return (unsigned long) pthread_getthreadid_np();
#else
	return (unsigned long) pthread_self();
#endif
}
#else
#error	"unknown OS"
#endif
