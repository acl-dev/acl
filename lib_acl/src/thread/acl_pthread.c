#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#include <errno.h>
#include <string.h>
#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_msg.h"
#include "stdlib/acl_malloc.h"
#include "stdlib/acl_ring.h"
#include "thread/acl_pthread.h"
#include "init/acl_init.h"

#endif

#include "../private/private_fifo.h"

#ifdef	ACL_WINDOWS

/*--------------------  ACL_WINDOWS 下模拟实现 Posix 标准接口函数 ----------------*/

#include <process.h>

#define PTHREAD_STACK_MIN      16384

typedef struct {
	acl_pthread_key_t key;
	void (*destructor)(void *);
} TLS_KEY;

typedef struct {
	TLS_KEY *tls_key;
	void *value;
} TLS_VALUE;

static int __thread_inited = 0;
static acl_pthread_once_t __create_thread_control_once = ACL_PTHREAD_ONCE_INIT;
static TLS_KEY __tls_key_list[ACL_PTHREAD_KEYS_MAX];
static acl_pthread_mutex_t __thread_lock;
static acl_pthread_key_t __tls_value_list_key = ACL_TLS_OUT_OF_INDEXES;

static void tls_value_list_free(void);

void acl_pthread_end(void)
{
	static int __thread_ended = 0;
	int   i;

	tls_value_list_free();

	if (__thread_ended)
		return;

	__thread_ended = 1;
	acl_pthread_mutex_destroy(&__thread_lock);

	for (i = 0; i < ACL_PTHREAD_KEYS_MAX; i++) {
		if (__tls_key_list[i].key >= 0
			&& __tls_key_list[i].key < ACL_PTHREAD_KEYS_MAX)
		{
			TlsFree(__tls_key_list[i].key);
			__tls_key_list[i].key = ACL_TLS_OUT_OF_INDEXES;
		}
		__tls_key_list[i].destructor = NULL;
	}
}

/* 每个进程的唯一初始化函数 */

static void acl_pthread_init_once(void)
{
	const char *myname = "acl_pthread_init_once";
	int   i;

	acl_pthread_mutex_init(&__thread_lock, NULL);
	__thread_inited = 1;

	for (i = 0; i < ACL_PTHREAD_KEYS_MAX; i++) {
		__tls_key_list[i].destructor = NULL;
		__tls_key_list[i].key = ACL_TLS_OUT_OF_INDEXES;
	}

	__tls_value_list_key = TlsAlloc();
	if (__tls_value_list_key == ACL_TLS_OUT_OF_INDEXES)
		acl_msg_fatal("%s(%d): TlsAlloc error(%s)",
			myname, __LINE__, acl_last_serror());
	if (__tls_value_list_key < 0 || __tls_value_list_key
		>= ACL_PTHREAD_KEYS_MAX)
	{
		acl_msg_fatal("%s(%d): TlsAlloc error(%s), not in(%d, %d)",
			myname, __LINE__, acl_last_serror(),
			0, ACL_PTHREAD_KEYS_MAX);
	}

	__tls_key_list[__tls_value_list_key].destructor = NULL;
	__tls_key_list[__tls_value_list_key].key = __tls_value_list_key;
}

/* 获得线程局部变量链表 */

static ACL_FIFO *tls_value_list_get(void)
{
	ACL_FIFO *tls_value_list_ptr;

	tls_value_list_ptr = TlsGetValue(__tls_value_list_key);
	if (tls_value_list_ptr == NULL) {
		tls_value_list_ptr = private_fifo_new();
		TlsSetValue(__tls_value_list_key, tls_value_list_ptr);
	}
	return tls_value_list_ptr;
}

static void tls_value_list_on_free(void *ctx)
{
	acl_default_free(__FILE__, __LINE__, ctx);
}

