#include "lib_acl.h"
#include <assert.h>

/**
 * 用户自定义数据结构
 */
typedef struct THREAD_CTX {
	acl_pthread_pool_t *thr_pool;
	int   i;
} THREAD_CTX;

/* 全局性静态变量 */
static acl_pthread_pool_t *__thr_pool = NULL;

/* 线程局部存储变量(C99支持此种方式声明，方便许多) */
static __thread unsigned int __local = 0;

static void free_buf_fn(void *arg)
{
	ACL_VSTRING *buf = (ACL_VSTRING*) arg;

	printf(">> current thread id=%u, buf = %s\r\n",
		(unsigned int) acl_pthread_self(), acl_vstring_str(buf));
	acl_vstring_free(buf);
}

static void worker_thread(void *arg)
{
	THREAD_CTX *ctx = (THREAD_CTX*) arg; /* 获得用户自定义对象 */
	unsigned int   i = 0;
	static __thread ACL_VSTRING *buf1 = NULL;
	static __thread ACL_VSTRING *buf2 = NULL;

	/* 仅是验证参数传递过程 */
	assert(ctx->thr_pool == __thr_pool);

	if (buf1 == NULL)
		buf1 = acl_vstring_alloc(256);
	if (buf2 == NULL)
		buf2 = acl_vstring_alloc(256);

	acl_vstring_sprintf(buf1, "buf1: tid=%u",
		(unsigned int) acl_pthread_self());
	acl_vstring_sprintf(buf2, "buf2: tid=%u",
		(unsigned int) acl_pthread_self());
	/* 注册函数，当该线程退出时自动释放 buf 内存空间 */
	acl_pthread_atexit_add(buf1, free_buf_fn);
	acl_pthread_atexit_add(buf2, free_buf_fn);

	while (i < 5) {
		if (__local != i)
			acl_msg_fatal("__local=%d invalid", __local);
		printf("thread id=%u, i=%d, __local=%d\r\n",
			(unsigned int) acl_pthread_self(), ctx->i, __local);
		i++;
		/* 在本线程中将线程局部变量加1 */
		__local++;
		sleep(1);
	}

	acl_myfree(ctx);

	/* 至此，该工作线程进入空闲状态，直到空闲超时退出 */
}

static int on_thread_init(void *arg)
{
	const char *myname = "on_thread_init";
	acl_pthread_pool_t *thr_pool = (acl_pthread_pool_t*) arg;

	/* 判断一下，仅是为了验证参数传递过程 */
	assert(thr_pool == __thr_pool);
	printf("%s: thread(%u) init now\r\n", myname, (unsigned int) acl_pthread_self());

	/* 返回0表示继续执行该线程获得的新任务，返回-1表示停止执行该任务 */
	return (0);
}

static void on_thread_exit(void *arg)
{
	const char *myname = "on_thread_exit";
	acl_pthread_pool_t *thr_pool = (acl_pthread_pool_t*) arg;

	/* 判断一下，仅是为了验证参数传递过程 */
	assert(thr_pool == __thr_pool);
	printf("%s: thread(%u) exit now\r\n", myname, (unsigned int) acl_pthread_self());
}

static void run_thread_pool(acl_pthread_pool_t *thr_pool)
{
	THREAD_CTX *ctx;  /* 用户自定义参数 */

	/* 设置全局静态变量 */
	__thr_pool = thr_pool;

	/* 设置线程开始时的回调函数 */
	(void) acl_pthread_pool_atinit(thr_pool, on_thread_init, thr_pool);

	/* 设置线程退出时的回调函数 */
	(void) acl_pthread_pool_atfree(thr_pool, on_thread_exit, thr_pool);

	ctx = (THREAD_CTX*) acl_mycalloc(1, sizeof(THREAD_CTX));
	assert(ctx);
	ctx->thr_pool = thr_pool;
	ctx->i = 0;

	/**
	* 向线程池中添加第一个任务，即启动第一个工作线程
	* @param wq 线程池句柄
	* @param worker_thread 工作线程的回调函数
	* @param event_type 此处写0即可
	* @param ctx 用户定义参数
	*/
	acl_pthread_pool_add(thr_pool, worker_thread, ctx);
	sleep(1);

	ctx = (THREAD_CTX*) acl_mycalloc(1, sizeof(THREAD_CTX));
	assert(ctx);
	ctx->thr_pool = thr_pool;
	ctx->i = 1;
	/* 向线程池中添加第二个任务，即启动第二个工作线程 */
	acl_pthread_pool_add(thr_pool, worker_thread, ctx);
}

