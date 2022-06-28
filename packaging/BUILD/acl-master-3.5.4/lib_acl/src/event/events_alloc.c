#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdlib.h>
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_mystring.h"
#include "event/acl_events.h"

#endif

#include "events.h"

static void event_default_loop(ACL_EVENT *eventp)
{
	const char *myname = "event_default_loop";
	acl_msg_fatal("%s(%d): Not supported in %s",
		myname, __LINE__, eventp->name);
}

static void event_default_enable_read(ACL_EVENT *eventp,
	ACL_VSTREAM *stream acl_unused, int timeout acl_unused,
	ACL_EVENT_NOTIFY_RDWR callback acl_unused, void *context acl_unused)
{
	const char *myname = "event_default_enable_read";
	acl_msg_fatal("%s(%d): Not supported in %s",
		myname, __LINE__, eventp->name);
}

static void event_default_enable_write(ACL_EVENT *eventp,
	ACL_VSTREAM *stream acl_unused, int timeout acl_unused,
	ACL_EVENT_NOTIFY_RDWR callback acl_unused, void *context acl_unused)
{
	const char *myname = "event_default_enable_write";
	acl_msg_fatal("%s(%d): Not supported in %s",
		myname, __LINE__, eventp->name);
}

static void event_default_disable_read(ACL_EVENT *eventp,
	ACL_VSTREAM *stream acl_unused)
{
	const char *myname = "event_default_disable_read";
	acl_msg_fatal("%s(%d): Not supported in %s",
		myname, __LINE__, eventp->name);
}

static void event_default_disable_write(ACL_EVENT *eventp,
	ACL_VSTREAM *stream acl_unused)
{
	const char *myname = "event_default_disable_write";
	acl_msg_fatal("%s(%d): Not supported in %s",
		myname, __LINE__, eventp->name);
}

static void event_default_disable_readwrite(ACL_EVENT *eventp,
	ACL_VSTREAM *stream acl_unused)
{
	const char *myname = "event_default_disable_readwrite";
	acl_msg_fatal("%s(%d): Not supported in %s",
		myname, __LINE__, eventp->name);
}

static int event_default_isrset(ACL_EVENT *eventp,
	ACL_VSTREAM *stream acl_unused)
{
	const char *myname = "event_default_isrset";
	acl_msg_fatal("%s(%d): Not supported in %s",
		myname, __LINE__, eventp->name);
	return (0);
}

static int event_default_iswset(ACL_EVENT *eventp,
	ACL_VSTREAM *stream acl_unused)
{
	const char *myname = "event_default_iswset";
	acl_msg_fatal("%s(%d): Not supported in %s",
		myname, __LINE__, eventp->name);
	return (0);
}

static int event_default_isxset(ACL_EVENT *eventp,
	ACL_VSTREAM *stream acl_unused)
{
	const char *myname = "event_default_isxset";
	acl_msg_fatal("%s(%d): Not supported %s",
		myname, __LINE__, eventp->name);
	return (0);
}

static acl_int64 event_default_timer_request(ACL_EVENT *eventp,
	ACL_EVENT_NOTIFY_TIME callback acl_unused, void *context acl_unused,
	acl_int64 delay acl_unused, int keep acl_unused)
{
	const char *myname = "event_default_request_timer";
	acl_msg_fatal("%s(%d): Not supported in %s",
		myname, __LINE__, eventp->name);
	return (0);
}

static acl_int64 event_default_timer_cancel(ACL_EVENT *eventp,
	ACL_EVENT_NOTIFY_TIME callback acl_unused, void *context acl_unused)
{
	const char *myname = "event_default_cancel_timer";
	acl_msg_fatal("%s(%d): Not supported in %s",
		myname, __LINE__, eventp->name);
	return (0);
}

static void event_default_timer_keep(ACL_EVENT *eventp,
	ACL_EVENT_NOTIFY_TIME callback acl_unused,
	void * ctx acl_unused, int keep acl_unused)
{
	const char *myname = "event_default_cancel_timer";
	acl_msg_fatal("%s(%d): Not supported in %s",
		myname, __LINE__, eventp->name);
}

static int event_default_timer_ifkeep(ACL_EVENT *eventp,
	ACL_EVENT_NOTIFY_TIME callback acl_unused, void * ctx acl_unused)
{
	const char *myname = "event_default_timer_ifkeep";
	acl_msg_fatal("%s(%d): Not supported in %s",
		myname, __LINE__, eventp->name);
	return 0;
}

static void event_default_add_dog(ACL_EVENT *eventp)
{
	const char *myname = "event_default_add_dog";
	acl_msg_fatal("%s(%d): Not supported in %s",
		myname, __LINE__, eventp->name);
}

static void event_default_free(ACL_EVENT *eventp)
{
	acl_myfree(eventp);
}

ACL_EVENT *event_alloc(size_t size)
{
	const char *myname = "event_alloc";
	ACL_EVENT *eventp;

	if (size < sizeof(ACL_EVENT))
		acl_msg_fatal("%s(%d): size(%d) too small",
			myname, __LINE__, (int) size);
	eventp = acl_mycalloc(1, size);

	ACL_SAFE_STRNCPY(eventp->name, "events_alloc", sizeof(eventp->name));
	eventp->init_fn              = NULL;
	eventp->loop_fn              = event_default_loop;
	eventp->free_fn              = event_default_free;
	eventp->add_dog_fn           = event_default_add_dog;
	eventp->enable_read_fn       = event_default_enable_read;
	eventp->enable_write_fn      = event_default_enable_write;
	eventp->enable_listen_fn     = event_default_enable_read;
	eventp->disable_read_fn      = event_default_disable_read;
	eventp->disable_write_fn     = event_default_disable_write;
	eventp->disable_readwrite_fn = event_default_disable_readwrite;
	eventp->isrset_fn            = event_default_isrset;
	eventp->iswset_fn            = event_default_iswset;
	eventp->isxset_fn            = event_default_isxset;
	eventp->timer_request        = event_default_timer_request;
	eventp->timer_cancel         = event_default_timer_cancel;
	eventp->timer_keep           = event_default_timer_keep;
	eventp->timer_ifkeep         = event_default_timer_ifkeep;

	return eventp;
}