static void tls_value_list_free(void)
{
	ACL_FIFO *tls_value_list_ptr;

	tls_value_list_ptr = TlsGetValue(__tls_value_list_key);
	if (tls_value_list_ptr != NULL) {
		TlsSetValue(__tls_value_list_key, NULL);
		private_fifo_free(tls_value_list_ptr, tls_value_list_on_free);
	}
}

#ifdef ACL_WIN32_STDC
static DWORD __stdcall RunThreadWrap(void *data)
#else
static DWORD WINAPI RunThreadWrap(LPVOID data)
#endif
{
	acl_pthread_t *thread = (acl_pthread_t *) data;
	void *return_arg;
	ACL_FIFO *tls_value_list_ptr = tls_value_list_get();
	unsigned long *tid = 0;

	/* 只是为了避免与主线程的 h_thread->handle = handle 产生冲突 */
	if (__thread_inited)
		acl_pthread_mutex_lock(&__thread_lock);
	if (__thread_inited)
		acl_pthread_mutex_unlock(&__thread_lock);

	thread->id = acl_pthread_self();

	return_arg = (void*) thread->start_routine(thread->routine_arg);

	/* 释放由 acl_pthread_setspecific 添加的线程局部变量 */
	while (1) {
		TLS_VALUE *tls_value = private_fifo_pop(tls_value_list_ptr);

		if (tls_value == NULL)
			break;

		if (tls_value->tls_key == NULL
			|| tls_value->tls_key->destructor == NULL
			|| tls_value->tls_key->key < 0
			|| tls_value->tls_key->key >= ACL_PTHREAD_KEYS_MAX)
		{
			acl_default_free(__FILE__, __LINE__, tls_value);
			continue;
		}
		tls_value->tls_key->destructor(tls_value->value);
		acl_default_free(__FILE__, __LINE__, tls_value);
	}

	private_fifo_free(tls_value_list_ptr, NULL);

	/* 如果线程创建时为分离方式则需要关闭线程句柄 */
	if (thread->detached) {
		if (!CloseHandle(thread->handle)) {
			acl_msg_error("close handle error(%s)", 
				acl_last_serror());
		}
	}

	acl_default_free(__FILE__, __LINE__, thread);
	return (DWORD) return_arg;
}

int  acl_pthread_create(acl_pthread_t *thread, acl_pthread_attr_t *attr,
	void *(*start_routine)(void *), void *arg)
{
	const char *myname = "acl_pthread_create";
	acl_pthread_t *h_thread;
	HANDLE handle;
	unsigned long id, flag;
	
	if (thread == NULL) {
		acl_msg_error("%s, %s(%d): input invalid",
			__FILE__, myname, __LINE__);
		acl_set_error(ACL_EINVAL);
		return ACL_EINVAL;
	}
	acl_pthread_once(&__create_thread_control_once, acl_pthread_init_once);
	memset(thread, 0, sizeof(acl_pthread_t));

	h_thread = acl_default_calloc(__FILE__, __LINE__,
			1, sizeof(acl_pthread_t));
	if (h_thread == NULL) {
		acl_msg_error("%s, %s(%d): calloc error(%s)",
			__FILE__, myname, __LINE__, acl_last_serror());
		acl_set_error(ACL_ENOMEM);
		return ACL_ENOMEM;
	}

	if (attr != NULL)
		h_thread->detached = attr->detached;
	else
		h_thread->detached = 1;
	h_thread->start_routine = start_routine;
	h_thread->routine_arg   = arg;

	if (__thread_inited) {
		acl_pthread_mutex_lock(&__thread_lock);
		flag = 0;
	} else
		flag = CREATE_SUSPENDED;

#ifdef ACL_WIN32_STDC
	h_thread->handle = handle = (HANDLE) _beginthreadex(NULL,
			attr ? (unsigned int) attr->stacksize : 0,
			RunThreadWrap,
			(void *) h_thread,
			flag,
			&id);
#else
	h_thread->handle = handle = CreateThread(NULL,
			attr ? attr->stacksize : 0,,
			RunThreadWrap,
			h_thread,
			flag,
			&id);
#endif

	if (__thread_inited)
		acl_pthread_mutex_unlock(&__thread_lock);
	else if (flag == CREATE_SUSPENDED && handle != 0)
		ResumeThread(handle);
	if (handle == 0) {
		acl_msg_error("%s, %s(%d): CreateThread error(%s)",
			__FILE__, myname, __LINE__, acl_last_serror());
		return -1;
	}
	thread->start_routine = start_routine;
	thread->routine_arg   = arg;
	thread->id            = id;
	thread->handle        = 0;

	/* 根据线程的属性来确定线程创建时是分离模式还是非分离模式 */

	if (attr == NULL || attr->detached) {
		thread->detached = 1;
		return 0;
	}

	thread->detached = 0;
	thread->handle = handle;
	return 0;
}

