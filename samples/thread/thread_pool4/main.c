#include <getopt.h>
#include "lib_acl.h"

static void thread_run(void *arg)
{
	(void) arg;
}

int main(void)
{
	int   nthreads = 200, count = 100, i;
	acl_pthread_pool_t *thrpool;

	thrpool = acl_thread_pool_create(nthreads, 1);

	for (i = 0; i < nthreads; i++)
		acl_pthread_pool_add(thrpool, thread_run, &count);

	sleep(2);

	for (i = 0; i < nthreads; i++)
		acl_pthread_pool_add(thrpool, thread_run, &count);

	printf("enter any key to exit\r\n");
	getchar();
	acl_pthread_pool_destroy(thrpool);
	return 0;
}
