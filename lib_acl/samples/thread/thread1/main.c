
#include "stdlib/acl_define_unix.h"
#ifdef ACL_UNIX
# ifndef	_GNU_SOURCE
#  define _GNU_SOURCE
# endif
# include <pthread.h>
#endif

#include "lib_acl.h"
#include <time.h>

typedef struct THREAD_CTX {
	int   i;
} THREAD_CTX;

#if 0
typedef volatile int pthread_spinlock_t;
extern int pthread_spin_init (pthread_spinlock_t *__lock, int __pshared);
extern int pthread_spin_lock (pthread_spinlock_t *__lock);
extern int pthread_spin_unlock (pthread_spinlock_t *__lock);
#endif

static void *test_thread_fn(void *arg)
{
	THREAD_CTX *ctx = (THREAD_CTX*) arg;

	ctx->i++;
	printf("current tid is: %lu\r\n", (unsigned long int) acl_pthread_self());

	return (ctx);
}

#ifdef ACL_UNIX
static pthread_spinlock_t __spin_lock;
#endif

static void *thread_nested_mutex(void *arg)
{
	acl_pthread_mutex_t *mutex = (acl_pthread_mutex_t*) arg;
	int   m = 2;
	int   i, n = 100000000;
	time_t begin, end;

#ifdef ACL_UNIX
	if (0) {
		printf("tid: %lu, begin spin lock\n", (unsigned long int) acl_pthread_self());

		time(&begin);
		for (i = 0; i < n; i++) {
			pthread_spin_lock(&__spin_lock);
			pthread_spin_unlock(&__spin_lock);
		}
		time(&end);
		printf("tid: %lu, lock over, spin, time: %ld\n",
			(unsigned long int) acl_pthread_self(), (long) end - begin);
	}
#endif

	if (1) {
		printf("tid: %lu, begin lock mutex\n",
			(unsigned long int) acl_pthread_self());
		time(&begin);
		for (i = 0; i < n; i++) {
			acl_thread_mutex_lock(mutex);
			acl_thread_mutex_unlock(mutex);
		}
		time(&end);
		printf("tid: %lu, lock mutex ok, nested: %d, sleep %d seconds, time: %ld\n",
			(unsigned long int) acl_pthread_self(),
			acl_thread_mutex_nested(mutex), m, (long) end - begin);
	}

	if (0) {
		sleep(m);
		printf("tid: %lu, wakeup, begin unlock\n",
			(unsigned long int) acl_pthread_self());
		for (i = 0; i < n; i++)
			acl_thread_mutex_unlock(mutex);
		time(&end);
		printf("tid: %lu, time: %ld, exit now\n",
			(unsigned long int) acl_pthread_self(), (long) end - begin);
	}

	return (NULL);
}

static void test_nested_mutex(void)
{
	acl_pthread_t tid;
	acl_pthread_attr_t attr;
	static acl_pthread_mutex_t mutex;

#ifdef ACL_UNIX
	pthread_spin_init(&__spin_lock, 0);
#endif

	acl_pthread_mutex_init(&mutex, NULL);
	acl_pthread_attr_init(&attr);
	acl_pthread_attr_setdetachstate(&attr, ACL_PTHREAD_CREATE_DETACHED);
	acl_pthread_create(&tid, &attr, thread_nested_mutex, &mutex);
	acl_pthread_create(&tid, &attr, thread_nested_mutex, &mutex);
}

#ifdef ACL_UNIX
static pthread_mutex_t mutex_init1 = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_init2 = PTHREAD_MUTEX_INITIALIZER;

static void test_thread(void)
{
	acl_pthread_t tid;
	acl_pthread_attr_t attr;
	THREAD_CTX ctx, *pctx;
	void *return_arg;
	unsigned int id;

	acl_pthread_mutex_lock(&mutex_init1);
	printf("lock mutex_init1 ok\n");
	acl_pthread_mutex_lock(&mutex_init2);
	printf("lock mutex_init2 ok\n");

	ctx.i = 0;
	acl_pthread_attr_init(&attr);
	acl_pthread_create(&tid, &attr, test_thread_fn, &ctx);
	acl_pthread_join(tid, (void**) &return_arg);
	pctx = (THREAD_CTX*) return_arg;
	memcpy(&id, &tid, sizeof(id));
	printf("ctx.i=%d, pctx->i=%d, the thread id is: %u\r\n",
		ctx.i, pctx->i, id);
}
#endif

static void free_vstring(void *arg)
{
	const char *myname = "free_vstring";
	ACL_VSTRING *buf = (ACL_VSTRING*) arg;

	acl_vstring_free(buf);
	printf("%s: tid=%d, free vstring ok\n", myname, (int) acl_pthread_self());
}

