#include "lib_acl.h"
#include <time.h>

static int __init_once_key = 0;

static void init_once(void)
{
	printf("%s: tid=%ld, sleep 1000 ms\r\n",
		__FUNCTION__, (long) acl_pthread_self());
	acl_doze(1000);
	__init_once_key++;
	printf("%s: tid=%ld: __init_once_key=%d\r\n",
		__FUNCTION__, (long int) acl_pthread_self(), __init_once_key);
}

static acl_pthread_once_t once_control = ACL_PTHREAD_ONCE_INIT;

static void *test_once_thread(void *arg acl_unused)
{
	acl_pthread_once(&once_control, init_once);

	printf("%s: tid=%ld: __init_once_key=%d\r\n",
		__FUNCTION__, (long int) acl_pthread_self(), __init_once_key);
	return NULL;
}

static void test_pthread_once(void)
{
	acl_pthread_t t1, t2;

	acl_pthread_create(&t1, NULL, test_once_thread, NULL);
	acl_pthread_create(&t2, NULL, test_once_thread, NULL);
	acl_pthread_join(t1, NULL);
	acl_pthread_join(t2, NULL);
	printf("At last, __init_once_key=%d\r\n", __init_once_key);
}

int main(int argc acl_unused, char *argv[] acl_unused)
{
	acl_pthread_mutexattr_t attr;
	acl_pthread_mutex_t mutex;

	acl_pthread_mutexattr_init(&attr);
	acl_pthread_mutexattr_settype(&attr, ACL_PTHREAD_MUTEX_RECURSIVE);
	acl_pthread_mutex_init(&mutex, &attr);
	acl_pthread_mutexattr_destroy(&attr);

	acl_pthread_mutex_lock(&mutex);
	acl_pthread_mutex_lock(&mutex);
	acl_pthread_mutex_lock(&mutex);
	acl_pthread_mutex_destroy(&mutex);

	printf("lock ok\r\n");
	test_pthread_once();
	return 0;
}
