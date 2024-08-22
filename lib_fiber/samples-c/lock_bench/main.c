#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include "fiber/libfiber.h"

static int __locking_count   = 100;

static void fiber_lock(ACL_FIBER *fiber, void *ctx)
{
	ACL_FIBER_LOCK *l = (ACL_FIBER_LOCK *) ctx;
	int i;

	for (i = 0; i < __locking_count; i++) {
		acl_fiber_lock_lock(l);
		acl_fiber_lock_unlock(l);
	}

	printf("fiber-%d over, count=%d\r\n", acl_fiber_id(fiber), i);
}

static void fiber_mutex(ACL_FIBER *fiber, void *ctx)
{
	ACL_FIBER_MUTEX *m = (ACL_FIBER_MUTEX *) ctx;
	int i;

	for (i = 0; i < __locking_count; i++) {
		acl_fiber_mutex_lock(m);
		acl_fiber_mutex_unlock(m);
	}

	printf("fiber-%d over, count=%d\r\n", acl_fiber_id(fiber), i);
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		" -M [if use ACL_FIBER_MUTEX]\r\n"
		" -c fibers_count\r\n"
		" -n lock_count\r\n", procname);
}

int main(int argc, char *argv[])
{
	int  ch, fibers_count = 10, i, use_mutex = 0;
	ACL_FIBER_LOCK  *l = acl_fiber_lock_create();
	ACL_FIBER_MUTEX *m = acl_fiber_mutex_create(0);

	while ((ch = getopt(argc, argv, "hc:n:M")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'c':
			fibers_count = atoi(optarg);
			break;
		case 'n':
			__locking_count = atoi(optarg);
			break;
		case 'M':
			use_mutex = 1;
			break;
		default:
			break;
		}
	}

	for (i = 0; i < fibers_count; i++) {
		if (use_mutex) {
			acl_fiber_create(fiber_mutex, m, 320000);
		} else {
			acl_fiber_create(fiber_lock, l, 320000);
		}
	}

	acl_fiber_schedule();

	printf("All over, fibers_count=%d, locking_count=%d\r\n",
		fibers_count, __locking_count);

	acl_fiber_lock_free(l);
	acl_fiber_mutex_free(m);

	return 0;
}
