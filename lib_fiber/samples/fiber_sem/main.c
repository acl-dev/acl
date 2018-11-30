#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include "fiber/lib_fiber.h"

static int __fibers_count = 10;

static void fiber_main(ACL_FIBER *fiber acl_unused, void *ctx)
{
	ACL_FIBER_SEM *sem = (ACL_FIBER_SEM *) ctx;
	int left;

	printf("fiber-%d begin to sem_wait\r\n", acl_fiber_self());
	left = acl_fiber_sem_wait(sem);
	printf("fiber-%d sem_wait ok, left: %d\r\n", acl_fiber_self(), left);

	printf("fiber-%d begin sleep\r\n", acl_fiber_self());
	acl_fiber_sleep(1);
	printf("fiber-%d wakeup\r\n", acl_fiber_self());

	left = acl_fiber_sem_post(sem);
	printf("fiber-%d sem_post ok, left: %d\r\n", acl_fiber_self(), left);

	if (--__fibers_count == 0) {
		printf("--- All fibers Over ----\r\n");
		acl_fiber_schedule_stop();
	}
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -n sem_max -c fibers_count\r\n", procname);
}

int main(int argc, char *argv[])
{
	int  ch, nfibers = __fibers_count, i, sem_max = 2;
	ACL_FIBER_SEM *sem;

	while ((ch = getopt(argc, argv, "hn:c:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			sem_max = atoi(optarg);
			break;
		case 'c':
			nfibers = atoi(optarg);
			break;
		default:
			break;
		}
	}

	__fibers_count = nfibers;
	sem = acl_fiber_sem_create(sem_max);

	for (i = 0; i < nfibers; i++)
		acl_fiber_create(fiber_main, sem, 320000);

	acl_fiber_schedule();
	acl_fiber_sem_free(sem);

	return 0;
}