int acl_pthread_once(acl_pthread_once_t *once_control,
	void (*init_routine)(void))
{
	int   n = 0;

	if (once_control == NULL || init_routine == NULL) {
		acl_set_error(ACL_EINVAL);
		return ACL_EINVAL;
	}

	/* 只有第一个调用 InterlockedCompareExchange 的线程才会执行
	 * init_routine, 后续线程永远在 InterlockedCompareExchange
	 * 外运行，并且一直进入空循环直至第一个线程执行 init_routine
	 * 完毕并且将 *once_control 重新赋值, 只有在多核环境中多个线程
	 * 同时运行至此时才有可能出现短暂的后续线程空循环现象，如果
	 * 多个线程顺序至此，则因为 *once_control 已经被第一个线程重新
	 * 赋值而不会进入循环体内只所以如此处理，是为了保证所有线程在
	 * 调用 acl_pthread_once 返回前 init_routine 必须被调用且仅能
	 * 被调用一次, 但在VC6下，InterlockedCompareExchange 接口定义
	 * 有些怪异，需要做硬性指定参数类型，参见 <Windows 高级编程指南>
	 * Jeffrey Richter, 366 页
	 */
	while (1) {
#ifdef MS_VC6
		LONG prev = InterlockedCompareExchange((PVOID) once_control,
			(PVOID) 1, (PVOID) ACL_PTHREAD_ONCE_INIT);
#else
		LONG prev = InterlockedCompareExchange(
			once_control, 1, ACL_PTHREAD_ONCE_INIT);
#endif
		if (prev == 2)
			return 0;
		else if (prev == 0) {
			/* 只有第一个线程才会至此 */
			init_routine();
			/* 将 *conce_control 重新赋值以使后续线程不进入 while
			 * 循环或从 while 循环中跳出
			 */
			InterlockedExchange(once_control, 2);
			return 0;
		} else {
			acl_assert(prev == 1);

			/* 防止空循环过多地浪费CPU */
			Sleep(1);  /** sleep 1ms */
		}
	}
	return 1;  /* 不可达代码，避免编译器报警告 */
}

int acl_pthread_attr_init(acl_pthread_attr_t *thr_attr)
{
	if (thr_attr == NULL) {
		acl_set_error(ACL_EINVAL);
		return ACL_EINVAL;
	}

	memset(&thr_attr->attr, 0, sizeof(thr_attr->attr));
	thr_attr->attr.bInheritHandle = 1;
	thr_attr->attr.lpSecurityDescriptor = NULL;
	thr_attr->attr.nLength = sizeof(SECURITY_ATTRIBUTES);
	thr_attr->stacksize = 0;
	thr_attr->detached = 0;

	return 0;
}

int acl_pthread_attr_setstacksize(acl_pthread_attr_t *attr, size_t stacksize)
{
	if (attr == NULL) {
		acl_set_error(ACL_EINVAL);
		return ACL_EINVAL;
	}
	if (stacksize < PTHREAD_STACK_MIN) {
		acl_set_error(ACL_EINVAL);
		return ACL_EINVAL;
	}
	attr->stacksize = stacksize;
	return 0;
}

