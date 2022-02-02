#include "lib_acl.h"
#include <time.h>

static int __init_once_key = 0;
static void init_once(void)
{
	__init_once_key++;
	printf("%s(%d, tid=%d): __init_once_key=%d\n",
		myname, __LINE__, (int) acl_pthread_self(), __init_once_key);
}

static acl_pthread_once_t once_control = ACL_PTHREAD_ONCE_INIT;

static void *test_once_thread(void *arg acl_unused)
{
	ACL_VSTRING *buf;
	static acl_pthread_key_t buf_key = -1;
	static __thread ACL_VSTRING *__vbuf = NULL;

	acl_pthread_once(&once_control, init_once);

	return NULL;
}

static void test_pthread_once(void)
{
	acl_pthread_t t1, t2;

	acl_pthread_create(&t1, NULL, test_once_thread, NULL);
	acl_pthread_create(&t2, NULL, test_once_thread, NULL);
	acl_pthread_join(t1, NULL);
	acl_pthread_join(t2, NULL);
}

int main(int argc acl_unused, char *argv[] acl_unused)
{
	test_pthread_once();

	return 0;
}
