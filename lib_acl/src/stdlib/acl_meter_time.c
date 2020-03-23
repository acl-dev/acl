#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_sys_patch.h"
#include "init/acl_init.h"
#include "thread/acl_pthread.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef	ACL_UNIX
#include <sys/time.h>
#endif
#include "stdlib/acl_meter_time.h"

#endif

static void dummy(void *ptr acl_unused)
{

}

static void free_tls(void *ptr)
{
	acl_myfree(ptr);
}

static void *__tls = NULL;

#if !defined(HAVE_NO_ATEXIT)
static void main_free_tls(void)
{
	if (__tls) {
		acl_myfree(__tls);
		__tls = NULL;
	}
}
#endif

static acl_pthread_key_t  once_key;
static void once_init(void)
{
	if ((unsigned long) acl_pthread_self() == acl_main_thread_self()) {
		acl_pthread_key_create(&once_key, dummy);
#if !defined(HAVE_NO_ATEXIT)
		atexit(main_free_tls);
#endif
	} else
		acl_pthread_key_create(&once_key, free_tls);
}

static acl_pthread_once_t once_control = ACL_PTHREAD_ONCE_INIT;
static void *tls_calloc(size_t len)
{
	void *ptr;

	(void) acl_pthread_once(&once_control, once_init);
	ptr = (void*) acl_pthread_getspecific(once_key);
	if (ptr == NULL) {
		ptr = acl_mycalloc(1, len);
		acl_pthread_setspecific(once_key, ptr);
		if ((unsigned long) acl_pthread_self() == acl_main_thread_self())
			__tls = ptr;
	}
	return ptr;
}

typedef struct {
	struct timeval stamp;
	int   init_done;
} METER_CTX_T;

double acl_meter_time(const char *filename, int line, const char *info)
{
	struct timeval now;
	double  f;
	METER_CTX_T *ctx = tls_calloc(sizeof(METER_CTX_T));

	if (ctx->init_done == 0) {
		ctx->init_done = 1;
		gettimeofday(&ctx->stamp, NULL);
	}

	gettimeofday(&now, NULL);
	now.tv_usec -= ctx->stamp.tv_usec;
	if (now.tv_usec < 0) {
		--now.tv_sec;
		now.tv_usec += 1000000;
	}
	now.tv_sec -= ctx->stamp.tv_sec;

	f = now.tv_sec * 1000.0 + now.tv_usec/1000.0;
	if (info)
		printf("tid=%lu, %s(%d), %s: time inter = %8.3f ms\r\n",
			(unsigned long) acl_pthread_self(), filename, line, info, f);
	else
		printf("tid=%lu, %s(%d): time inter = %8.3f ms\r\n",
			(unsigned long) acl_pthread_self(), filename, line, f);

	gettimeofday(&ctx->stamp, NULL);
	return (f);
}