int acl_pthread_attr_setdetachstate(acl_pthread_attr_t *thr_attr, int detached)
{
	if (thr_attr == NULL) {
		acl_set_error(ACL_EINVAL);
		return ACL_EINVAL;
	}
	thr_attr->detached = detached;
	return 0;
}

int acl_pthread_attr_destroy(acl_pthread_attr_t *thr_attr)
{
	if (thr_attr == NULL) {
		acl_set_error(ACL_EINVAL);
		return ACL_EINVAL;
	}
	memset(&thr_attr->attr, 0, sizeof(thr_attr->attr));
	return 0;
}

unsigned long acl_pthread_self(void)
{
	return GetCurrentThreadId();
}

int acl_pthread_key_create(acl_pthread_key_t *key_ptr,
	void (*destructor)(void*))
{
	const char *myname = "acl_pthread_key_create";

	acl_pthread_once(&__create_thread_control_once, acl_pthread_init_once);

	*key_ptr = TlsAlloc();
	if (*key_ptr == ACL_TLS_OUT_OF_INDEXES) {
		acl_set_error(ACL_ENOMEM);
		return ACL_ENOMEM;
	} else if (*key_ptr >= ACL_PTHREAD_KEYS_MAX) {
		acl_msg_error("%s(%d): key(%d) > ACL_PTHREAD_KEYS_MAX(%d)",
			myname, __LINE__, *key_ptr, ACL_PTHREAD_KEYS_MAX);
		TlsFree(*key_ptr);
		*key_ptr = ACL_TLS_OUT_OF_INDEXES;
		acl_set_error(ACL_ENOMEM);
		return ACL_ENOMEM;
	}

	__tls_key_list[*key_ptr].destructor = destructor;
	__tls_key_list[*key_ptr].key = *key_ptr;
	return 0;
}

void *acl_pthread_getspecific(acl_pthread_key_t key)
{
	return TlsGetValue(key);
}

int acl_pthread_setspecific(acl_pthread_key_t key, void *value)
{
	const char *myname = "acl_pthread_setspecific";
	ACL_FIFO *tls_value_list_ptr = tls_value_list_get();
	ACL_ITER iter;

	if (key < 0 || key >= ACL_PTHREAD_KEYS_MAX) {
		acl_msg_error("%s(%d): key(%d) invalid",
			myname, __LINE__, key);
		acl_set_error(ACL_EINVAL);
		return ACL_EINVAL;
	}
	if (__tls_key_list[key].key != key) {
		acl_msg_error("%s(%d): __tls_key_list[%d].key(%d) != key(%d)",
			myname, __LINE__, key, __tls_key_list[key].key, key);
		acl_set_error(ACL_EINVAL);
		return ACL_EINVAL;
	}

	acl_foreach(iter, tls_value_list_ptr) {
		TLS_VALUE *tls_value = (TLS_VALUE*) iter.data;
		if (tls_value->tls_key != NULL
			&& tls_value->tls_key->key == key)
		{
			/* 如果相同的键存在则需要先释放旧数据 */
			if (tls_value->tls_key->destructor || tls_value->value)
				tls_value->tls_key->destructor(tls_value->value);
			tls_value->tls_key = NULL;
			tls_value->value = NULL;
			break;
		}
	}

	if (TlsSetValue(key, value)) {
		TLS_VALUE *tls_value = (TLS_VALUE*)
			acl_default_malloc(__FILE__, __LINE__,
				sizeof(TLS_VALUE));
		tls_value->tls_key = &__tls_key_list[key];
		tls_value->value = value;
		private_fifo_push(tls_value_list_ptr, tls_value);
		return 0;
	} else {
		acl_msg_error("%s(%d): TlsSetValue(key=%d) error(%s)",
			myname, __LINE__, key, acl_last_serror());
		return -1;
	}
}

