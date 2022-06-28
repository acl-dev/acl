#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_stdlib.h"
#include "event/acl_events.h"
#include "thread/acl_pthread_pool.h"
#include "net/acl_net.h"
#include "ioctl/acl_ioctl.h"

#endif

#include "ioctl_internal.h"

static void worker_callback_r(void *context)
{
	ACL_IOCTL_CTX *ctx = (ACL_IOCTL_CTX *) context;
	ACL_IOCTL *ioc = ctx->ioc;
	ACL_VSTREAM *stream = ctx->stream;
	ACL_IOCTL_NOTIFY_FN notify_fn = ctx->notify_fn;
	void *arg = ctx->context;

	notify_fn(ctx->event_type, ioc, stream, arg);
}

void read_notify_callback_r(int event_type, ACL_EVENT *event acl_unused,
	ACL_VSTREAM *stream acl_unused, void *context)
{
	ACL_IOCTL_CTX *ctx = (ACL_IOCTL_CTX *) context;
	ACL_IOCTL *ioc = ctx->ioc;

	ctx->event_type = event_type;

	switch (event_type) {
	case ACL_EVENT_READ:
	case ACL_EVENT_RW_TIMEOUT:
	case ACL_EVENT_XCPT:
		acl_pthread_pool_add(ioc->tp, worker_callback_r, ctx);
		break;
	default:
		acl_msg_fatal("%s(%d): unknown event type(%d)",
			__FILE__, __LINE__, event_type);
		/* not reached */
		break;
	}
}

void write_notify_callback_r(int event_type, ACL_EVENT *event acl_unused,
	ACL_VSTREAM *stream acl_unused, void *context)
{
	ACL_IOCTL_CTX *ctx = (ACL_IOCTL_CTX *) context;
	ACL_IOCTL *ioc = ctx->ioc;

	ctx->event_type = event_type;

	switch (event_type) {
	case ACL_EVENT_WRITE:
	case ACL_EVENT_RW_TIMEOUT:
	case ACL_EVENT_XCPT:
		acl_pthread_pool_add(ioc->tp, worker_callback_r, ctx);
		break;
	default:
		acl_msg_fatal("%s(%d): unknown event type(%d)",
			__FILE__, __LINE__, event_type);
		/* not reached */
		break;
	}
}

void listen_notify_callback_r(int event_type, ACL_EVENT *event acl_unused,
	ACL_VSTREAM *stream acl_unused, void *context)
{
	ACL_IOCTL_CTX *ctx= (ACL_IOCTL_CTX *) context;
	ACL_IOCTL *ioc = ctx->ioc;
	ACL_IOCTL_NOTIFY_FN notify_fn = ctx->notify_fn;
	void *arg = ctx->context;

	ctx->event_type = event_type;

	switch (event_type) {
	case ACL_EVENT_READ:
	case ACL_EVENT_RW_TIMEOUT:
	case ACL_EVENT_XCPT:
		notify_fn(event_type, ioc, stream, arg);
		break;
	default:
		acl_msg_fatal("%s(%d): unknown event type(%d)",
			__FILE__, __LINE__, event_type);
		/* not reached */
		break;
	}
}

static void worker_ready_callback(void *context)
{
	ACL_IOCTL_CTX *ctx = (ACL_IOCTL_CTX *) context;
	ACL_IOCTL *ioc = ctx->ioc;
	ACL_IOCTL_WORKER_FN callback = ctx->worker_fn;
	void *arg = ctx->context;

	acl_myfree(ctx);
	callback(ioc, arg);
}

int acl_ioctl_add(ACL_IOCTL *ioc, ACL_IOCTL_WORKER_FN callback, void *arg)
{
	const char *myname = "acl_ioctl_add";
	ACL_IOCTL_CTX *ctx;

	if (ioc == NULL || ioc->tp == NULL)
		acl_msg_fatal("%s(%d): input invalid", myname, __LINE__);

	ctx = acl_mymalloc(sizeof(ACL_IOCTL_CTX));
	ctx->ioc       = ioc;
	ctx->worker_fn = callback;
	ctx->context   = arg;

	acl_pthread_pool_add(ioc->tp, worker_ready_callback, ctx);
	return 0;
}

int acl_ioctl_nworker(ACL_IOCTL *ioc)
{
	return acl_pthread_pool_size(ioc->tp);
}
