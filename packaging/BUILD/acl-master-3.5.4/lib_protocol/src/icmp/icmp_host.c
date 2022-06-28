#include "StdAfx.h"
#include "icmp_struct.h"
#include "icmp_private.h"
#include "icmp/lib_icmp.h"

void icmp_host_free(ICMP_HOST *host)
{
	size_t i;

	for (i = 0; i < host->npkt; i++)
		icmp_pkt_free(host->pkts[i]);

	acl_myfree(host);
}

ICMP_HOST *icmp_host_alloc(ICMP_CHAT *chat, const char *domain, const char *ip)
{
	ICMP_HOST *host;
	char *ptr;

	if (ip == NULL || *ip == 0)
		acl_msg_fatal("%s(%d): ip null", __FUNCTION__, __LINE__);

	host = (ICMP_HOST*) acl_mycalloc(1, sizeof(ICMP_HOST));
	if (domain && *domain)
		ACL_SAFE_STRNCPY(host->domain, domain, sizeof(host->domain));
	ACL_SAFE_STRNCPY(host->dest_ip, ip, sizeof(host->dest_ip));

	ptr = strchr(host->dest_ip, ':');
	if (ptr)
		*ptr = 0; /* È¥µô¶Ë¿Ú×Ö¶Î */

	if (chat->aio != NULL)
		icmp_chat_aio_add(chat, host);
	acl_ring_prepend(&chat->host_head, &host->host_ring);
	host->chat  = chat;
	host->nsent = 0;
	host->dest.sin_family      = AF_INET;
	host->dest.sin_addr.s_addr = inet_addr(host->dest_ip);
	return host;
}

void icmp_host_init(ICMP_HOST *host, unsigned char type, unsigned char code,
	size_t npkt, size_t dlen, int delay, int timeout)
{
	size_t i;

	if (npkt == 0)
		npkt = 5;
	else if (npkt > 10240)
		npkt = 10240;

	host->delay   = delay;
	host->timeout = timeout;

	host->dlen = dlen;
	host->npkt = npkt;
	host->pkts = (ICMP_PKT**) acl_mycalloc(host->npkt, sizeof(ICMP_PKT*));

	for (i = 0; i < npkt; i++) {
		ICMP_PKT *pkt = icmp_pkt_alloc();
		icmp_pkt_client(host, pkt, type, code, NULL, dlen);
		host->pkts[i] = pkt;
	}
}

ICMP_HOST *icmp_host_new(ICMP_CHAT *chat, const char *domain, const char *ip,
	size_t npkt, size_t dlen, int delay, int timeout)
{
	ICMP_HOST *host = icmp_host_alloc(chat, domain, ip);
	icmp_host_init(host, ICMP_TYPE_ECHO, 0, npkt, dlen, delay, timeout);
	return host;
}

void icmp_host_set(ICMP_HOST *host, void *arg,
	void (*stat_respond)(ICMP_PKT_STATUS*, void*),
	void (*stat_timeout)(ICMP_PKT_STATUS*, void*),
	void (*stat_unreach)(ICMP_PKT_STATUS*, void*),
	void (*stat_finish)(ICMP_HOST*, void*))
{
	host->arg = arg;
	host->stat_respond = stat_respond;
	host->stat_unreach = stat_unreach;
	host->stat_timeout = stat_timeout;
	host->stat_finish  = stat_finish;
}