static void *test_tls_thread(void *arg acl_unused)
{
	const char *myname = "test_tls_thread";
	static acl_pthread_key_t key = -1;
	ACL_VSTRING *buf;
	time_t begin;
	int   i;
	unsigned long tid;
	const char *p = acl_last_serror();

	printf("%s(%d): last serror: %s, %p\n", myname, __LINE__, p, p);

	printf(">>>%s(%d)(tid=%d): key = %d\n", myname, __LINE__, (int) acl_pthread_self(), key);
	sleep(1);
	printf(">>>%s(%d)(tid=%d): key = %d\n", myname, __LINE__, (int) acl_pthread_self(), key);
	if (1) {
		time(&begin);
		for (i = 0; i < 10000000; i++) {
			buf = acl_pthread_tls_get(&key);
			if (buf == NULL) {
				printf(">>>%s(tid=%d): buf null, key = %d\n",
					myname, (int) acl_pthread_self(), key);
				buf = acl_vstring_alloc(256);
				acl_pthread_tls_set(key, buf, free_vstring);
			}
			tid = acl_pthread_self();
		}
		printf(">>>%s(%d)(tid=%d): time cose %d seconds, tid: %ld\n",
			myname, __LINE__, (int) acl_pthread_self(),
			(int) (time(NULL) - begin), tid);
		acl_vstring_sprintf(buf, "%s, tid=%lu", myname, (unsigned long) acl_pthread_self());
		printf(">>>%s: buf=%s\n", myname, acl_vstring_str(buf));
	} else {
		time(&begin);
		for (i = 0; i < 1000000; i++) {
			buf = acl_vstring_alloc(256);
			acl_vstring_free(buf);
		}
		printf(">>>%s(%d)(tid=%d): time cose %d seconds\n",
			myname, __LINE__, (int) acl_pthread_self(),
			(int) (time(NULL) - begin));
	}
	sleep(1);
	return (NULL);
}

static void test_tls(void)
{
	acl_pthread_attr_t attr;
	acl_pthread_t t1, t2;

	acl_pthread_attr_init(&attr);
	acl_pthread_create(&t1, &attr, test_tls_thread, NULL);
	acl_pthread_create(&t2, &attr, test_tls_thread, NULL);
	acl_pthread_join(t1, NULL);
	acl_pthread_join(t2, NULL);
	printf(">>> test_tls: over now\n");
}

static int __init_once_key = 0;
static void init_once(void)
{
	const char *myname = "init_once";

	__init_once_key++;
	printf("%s(%d, tid=%d): __init_once_key=%d\n",
		myname, __LINE__, (int) acl_pthread_self(), __init_once_key);
}

static void free_buf(void *ctx)
{
	ACL_VSTRING *buf = (ACL_VSTRING*) ctx;
	acl_vstring_free(buf);
	printf(">>>>%s(tid=%d): free vstring ok\n",
		__FUNCTION__, (int) acl_pthread_self());
}

static void free_vbuf(void *ctx)
{
	ACL_VSTRING *buf = (ACL_VSTRING*) ctx;
	acl_vstring_free(buf);
	printf(">>>>%s(tid=%d): free vstring ok\n",
		__FUNCTION__, (int) acl_pthread_self());
}

static acl_pthread_once_t once_control = ACL_PTHREAD_ONCE_INIT;
static void *test_once_thread(void *arg acl_unused)
{
	ACL_VSTRING *buf;
	static acl_pthread_key_t buf_key = -1;
	static __thread ACL_VSTRING *__vbuf = NULL;

	acl_pthread_once(&once_control, init_once);

	if (__vbuf == NULL) {
		__vbuf = acl_vstring_alloc(256);
		acl_pthread_atexit_add(__vbuf, free_vbuf);
	}

	buf = (ACL_VSTRING*) acl_pthread_tls_get(&buf_key);
	if (buf == NULL) {
		buf = acl_vstring_alloc(256);
		acl_pthread_tls_set(buf_key, buf, free_buf);
	}

	buf = (ACL_VSTRING*) acl_pthread_tls_get(&buf_key);
	if (buf == NULL)
		printf(">>>%s(tid=%d) buf null\n", __FUNCTION__, (int) acl_pthread_self());
	else
		printf(">>>%s(tid=%d) buf not null\n", __FUNCTION__, (int) acl_pthread_self());

	printf(">>>%s: in thread %d, last error: %s\n",
		__FUNCTION__, (int) acl_pthread_self(), acl_last_serror());

	return (NULL);
}

static void test_pthread_once(void)
{
	acl_pthread_attr_t attr;
	acl_pthread_t t1, t2;

	acl_pthread_attr_init(&attr);
	acl_pthread_attr_setstacksize(&attr, 10240000);
	acl_pthread_create(&t1, &attr, test_once_thread, NULL);
	acl_pthread_create(&t2, &attr, test_once_thread, NULL);
	acl_pthread_join(t1, NULL);
	acl_pthread_join(t2, NULL);
	printf(">>> test_pthread_once: over now, enter any key to exit...\n");
	getchar();
}

int main(int argc acl_unused, char *argv[] acl_unused)
{
	if (0) {
		test_pthread_once();
		return (0);
	}

	printf("%s(%d): last serror: %s, %p\n", __FUNCTION__, __LINE__,
		acl_last_serror(), acl_last_serror());
	if (0) {
		test_tls();
		return (0);
	}

#ifdef ACL_UNIX
	test_thread();
#endif

	printf(">>>>main tid: %u\n", (unsigned) acl_pthread_self());
	if (0)
		test_nested_mutex();

	while (1)
		sleep(10);
	return (0);
}

