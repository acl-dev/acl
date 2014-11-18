#include "StdAfx.h"
#include "icmp_struct.h"
#include "icmp_private.h"
#include "icmp/lib_icmp.h"

void icmp_host_free(ICMP_HOST *host)
{
	ICMP_PKT *pkt;
	ACL_RING *ring_ptr;

	while ((ring_ptr = acl_ring_pop_head(&host->pkt_head)) != NULL) {
		pkt = RING_TO_PKT(ring_ptr);
		imcp_pkt_free(pkt);
	}

	acl_myfree(host);
}

ICMP_HOST* icmp_host_new(ICMP_CHAT *chat, const char *domain, const char *ip,
	size_t npkt, size_t dlen, int delay, int timeout)
{
	const char* myname = "icmp_host_new";
	ICMP_HOST *host;
	ICMP_PKT *pkt;
	size_t i;
	char *ptr;

	if (ip == NULL || *ip == 0)
		acl_msg_fatal("%s(%d): ip invalid", myname, __LINE__);

	if (npkt == 0)
		npkt = 5;
	else if (npkt > 10240)
		npkt = 10240;

	/* ·ÖÅäÄÚ´æ¿é */

	host = (ICMP_HOST*) acl_mycalloc(1, sizeof(ICMP_HOST));

	if (domain && *domain)
		ACL_SAFE_STRNCPY(host->domain, domain, sizeof(host->domain));
	ACL_SAFE_STRNCPY(host->dest_ip, ip, sizeof(host->dest_ip));
	ptr = strchr(host->dest_ip, ':');
	if (ptr)
		*ptr = 0; /* È¥µô¶Ë¿Ú×Ö¶Î */

	host->dest.sin_family = AF_INET;
	host->dest.sin_addr.s_addr = inet_addr(host->dest_ip);
	/*	host->dest.sin_port = htons(53); */
	host->chat = chat;
	host->timeout = timeout;
	host->delay = delay;
	host->dlen = dlen;
	host->npkt = npkt;
	host->nsent = 0;

	acl_ring_init(&host->pkt_head);
	for (i = 0; i < npkt; i++) {
		pkt = imcp_pkt_pack(dlen, host);
		acl_ring_prepend(&host->pkt_head, &pkt->pkt_ring);
	}

	acl_ring_prepend(&chat->host_head, &host->host_ring);
	return (host);
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
	host->stat_finish = stat_finish;
}

