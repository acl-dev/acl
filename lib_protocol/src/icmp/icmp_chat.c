#include "StdAfx.h"
#include "icmp_struct.h"
#include "icmp_private.h"
#include "icmp/lib_icmp.h"

#ifdef WIN32
#include <process.h>
#elif	defined(ACL_UNIX)
#include <unistd.h>
#endif

ICMP_CHAT *icmp_chat_create(ACL_AIO* aio, int check_tid)
{
	ICMP_CHAT *chat;

	chat = (ICMP_CHAT*) acl_mycalloc(1, sizeof(ICMP_CHAT));
	chat->aio = aio;
	acl_ring_init(&chat->host_head);
	chat->is = icmp_stream_open(aio);
	chat->seq_no = 0;
	chat->count = 0;
#ifdef ACL_UNIX
	chat->pid = getpid();
#elif defined(WIN32)
	chat->pid = _getpid();
#endif
	chat->tid = (unsigned long) acl_pthread_self();
	chat->check_tid = check_tid;

	if (aio != NULL)
		icmp_chat_aio_init(chat, aio);
	else
		icmp_chat_sio_init(chat);

	return (chat);
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
	return (chat->seq_no);
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
	return (chat->count);
}

int icmp_chat_size(ICMP_CHAT *chat)
{
	return (acl_ring_size(&chat->host_head));
}

int icmp_chat_finish(ICMP_CHAT *chat)
{
	if (chat->count == acl_ring_size(&chat->host_head))
		return (1);
	return (0);
}
