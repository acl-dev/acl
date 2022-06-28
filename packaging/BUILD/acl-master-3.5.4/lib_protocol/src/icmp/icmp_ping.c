#include "StdAfx.h"
#include <signal.h>
#include "icmp_struct.h"
#include "icmp_private.h"
#include "icmp/lib_icmp.h"

void icmp_ping_one(ICMP_CHAT *chat, const char *domain, const char *ip,
	size_t npkt, int delay, int timeout)
{
	ICMP_HOST *host;

	acl_assert(chat);
	host = icmp_host_new(chat, domain, ip, npkt, 64, delay, timeout);
	host->enable_log = 1;
	icmp_chat(host);
}