int acl_pthread_detach(acl_pthread_t thread)
{
	const char *myname = "acl_pthread_detach";

	if (thread.detached)
		return -1;
	if (thread.handle == 0)
		return -1;

	if (!CloseHandle(thread.handle)) {
		acl_msg_error("close handle error(%s)", acl_last_serror());
	}
	return 0;
}

int acl_pthread_join(acl_pthread_t thread, void **thread_return)
{
	const char *myname = "acl_pthread_join";
	void *return_arg;

	if (thread.detached) {
		acl_msg_error("%s(%d): thread has been detached",
			myname, __LINE__);
		return -1;
	}
	if (thread.handle == 0) {
		acl_msg_error("%s(%d): thread->handle == 0", myname, __LINE__);
		return -1;
	}

	WaitForSingleObject(thread.handle, INFINITE);
	if (GetExitCodeThread(thread.handle, (LPDWORD) &return_arg)) {
		if (thread_return != NULL)
			*thread_return = return_arg;
	}

	if (!CloseHandle(thread.handle)) {
		acl_msg_error("close handle error(%s)", 
			acl_last_serror());
	}
	return 0;
}

#endif /* ACL_WINDOWS */

/*----------------- 跨平台的通用函数集，是 Posix 标准的扩展 ----------------*/

/*--------------------------------------------------------------------------*/

typedef struct pthread_atexit {
	void   (*free_fn)(void *);
	void   *arg;
} pthread_atexit_t;

static acl_pthread_key_t __pthread_atexit_key = (acl_pthread_key_t ) ACL_TLS_OUT_OF_INDEXES;
static acl_pthread_once_t __pthread_atexit_control_once = ACL_PTHREAD_ONCE_INIT;

static void pthread_atexit_done(void *arg) 
{
	ACL_FIFO *id_list = (ACL_FIFO*) arg;
	pthread_atexit_t *id_ptr;

	while (1) {
		id_ptr = (pthread_atexit_t*) private_fifo_pop(id_list);
		if (id_ptr == NULL)
			break;
		if (id_ptr->free_fn)
			id_ptr->free_fn(id_ptr->arg);
		acl_default_free(__FILE__, __LINE__, id_ptr);
	}
	private_fifo_free(id_list, NULL);
}

static void pthread_atexit_init(void)
{
	acl_pthread_key_create(&__pthread_atexit_key, pthread_atexit_done);
}

int acl_pthread_atexit_add(void *arg, void (*free_fn)(void *))
{
	const char *myname = "acl_pthread_atexit_add";
	pthread_atexit_t *id;
	ACL_FIFO *id_list;

	if (arg == NULL) {
		acl_set_error(ACL_EINVAL);
		return ACL_EINVAL;
	}
	acl_pthread_once(&__pthread_atexit_control_once, pthread_atexit_init);
	if (__pthread_atexit_key == (acl_pthread_key_t) ACL_TLS_OUT_OF_INDEXES) {
		acl_msg_error("%s(%d): __pthread_atexit_key(%d) invalid",
			myname, __LINE__, (int) __pthread_atexit_key);
		return -1;
	}

	id = (pthread_atexit_t*) acl_default_malloc(__FILE__,
		__LINE__, sizeof(pthread_atexit_t));
	if (id == NULL) {
		acl_msg_error("%s(%d): malloc error(%s)",
			myname, __LINE__, acl_last_serror());
		acl_set_error(ACL_ENOMEM);
		return ACL_ENOMEM;
	}
	id->free_fn = free_fn;
	id->arg = arg;

	id_list = (ACL_FIFO*) acl_pthread_getspecific(__pthread_atexit_key);
	if (id_list == NULL) {
		id_list = private_fifo_new();
		if (acl_pthread_setspecific(__pthread_atexit_key, id_list) != 0) {
			acl_msg_error("%s(%d): pthread_setspecific: %s, key(%d)",
				myname, __LINE__, acl_last_serror(),
				(int) __pthread_atexit_key);
			return -1;
		}
	}
	private_fifo_push(id_list, id);
	return 0;
}

