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

/*--------------------  ACL_WINDOWS ��ģ��ʵ�� Posix ��׼�ӿں��� ----------------*/

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

/* ÿ�����̵�Ψһ��ʼ������ */

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

/* ����ֲ߳̾��������� */

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

	/* ֻ��Ϊ�˱��������̵߳� h_thread->handle = handle ������ͻ */
	if (__thread_inited)
		acl_pthread_mutex_lock(&__thread_lock);
	if (__thread_inited)
		acl_pthread_mutex_unlock(&__thread_lock);

	thread->id = acl_pthread_self();

	return_arg = (void*) thread->start_routine(thread->routine_arg);

	/* �ͷ��� acl_pthread_setspecific ��ӵ��ֲ߳̾����� */
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

	/* ����̴߳���ʱΪ���뷽ʽ����Ҫ�ر��߳̾�� */
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

	/* �����̵߳�������ȷ���̴߳���ʱ�Ƿ���ģʽ���ǷǷ���ģʽ */

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

	/* ֻ�е�һ������ InterlockedCompareExchange ���̲߳Ż�ִ��
	 * init_routine, �����߳���Զ�� InterlockedCompareExchange
	 * �����У�����һֱ�����ѭ��ֱ����һ���߳�ִ�� init_routine
	 * ��ϲ��ҽ� *once_control ���¸�ֵ, ֻ���ڶ�˻����ж���߳�
	 * ͬʱ��������ʱ���п��ܳ��ֶ��ݵĺ����߳̿�ѭ���������
	 * ����߳�˳�����ˣ�����Ϊ *once_control �Ѿ�����һ���߳�����
	 * ��ֵ���������ѭ������ֻ������˴�����Ϊ�˱�֤�����߳���
	 * ���� acl_pthread_once ����ǰ init_routine ���뱻�����ҽ���
	 * ������һ��, ����VC6�£�InterlockedCompareExchange �ӿڶ���
	 * ��Щ���죬��Ҫ��Ӳ��ָ���������ͣ��μ� <Windows �߼����ָ��>
	 * Jeffrey Richter, 366 ҳ
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
			/* ֻ�е�һ���̲߳Ż����� */
			init_routine();
			/* �� *conce_control ���¸�ֵ��ʹ�����̲߳����� while
			 * ѭ����� while ѭ��������
			 */
			InterlockedExchange(once_control, 2);
			return 0;
		} else {
			acl_assert(prev == 1);

			/* ��ֹ��ѭ��������˷�CPU */
			Sleep(1);  /** sleep 1ms */
		}
	}
	return 1;  /* ���ɴ���룬��������������� */
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
			/* �����ͬ�ļ���������Ҫ���ͷž����� */
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

/*----------------- ��ƽ̨��ͨ�ú��������� Posix ��׼����չ ----------------*/

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

/* �߳��˳�ʱ���ô˺����ͷ����ڱ��̵߳ľֲ����� */

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

/* ���߳��˳�ʱ�ͷžֲ����� */

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
		/* ��Ϊ���߳��в����ڸ��ֲ߳̾�������������Ҫ����һ���µ� */
		tls_ctxes = (TLS_CTX*) acl_default_malloc(__FILE__, __LINE__,
				acl_tls_ctx_max * sizeof(TLS_CTX));
		if (acl_pthread_setspecific(__tls_ctx_key, tls_ctxes) != 0) {
			acl_default_free(__FILE__, __LINE__, tls_ctxes);
			acl_msg_error("%s(%d): pthread_setspecific: %s, tid(%lu)",
				myname, __LINE__, acl_last_serror(),
				(unsigned long) acl_pthread_self());
			return NULL;
		}
		/* ��ʼ�� */
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

	/* ����ü��Ѿ�������ȡ����Ӧ���� */
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

	/* �ҳ�һ����λ */
	for (i = 0; i < acl_tls_ctx_max; i++) {
		if (tls_ctxes[i].key == (acl_pthread_key_t) ACL_TLS_OUT_OF_INDEXES)
			break;
	}

	/* ���û�п�λ�����򷵻ؿղ��ô����־λ */
	if (i == acl_tls_ctx_max) {
		acl_msg_error("%s(%d): no space for tls key", myname, __LINE__);
		*key_ptr = (acl_pthread_key_t) ACL_TLS_OUT_OF_INDEXES;
		acl_set_error(ACL_ENOMEM);
		return NULL;
	}

	/* Ϊ�·���ļ���ʼ���ֲ߳̾����ݶ��� */
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
	/* ����ü�ֵ���ھ�������������Ҫ�ͷŵ������� */
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
	memcpy((void *)control_once, (void *)&__tls_ctx_control_once,
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
