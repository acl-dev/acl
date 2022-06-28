#include "StdAfx.h"
#include "icmp_struct.h"
#include "icmp_private.h"
#include "icmp/lib_icmp.h"

#ifdef ACL_WINDOWS
#include <process.h>
#elif	defined(ACL_UNIX)
#include <unistd.h>
#endif

static unsigned long __unique_id = 0;
static ACL_ATOMIC *__unique_lock = NULL;
static acl_pthread_once_t once_control = ACL_PTHREAD_ONCE_INIT;

#ifndef HAVE_NO_ATEXIT
static void proc_on_exit(void)
{
	if (__unique_lock) {
		acl_atomic_free(__unique_lock);
		__unique_lock = NULL;
	}
}
#endif

static void icmp_once(void)
{
	__unique_lock = acl_atomic_new();
	acl_atomic_set(__unique_lock, &__unique_id);
#ifndef HAVE_NO_ATEXIT
	atexit(proc_on_exit);
#endif
}

ICMP_CHAT *icmp_chat_create(ACL_AIO* aio, int check_id)
{
	ICMP_CHAT *chat;

	if (acl_pthread_once(&once_control, icmp_once) != 0)
		acl_msg_fatal("acl_pthread_once failed %s", acl_last_serror());

	chat = (ICMP_CHAT*) acl_mycalloc(1, sizeof(ICMP_CHAT));
	acl_ring_init(&chat->host_head);

	chat->aio  = aio;
	chat->is   = icmp_stream_open(aio);
	chat->seq  = 0;
	chat->cnt  = 0;
#ifdef ACL_UNIX
	chat->pid  = getpid();
#elif defined(ACL_WINDOWS)
	chat->pid  = _getpid();
#endif
	chat->pid &= 0xffff;

	chat->gid  = (unsigned) acl_atomic_int64_fetch_add(__unique_lock, 1);
	chat->check_id = check_id;

	return chat;
}

void icmp_chat_free(ICMP_CHAT *chat)
{
	if (chat->aio)
		icmp_chat_aio_free(chat);
	else
		icmp_chat_sio_free(chat);
}

unsigned short icmp_chat_seqno(ICMP_CHAT *chat)
{
	return chat->seq;
}

void icmp_chat(ICMP_HOST *host)
{
	if (host->chat->aio)
		icmp_chat_aio(host);
	else
		icmp_chat_sio(host);
}

int icmp_chat_count(ICMP_CHAT *chat)
{
	return chat->cnt;
}

int icmp_chat_size(ICMP_CHAT *chat)
{
	return acl_ring_size(&chat->host_head);
}

int icmp_chat_finish(ICMP_CHAT *chat)
{
	if (chat->cnt == acl_ring_size(&chat->host_head))
		return 1;
	return 0;
}
