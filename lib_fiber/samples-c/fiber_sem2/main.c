#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include "fiber/lib_fiber.h"

static void fiber_waiter(ACL_FIBER *fiber acl_unused, void *ctx)
{
	ACL_FIBER_SEM *sem = (ACL_FIBER_SEM *) ctx;
	int ret, timeo = 2000;
	time_t begin, end;

	printf("fiber-%d: wait %d milliseconds\r\n", acl_fiber_self(), timeo);
	begin = time(NULL);
	ret = acl_fiber_sem_timed_wait(sem, timeo);
	end = time(NULL);

	if (ret >= 0) {
		printf("fiber-%d: wait ok, tc: %ld seconds\r\n",
			acl_fiber_self(), end - begin);
	} else if (errno == EAGAIN) {
		printf("fiber-%d: wait timed out, error=%s, tc: %ld seconds\r\n",
			acl_fiber_self(), strerror(errno), end - begin);
	} else {
		printf("%s: wait error, error=%s, tc: %ld seconds\r\n",
			__FUNCTION__, strerror(errno), end - begin);
	}
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n", procname);
}

int main(int argc, char *argv[])
{
	int  ch;
	ACL_FIBER_SEM *sem;

	while ((ch = getopt(argc, argv, "h")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		default:
			break;
		}
	}


	sem = acl_fiber_sem_create(0);

	acl_fiber_create(fiber_waiter, sem, 128000);

	acl_fiber_schedule();

	acl_fiber_sem_free(sem);

	printf("--- All fibers Over ----\r\n");
	return 0;
}
