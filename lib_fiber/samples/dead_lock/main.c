#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include "fiber/libfiber.h"

typedef struct {
	ACL_FIBER_MUTEX *locks[2];
} MUTEX_CTX;

static void fiber_main(ACL_FIBER *fb, void *arg)
{
	MUTEX_CTX *ctx = (MUTEX_CTX*) arg;
	ACL_FIBER_MUTEX *lock = ctx->locks[acl_fiber_id(fb) % 2];

	acl_fiber_mutex_lock(lock);
	printf("fiber-%d lock %p ok\r\n", acl_fiber_self(), lock);

	acl_fiber_sleep(1);

	lock = ctx->locks[(acl_fiber_id(fb) + 1) % 2];

	printf("fiber-%d begin to lock %p\r\n", acl_fiber_self(), lock);
	acl_fiber_mutex_lock(lock);
	printf("fiber-%d lock %p ok\r\n", acl_fiber_self(), lock);
}

static void *thread_main(void *arg)
{
	MUTEX_CTX *ctx = (MUTEX_CTX*) arg;

	sleep(1);
	printf("thread-%lu begin to lock %p\r\n", pthread_self(), ctx->locks[0]);
	acl_fiber_mutex_lock(ctx->locks[0]);
	printf("thread-%lu lock %p ok\r\n", pthread_self(), ctx->locks[0]);

	printf("thread-%lu begin to lock %p\r\n", pthread_self(), ctx->locks[1]);
	acl_fiber_mutex_lock(ctx->locks[1]);
	printf("thread-%lu lock %p ok\r\n", pthread_self(), ctx->locks[1]);

	return NULL;
}

static void fiber_profile(ACL_FIBER *fb, void *ctx)
{
	(void) fb;
	(void) ctx;

	while (1) {
		sleep(1);
		printf("\r\n");
		acl_fiber_mutex_profile();
	}
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n", procname);
}

int main(int argc, char *argv[])
{
	int  ch;
	MUTEX_CTX ctx;
	pthread_t tid;

	ctx.locks[0] = acl_fiber_mutex_create(0);
	ctx.locks[1] = acl_fiber_mutex_create(0);

	while ((ch = getopt(argc, argv, "h")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		default:
			break;
		}
	}
 
	acl_fiber_create(fiber_main, &ctx, 32000);
	acl_fiber_create(fiber_main, &ctx, 32000);
	acl_fiber_create(fiber_profile, NULL, 32000);

	pthread_create(&tid, NULL, thread_main, &ctx);

	acl_fiber_schedule();

	acl_fiber_mutex_free(ctx.locks[0]);
	acl_fiber_mutex_free(ctx.locks[1]);

	pthread_join(tid, NULL);
	return 0;
}
