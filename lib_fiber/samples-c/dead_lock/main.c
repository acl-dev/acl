#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include "fiber/libfiber.h"

typedef struct {
	ACL_FIBER_MUTEX **locks;
	size_t nlocks;
} MUTEX_CTX;

static void fiber_main(ACL_FIBER *fb, void *arg)
{
	MUTEX_CTX *ctx = (MUTEX_CTX*) arg;
	int i = acl_fiber_id(fb) % ctx->nlocks;
	ACL_FIBER_MUTEX *lock = ctx->locks[i];

	printf("fiber-%d wait, locks[%d]=%p\r\n", acl_fiber_self(), i, lock);
	acl_fiber_mutex_lock(lock);
	printf("fiber-%d locked, locks[%d]=%p\r\n", acl_fiber_self(), i, lock);

	acl_fiber_sleep(1);

	i = (acl_fiber_id(fb) + 1) % ctx->nlocks;
	lock = ctx->locks[i];

	printf("fiber-%d wait,locks[%d]=%p\r\n", acl_fiber_self(), i, lock);
	acl_fiber_mutex_lock(lock);
	printf("fiber-%d locked, locks[%d]=%p\r\n", acl_fiber_self(), i, lock);

	free(ctx);
}

static void *thread_main(void *arg)
{
	MUTEX_CTX *ctx = (MUTEX_CTX*) arg;
	ACL_FIBER_MUTEX *lock;

	free(ctx); return NULL;

	sleep(1);

	lock = ctx->locks[0];

	printf("thread-%lu begin to lock %p\r\n", (long) pthread_self(), lock);
	acl_fiber_mutex_lock(ctx->locks[0]);
	printf("thread-%lu lock %p ok\r\n", (long) pthread_self(), lock);

	lock = ctx->locks[1 % ctx->nlocks];
	printf("thread-%lu begin to lock %p\r\n", (long) pthread_self(), lock);
	acl_fiber_mutex_lock(lock);
	printf("thread-%lu lock %p ok\r\n", (long) pthread_self(), lock);

	free(ctx);
	return NULL;
}

static void fiber_check(ACL_FIBER *fb, void *ctx)
{
	(void) fb;
	(void) ctx;

	while (1) {
		ACL_FIBER_MUTEX_STATS *stats;

		sleep(1);
		printf("\r\n");
		//acl_fiber_mutex_profile();
		stats = acl_fiber_mutex_deadlock();
		if (stats) {
			acl_fiber_mutex_stats_show(stats);
			acl_fiber_mutex_stats_free(stats);
		} else {
			printf("No deadlock happened!\r\n");
		}
		printf("===============================================\r\n");
	}
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -c fibers_count -n locks_count\r\n", procname);
}

int main(int argc, char *argv[])
{
	int  i, ch, nlocks = 2, nfibers = 2;
	ACL_FIBER_MUTEX **locks;
	MUTEX_CTX *ctx;
	pthread_t  tid;

	while ((ch = getopt(argc, argv, "hc:n:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'c':
			nfibers = atoi(optarg);
			break;
		case 'n':
			nlocks = atoi(optarg);
			if (nlocks <= 0) {
				nlocks = 1;
			}
			break;
		default:
			break;
		}
	}

	locks = (ACL_FIBER_MUTEX**) malloc(nlocks * sizeof(ACL_FIBER_MUTEX*));
 
	for (i = 0; i < nlocks; i++) {
		locks[i] = acl_fiber_mutex_create(0);
	}

	for (i = 0; i < nfibers; i++) {
		ctx = (MUTEX_CTX*) malloc(sizeof(MUTEX_CTX));
		ctx->locks  = locks;
		ctx->nlocks = nlocks;
		acl_fiber_create(fiber_main, ctx, 32000);
	}

	acl_fiber_create(fiber_check, NULL, 32000);

	ctx = (MUTEX_CTX*) malloc(sizeof(MUTEX_CTX));
	ctx->locks  = locks;
	ctx->nlocks =  nlocks;

	pthread_create(&tid, NULL, thread_main, ctx);

	acl_fiber_schedule();

	pthread_join(tid, NULL);

	for (i = 0; i < nlocks; i++) {
		acl_fiber_mutex_free(locks[i]);
	}

	free(locks);
	return 0;
}
