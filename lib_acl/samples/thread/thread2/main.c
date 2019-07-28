#include "stdlib/acl_define_unix.h"
#ifdef ACL_UNIX
# ifndef	_GNU_SOURCE
#  define _GNU_SOURCE
# endif
# include <pthread.h>
#endif

#include "lib_acl.h"
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>

static void *thread_main(void *arg acl_unused)
{
	pid_t pid;
	acl_pthread_mutex_t *__lock;
	int   ret;
	char  ebuf[256];

	__lock = (acl_pthread_mutex_t*) acl_mycalloc(1,
			sizeof(acl_pthread_mutex_t));
	acl_pthread_mutex_init(__lock, NULL);

	printf("current pid: %d, tid: %lu\r\n", getpid(), acl_pthread_self());

	if ((ret = acl_pthread_mutex_lock(__lock)) == 0)
		printf("0: main thread(%lu) lock ok\r\n", acl_pthread_self());
	else
		printf("0: main thread(%lu) lock error(%d:%s)\r\n",
			acl_pthread_self(), ret,
			acl_strerror(ret, ebuf, sizeof(ebuf)));

	switch ((pid = fork())) {
	case 0:
		/* 先解锁 */
		if ((ret = acl_pthread_mutex_unlock(__lock)) == 0)
			printf("1: child thread(%lu) unlock ok\r\n",
				acl_pthread_self());
		else
			printf("1: child thread(%lu) unlock error(%d:%s)\r\n",
				acl_pthread_self(), ret,
				acl_strerror(ret, ebuf, sizeof(ebuf)));

		/* 再加锁 */
		if ((ret = acl_pthread_mutex_lock(__lock)) == 0)
			printf("1: child thread(%lu) lock ok\r\n",
				acl_pthread_self());
		else
			printf("1: child thread(%lu) lock error(%d:%s)\r\n",
				acl_pthread_self(), ret,
				acl_strerror(ret, ebuf, sizeof(ebuf)));

		/* 再释放锁--该函数会报错 */
		if ((ret = acl_pthread_mutex_destroy(__lock)) == 0)
			printf("1: child thread(%lu) destroy ok\r\n",
				acl_pthread_self());
		else
			printf("1: child thread(%lu) destroy error(%d:%s)\r\n",
				acl_pthread_self(), ret,
				acl_strerror(ret, ebuf, sizeof(ebuf)));
		acl_myfree(__lock);
		exit (0);

	case -1:
		printf("fork failed\r\n");
		exit (0);
	default:
		printf("0: parent, child pid: %d, tid: %lu\r\n",
			getpid(), acl_pthread_self());
		sleep(1);

		{
			int  status;
			pid_t pp;
			pp = wait(&status);
			printf("wait's pid: %d, pid: %d, status: %d\r\n",
				pp, pid, status);
		}

		if ((ret = acl_pthread_mutex_destroy(__lock)) == 0)
			printf("0: parent thread(%lu) destroy lock ok\r\n",
				acl_pthread_self());
		else
			printf("0: parent thread(%lu) destroy err(%d:%s)\r\n",
				acl_pthread_self(), ret,
				acl_strerror(ret, ebuf, sizeof(ebuf)));
		acl_myfree(__lock);
		break;
	}

	return NULL;
}

static void test_thread(void)
{
	acl_pthread_attr_t attr;
	acl_pthread_t t1;

	acl_pthread_attr_init(&attr);
	acl_pthread_attr_setstacksize(&attr, 10240000);
	acl_pthread_create(&t1, &attr, thread_main, NULL);
	acl_pthread_join(t1, NULL);
	printf(">>> test_pthread_once: over now, enter any key to exit...\n");
	getchar();
}

int main(int argc acl_unused, char *argv[] acl_unused)
{
	test_thread();
	return (0);
}
