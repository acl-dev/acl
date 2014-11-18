#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdlib.h>
#include <stdio.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_msg.h"
#include "net/acl_vstream_net.h"
#include "stdlib/acl_iostuff.h"
#include "event/acl_events.h"

#endif

#include "events_dog.h"

struct EVENT_DOG {
	ACL_EVENT *eventp; 
	ACL_VSTREAM *sstream;
	ACL_VSTREAM *server;
	ACL_VSTREAM *client;
	int   thread_mode;
};

/* forward declare */

static void event_dog_reopen(EVENT_DOG *evdog);

static void event_dog_close(EVENT_DOG *evdog)
{
	if (evdog->sstream)
		acl_vstream_close(evdog->sstream);
	if (evdog->server)
		acl_vstream_close(evdog->server);
	if (evdog->client) {
		if (!evdog->thread_mode)
			acl_event_disable_read(evdog->eventp, evdog->client);
		acl_vstream_close(evdog->client);
	}

	evdog->sstream = NULL;
	evdog->server = NULL;
	evdog->client = NULL;
}

static void read_fn(int event_type acl_unused, ACL_EVENT *event,
	ACL_VSTREAM *stream, void *context)
{
	const char *myname = "read_fn";
	EVENT_DOG *evdog = (EVENT_DOG*) context;
	char  buf[2];

	if (evdog->client != stream)
		acl_msg_fatal("%s(%d), %s: stream != evdog->client",
			__FILE__, __LINE__, myname);

	evdog->client->rw_timeout = 1;
	if (acl_vstream_readn(evdog->client, buf, 1) == ACL_VSTREAM_EOF) {
	        acl_event_disable_read(event, stream);
		event_dog_reopen(evdog);
	} else
		acl_event_enable_read(event, stream, 0, read_fn, evdog);
}

static void event_dog_open(EVENT_DOG *evdog)
{
	const char *myname = "event_dog_open";
	const char *addr = "127.0.0.1:0";
	char  buf[32];

	buf[0] = 'x';
	buf[1] = 0;

	evdog->sstream = acl_vstream_listen(addr, 32);
	if (evdog->sstream == NULL)
		acl_msg_fatal("%s(%d): listen on addr(%s) error(%s)",
			myname, __LINE__, addr, acl_last_serror());

	evdog->server = acl_vstream_connect(ACL_VSTREAM_LOCAL(evdog->sstream),
			ACL_BLOCKING, 0, 0, 1024);
	if (evdog->server == NULL)
		acl_msg_fatal("%s(%d): connect to addr(%s) error(%s)",
			myname, __LINE__, addr, acl_last_serror());

	if (acl_vstream_writen(evdog->server, buf, 1) == ACL_VSTREAM_EOF)
		acl_msg_fatal("%s(%d): pre write error(%s)",
			myname, __LINE__, acl_last_serror());

	evdog->client = acl_vstream_accept(evdog->sstream, buf, sizeof(buf));
	if (evdog->client == NULL)
		acl_msg_fatal("%s(%d): accept error(%s)",
			myname, __LINE__, acl_last_serror());

	if (acl_vstream_readn(evdog->client, buf, 1) == ACL_VSTREAM_EOF)
		acl_msg_fatal("%s(%d): pre read error(%s)",
			myname, __LINE__, acl_last_serror());

	acl_vstream_close(evdog->sstream);
	evdog->sstream = NULL;

	acl_event_enable_read(evdog->eventp, evdog->client, 0, read_fn, evdog);
}

static void event_dog_reopen(EVENT_DOG *evdog)
{
	event_dog_close(evdog);
	event_dog_open(evdog);
}

EVENT_DOG *event_dog_create(ACL_EVENT *eventp, int thread_mode)
{
	EVENT_DOG *evdog;

	evdog = (EVENT_DOG*) acl_mycalloc(1, sizeof(EVENT_DOG));
	evdog->eventp = eventp;
	evdog->thread_mode = thread_mode;

	event_dog_open(evdog);
	return evdog;
}

ACL_VSTREAM *event_dog_client(EVENT_DOG *evdog)
{
	if (evdog && evdog->client)
		return (evdog->client);
	return NULL;
}

void event_dog_notify(EVENT_DOG *evdog)
{
	const char *myname = "event_dog_notify";
	char  buf[2];

	buf[0] = '0';
	buf[1] = 0;
	
	if (acl_vstream_writen(evdog->server, buf, 1) == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): notify error, reset", myname, __LINE__);
		event_dog_reopen(evdog);
	}
}

void event_dog_free(EVENT_DOG *evdog)
{
	event_dog_close(evdog);
	acl_myfree(evdog);
}