int acl_pthread_atexit_remove(void *arg, void (*free_fn)(void*))
{
	const char *myname = "acl_pthread_atexit_remove";
	ACL_FIFO *id_list;
	ACL_ITER iter;

	if (arg == NULL) {
		acl_set_error(ACL_EINVAL);
		return -1;
	}
	if (__pthread_atexit_key == (acl_pthread_key_t) ACL_TLS_OUT_OF_INDEXES) {
		acl_msg_error("%s(%d): __pthread_atexit_key(%d)  invalid",
			myname, __LINE__, (int) __pthread_atexit_key);
		acl_set_error(ACL_EINVAL);
		return -1;
	}
	id_list = (ACL_FIFO*) acl_pthread_getspecific(__pthread_atexit_key);
	if (id_list == NULL) {
		acl_msg_error("%s(%d): __pthread_atexit_key(%d) no exist"
			" in tid(%lu)", myname, __LINE__,
			(int) __pthread_atexit_key,
			(unsigned long) acl_pthread_self());
		return -1;
	}

	acl_foreach(iter, id_list) {
		pthread_atexit_t *id_ptr = (pthread_atexit_t*) iter.data;

		if (id_ptr->free_fn == free_fn && id_ptr->arg == arg) {
			ACL_FIFO_INFO *id_info = acl_iter_info(iter, id_list);
			private_delete_info(id_list, id_info);
			acl_default_free(__FILE__, __LINE__, id_ptr);
			break;
		}
	}
	return 0;
}

/*----------------------------------------------------------------------------*/

typedef struct {
	acl_pthread_key_t key;
	void *ptr;
	void (*free_fn)(void*);
} TLS_CTX;

static int acl_tls_ctx_max = 1024;
static acl_pthread_once_t __tls_ctx_control_once = ACL_PTHREAD_ONCE_INIT;
static acl_pthread_key_t __tls_ctx_key = (acl_pthread_key_t) ACL_TLS_OUT_OF_INDEXES;
static TLS_CTX *__main_tls_ctx = NULL;

int acl_pthread_tls_set_max(int max)
{
	if (max <= 0) {
		acl_set_error(ACL_EINVAL);
		return ACL_EINVAL;
	} else {
		acl_tls_ctx_max = max;
		return 0;
	}
}

int acl_pthread_tls_get_max(void)
{
	return acl_tls_ctx_max;
}

/* 线程退出时调用此函数释放属于本线程的局部变量 */

static void tls_ctx_free(void *ctx)
{
	TLS_CTX *tls_ctxes = (TLS_CTX*) ctx;
	int   i;

	for (i = 0; i < acl_tls_ctx_max; i++) {
		if (tls_ctxes[i].ptr != NULL && tls_ctxes[i].free_fn != NULL) {
			tls_ctxes[i].free_fn(tls_ctxes[i].ptr);
		}
	}
	acl_default_free(__FILE__, __LINE__, tls_ctxes);
}

/* 主线程退出时释放局部变量 */

static void main_tls_ctx_free(void)
{
	if (__main_tls_ctx)
		tls_ctx_free(__main_tls_ctx);
}

static void dummy_free(void *ctx acl_unused)
{
}

static void tls_ctx_once_init(void)
{
	if ((unsigned long) acl_pthread_self() ==
		(unsigned long) acl_main_thread_self())
	{
		acl_pthread_key_create(&__tls_ctx_key, dummy_free);
		atexit(main_tls_ctx_free);
	} else
		acl_pthread_key_create(&__tls_ctx_key, tls_ctx_free);
}

