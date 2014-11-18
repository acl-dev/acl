#include "StdAfx.h"
#include "icmp_struct.h"
#include "icmp_private.h"
#include "icmp/lib_icmp.h"

void icmp_stat_timeout(ICMP_PKT *pkt)
{
	pkt->pkt_status.status = ICMP_STATUS_TIMEOUT;
	if (pkt->icmp_host->enable_log)
		acl_msg_info("%s Ping timeout, icmp_seq %d",
			pkt->icmp_host->dest_ip, pkt->hdr.seq);
	if (pkt->icmp_host->stat_timeout != NULL)
		pkt->icmp_host->stat_timeout(&pkt->pkt_status, pkt->icmp_host->arg);
}

void icmp_stat_unreach(ICMP_PKT *pkt)
{
	pkt->pkt_status.status = ICMP_STATUS_UNREACH;
	if (pkt->icmp_host->enable_log)
		acl_msg_info("%s Destination host unreachable.", pkt->icmp_host->dest_ip);
	if (pkt->icmp_host->stat_unreach != NULL)
		pkt->icmp_host->stat_unreach(&pkt->pkt_status, pkt->icmp_host->arg);
}

void icmp_stat_report(ICMP_PKT *pkt)
{
	pkt->pkt_status.status = ICMP_STATUS_OK;
	if (pkt->icmp_host->enable_log)
		acl_msg_info("Reply from %s: bytes=%d time=%.3fms TTL=%d icmp_seq=%d status=%d",
			pkt->pkt_status.frome_ip, (int) pkt->pkt_status.reply_len,
			pkt->pkt_status.rtt, pkt->pkt_status.ttl,
			pkt->pkt_status.seq, pkt->pkt_status.status);

	if (pkt->icmp_host->stat_respond != NULL)
		pkt->icmp_host->stat_respond(&pkt->pkt_status, pkt->icmp_host->arg);
}

static void icmp_status(ICMP_HOST *host, int flag)
{
	int  nok = 0;
	double Minimun = 65535, Maximum = 0, Total = 0;
	ICMP_PKT *pkt;

	pkt = ICMP_PKT_NEXT(&host->pkt_head, &host->pkt_head);
	while (pkt) {
		if (pkt->pkt_status.status  == ICMP_STATUS_OK) {
			nok++;
			if (pkt->pkt_status.rtt < Minimun)
				Minimun = pkt->pkt_status.rtt;
			if (pkt->pkt_status.rtt > Maximum)
				Maximum = pkt->pkt_status.rtt;
			Total += pkt->pkt_status.rtt;
		}
		pkt = ICMP_PKT_NEXT(&host->pkt_head, &pkt->pkt_ring);
	}
	
	host->icmp_stat.nsent = host->nsent;
	host->icmp_stat.nreceived = nok;
	host->icmp_stat.loss = host->nsent > 0 ?
		(host->nsent - nok) * 100/host->nsent : 0;
	host->icmp_stat.tmax = Maximum;
	host->icmp_stat.tmin = Minimun;
	host->icmp_stat.tsum = Total;
	host->icmp_stat.tave = nok > 0 ? host->icmp_stat.tsum/nok : 0;

	if (flag) {
		acl_msg_info("Ping statistics for %s: %s",
			host->dest_ip, host->domain[0] != 0 ? host->domain : "");
		acl_msg_info("\tPackets: Sent = %d, Received = %d, Lost = %d (%.2f%% loss),",
			(int) host->icmp_stat.nsent, (int) host->icmp_stat.nreceived,
			(int) (host->icmp_stat.nsent - host->icmp_stat.nreceived),
			host->icmp_stat.loss);

		if (nok > 0) {
			acl_msg_info("Approximate round trip times in milli-seconds:");
			acl_msg_info("\tMinimum = %.3f ms, Maximum = %.3f ms, Average = %.3f ms",
				host->icmp_stat.tmin, host->icmp_stat.tmax, host->icmp_stat.tave);
		}
	}
}

void icmp_stat_host(ICMP_HOST *host, int show_flag)
{
	icmp_status(host, show_flag);
}

void icmp_stat_finish(ICMP_HOST *host)
{
	icmp_status(host, 0);
}

void icmp_stat(ICMP_CHAT *chat)
{
	ICMP_HOST *host;

	acl_msg_info("\r\n>>>hosts' size=%d", acl_ring_size(&chat->host_head));

	host = ICMP_HOST_NEXT(&chat->host_head, &chat->host_head);
	while (host) {
		icmp_stat_host(host, 1);
		host = ICMP_HOST_NEXT(&chat->host_head, &host->host_ring);
	}
}