static void main_thread_atexit(void *arg)
{
	ACL_VSTRING *buf = (ACL_VSTRING*) arg;

	printf("main thread exit now, tid=%u, buf=%s\r\n",
		(unsigned int) acl_pthread_self(), acl_vstring_str(buf));
	printf("in the main thread_atexit, input any key to exit\r\n");
	getchar();
}

static acl_pthread_pool_t *thr_pool_create(int threads, int timeout)
{
	acl_pthread_pool_attr_t attr;
	acl_pthread_pool_t *thr_pool;

	acl_pthread_pool_attr_init(&attr);
	acl_pthread_pool_attr_set_threads_limit(&attr, threads);
	acl_pthread_pool_attr_set_idle_timeout(&attr, timeout);

	/* 创建半驻留线程句柄 */
	thr_pool = acl_pthread_pool_create(&attr);
	assert(thr_pool);
	return (thr_pool);
}

typedef struct {
	ACL_VSTREAM *fp;
	int  i;
} RUN_CTX;
static acl_pthread_mutex_t __mutex;
static int  __i = 0;
static void run_thread(void *arg)
{
	RUN_CTX *ctx = (RUN_CTX*) arg;

	acl_pthread_mutex_lock(&__mutex);
	if (0)
		acl_vstream_fprintf(ctx->fp, "hello world, id: %d, i: %d\n", ctx->i, __i++);
	else
		__i++;
	acl_pthread_mutex_unlock(&__mutex);
	acl_myfree(ctx);
}

static void test_thread_pool(void)
{
	acl_pthread_pool_t *thr_pool;
	ACL_VSTREAM *fp = acl_vstream_fopen("test.log", O_WRONLY | O_CREAT, 0600, 4096);
	int   i;

	acl_pthread_mutex_init(&__mutex, NULL);
	thr_pool = acl_thread_pool_create(100, 10);

	for (i = 0; i < 1000000; i++) {
		RUN_CTX *ctx = (RUN_CTX*) acl_mymalloc(sizeof(RUN_CTX));
		ctx->fp = fp;
		ctx->i = i;
		acl_pthread_pool_add(thr_pool, run_thread, ctx);
	}

	acl_pthread_pool_destroy(thr_pool);
	acl_pthread_mutex_destroy(&__mutex);
	acl_vstream_close(fp);
	printf("last i: %d\r\n", __i);
}

int main(int argc acl_unused, char *argv[] acl_unused)
{
	acl_pthread_pool_t *thr_pool;
	int  max_threads = 20;  /* 最多并发20个线程 */
	int  idle_timeout = 10; /* 每个工作线程空闲10秒后自动退出 */
	static __thread ACL_VSTRING *buf = NULL;

	if (1) {
		test_thread_pool();
		exit (0);
	}

	buf = acl_vstring_alloc(256);
	acl_vstring_sprintf(buf, "in main thread, id=%u",
		(unsigned int) acl_pthread_self());
	acl_pthread_atexit_add(buf, main_thread_atexit);


	thr_pool = thr_pool_create(max_threads, idle_timeout);
	run_thread_pool(thr_pool);

	if (0) {
		/* 如果立即运行 acl_pthread_pool_destroy，则由于调用了线程池销毁函数，
		 * 主线程便立刻通知空闲线程退出，所有空闲线程不必等待空闲超时时间便可退出,
		 */
		printf("> wait all threads to be idle and free thread pool\r\n");
		/* 立即销毁线程池 */
		acl_pthread_pool_destroy(thr_pool);
	} else {
		/* 因为不立即调用 acl_pthread_pool_destroy，所有所有空闲线程都是当空闲
		 * 超时时间到达后才退出
		 */
		while (1) {
			int   ret;

			ret = acl_pthread_pool_size(thr_pool);
			if (ret == 0)
				break;
			printf("> current threads in thread pool is: %d\r\n", ret);
			sleep(1);
		}
		/* 线程池中的工作线程数为0时销毁线程池 */
		printf("> all worker thread exit now\r\n");
		acl_pthread_pool_destroy(thr_pool);
	}

	/* 主线程等待用户在终端输入任意字符后退出 */
	printf("> enter any key to exit\r\n");
	getchar();

	return (0);
}