void *acl_pthread_tls_get(acl_pthread_key_t *key_ptr)
{
	const char *myname = "acl_pthread_tls_get";
	TLS_CTX *tls_ctxes;
	int   i;

	acl_pthread_once(&__tls_ctx_control_once, tls_ctx_once_init);
	if (__tls_ctx_key == (acl_pthread_key_t) ACL_TLS_OUT_OF_INDEXES) {
		acl_msg_error("%s(%d): __tls_ctx_key invalid, tid(%lu)",
			myname, __LINE__, (unsigned long) acl_pthread_self());
		return NULL;
	}
	tls_ctxes = (TLS_CTX*) acl_pthread_getspecific(__tls_ctx_key);
	if (tls_ctxes == NULL) {
		/* 因为该线程中不存在该线程局部变量，所以需要分配一个新的 */
		tls_ctxes = (TLS_CTX*) acl_default_malloc(__FILE__, __LINE__,
				acl_tls_ctx_max * sizeof(TLS_CTX));
		if (acl_pthread_setspecific(__tls_ctx_key, tls_ctxes) != 0) {
			acl_default_free(__FILE__, __LINE__, tls_ctxes);
			acl_msg_error("%s(%d): pthread_setspecific: %s, tid(%lu)",
				myname, __LINE__, acl_last_serror(),
				(unsigned long) acl_pthread_self());
			return NULL;
		}
		/* 初始化 */
		for (i = 0; i < acl_tls_ctx_max; i++) {
			tls_ctxes[i].key = (acl_pthread_key_t) ACL_TLS_OUT_OF_INDEXES;
			tls_ctxes[i].ptr = NULL;
			tls_ctxes[i].free_fn = NULL;
		}

		if ((unsigned long) acl_pthread_self()
			== (unsigned long) acl_main_thread_self())
		{
			__main_tls_ctx = tls_ctxes;
		}
	}

	/* 如果该键已经存在则取出对应数据 */
	if ((int) (*key_ptr) >= 0 && (int) (*key_ptr) < acl_tls_ctx_max) {
		if (tls_ctxes[(int) (*key_ptr)].key == *key_ptr)
			return tls_ctxes[(int) (*key_ptr)].ptr;
		if (tls_ctxes[(int) (*key_ptr)].key
			== (acl_pthread_key_t) ACL_TLS_OUT_OF_INDEXES)
		{
			tls_ctxes[(int) (*key_ptr)].key = *key_ptr;
			return tls_ctxes[(int) (*key_ptr)].ptr;
		}
		acl_msg_warn("%s(%d): tls_ctxes[%d].key(%d)!= key(%d)",
			myname, __LINE__, (int) (*key_ptr),
			(int) tls_ctxes[(int) (*key_ptr)].key, (int) (*key_ptr));
		return NULL;
	}

	/* 找出一个空位 */
	for (i = 0; i < acl_tls_ctx_max; i++) {
		if (tls_ctxes[i].key == (acl_pthread_key_t) ACL_TLS_OUT_OF_INDEXES)
			break;
	}

	/* 如果没有空位可用则返回空并置错误标志位 */
	if (i == acl_tls_ctx_max) {
		acl_msg_error("%s(%d): no space for tls key", myname, __LINE__);
		*key_ptr = (acl_pthread_key_t) ACL_TLS_OUT_OF_INDEXES;
		acl_set_error(ACL_ENOMEM);
		return NULL;
	}

	/* 为新分配的键初始化线程局部数据对象 */
	tls_ctxes[i].key = (acl_pthread_key_t) i;
	tls_ctxes[i].free_fn = NULL;
	tls_ctxes[i].ptr = NULL;
	*key_ptr = (acl_pthread_key_t) i;
	return NULL;
}

int acl_pthread_tls_set(acl_pthread_key_t key, void *ptr,
	void (*free_fn)(void *))
{
	const char *myname = "acl_pthread_tls_set";
	TLS_CTX *tls_ctxes;

	if ((int) key < 0 || (int) key >= acl_tls_ctx_max) {
		acl_msg_error("%s(%d): key(%d) invalid",
			myname, __LINE__, (int) key);
		acl_set_error(ACL_EINVAL);
		return ACL_EINVAL;
	}

	if (__tls_ctx_key == (acl_pthread_key_t) ACL_TLS_OUT_OF_INDEXES) {
		acl_msg_error("%s(%d): __tls_ctx_key invalid, tid(%lu)",
			myname, __LINE__, (unsigned long) acl_pthread_self());
		acl_set_error(ACL_ENOMEM);
		return ACL_ENOMEM;
	}
	tls_ctxes = (TLS_CTX*) acl_pthread_getspecific(__tls_ctx_key);
	if (tls_ctxes == NULL) {
		acl_msg_error("%s(%d): __tls_ctx_key(%d) no exist",
			myname, __LINE__, (int) __tls_ctx_key);
		return -1;
	}
	if (tls_ctxes[(int) key].key != key) {
		acl_msg_error("%s(%d): key(%d) invalid",
			myname, __LINE__, (int) key);
		acl_set_error(ACL_EINVAL);
		return ACL_EINVAL;
	}
	/* 如果该键值存在旧数据则首先需要释放掉旧数据 */
	if (tls_ctxes[(int) key].ptr != NULL && tls_ctxes[(int) key].free_fn != NULL)
		tls_ctxes[(int) key].free_fn(tls_ctxes[(int) key].ptr);

	tls_ctxes[(int) key].free_fn = free_fn;
	tls_ctxes[(int) key].ptr = ptr;
	return 0;
}

int acl_pthread_tls_del(acl_pthread_key_t key)
{
	const char *myname = "acl_pthread_tls_del";
	TLS_CTX *tls_ctxes;

	if ((int) key < 0 || (int) key >= acl_tls_ctx_max) {
		acl_msg_error("%s(%d): key(%d) invalid",
			myname, __LINE__, (int) key);
		acl_set_error(ACL_EINVAL);
		return ACL_EINVAL;
	}

	if (__tls_ctx_key == (acl_pthread_key_t) ACL_TLS_OUT_OF_INDEXES) {
		acl_msg_error("%s(%d): __tls_ctx_key invalid, tid(%lu)",
			myname, __LINE__, (unsigned long) acl_pthread_self());
		acl_set_error(ACL_ENOMEM);
		return ACL_ENOMEM;
	}

	tls_ctxes = (TLS_CTX*) acl_pthread_getspecific(__tls_ctx_key);
	if (tls_ctxes == NULL) {
		acl_msg_error("%s(%d): __tls_ctx_key(%d) no exist",
			myname, __LINE__, (int) __tls_ctx_key);
		return -1;
	}

	if (tls_ctxes[(int) key].key != key) {
		acl_msg_error("%s(%d): key(%d) invalid",
			myname, __LINE__, (int) key);
		acl_set_error(ACL_EINVAL);
		return ACL_EINVAL;
	}

	tls_ctxes[(int) key].free_fn = NULL;
	tls_ctxes[(int) key].ptr = NULL;
	tls_ctxes[(int) key].key = (acl_pthread_key_t) ACL_TLS_OUT_OF_INDEXES;
	return 0;
}

void acl_pthread_tls_once_get(acl_pthread_once_t *control_once)
{
	memcpy(control_once, &__tls_ctx_control_once,
		sizeof(acl_pthread_once_t));
}

void acl_pthread_tls_once_set(acl_pthread_once_t control_once)
{
	__tls_ctx_control_once = control_once;
}

acl_pthread_key_t acl_pthread_tls_key_get(void)
{
	return __tls_ctx_key;
}

void acl_pthread_tls_key_set(acl_pthread_key_t key)
{
	__tls_ctx_key = key;
}
