#include "StdAfx.h"
#include "icmp_struct.h"
#include "icmp_private.h"
#include "icmp/lib_icmp.h"

double stamp_sub(const struct timeval *from, const struct timeval *sub)
{
	struct timeval res;

	memcpy(&res, from, sizeof(struct timeval));

	res.tv_usec -= sub->tv_usec;
	if (res.tv_usec < 0) {
		--res.tv_sec;
		res.tv_usec += 1000000;
	}
	res.tv_sec -= sub->tv_sec;

	return res.tv_sec * 1000.0 + res.tv_usec/1000.0;
}

static void icmp_rtt_compute(ICMP_PKT *pkt)
{
	struct timeval now;

	gettimeofday(&now, NULL);
	pkt->pkt_status.rtt = stamp_sub(&now, &pkt->stamp);
}

void icmp_stat_timeout(ICMP_HOST *host, ICMP_PKT *pkt)
{
	icmp_rtt_compute(pkt);
	pkt->pkt_status.status = ICMP_STATUS_TIMEOUT;
	if (host->enable_log)
		acl_msg_info("%s Ping timeout, icmp_seq %d",
			host->dest_ip, pkt->hdr.seq);
	if (host->stat_timeout != NULL)
		host->stat_timeout(&pkt->pkt_status, host->arg);
}

void icmp_stat_unreach(ICMP_HOST *host, ICMP_PKT *pkt)
{
	icmp_rtt_compute(pkt);
	pkt->pkt_status.status = ICMP_STATUS_UNREACH;
	if (host->enable_log)
		acl_msg_info("%s Destination host unreachable.", host->dest_ip);
	if (host->stat_unreach != NULL)
		host->stat_unreach(&pkt->pkt_status, host->arg);
}

void icmp_stat_report(ICMP_HOST *host, ICMP_PKT *pkt)
{
	icmp_rtt_compute(pkt);
	pkt->pkt_status.status = ICMP_STATUS_OK;
	if (host->enable_log)
		acl_msg_info("Reply from %s: bytes=%d time=%.3fms TTL=%d icmp_seq=%d status=%d",
			pkt->pkt_status.from_ip, (int) pkt->pkt_status.reply_len,
			pkt->pkt_status.rtt, pkt->pkt_status.ttl,
			pkt->pkt_status.seq, pkt->pkt_status.status);

	if (host->stat_respond != NULL)
		host->stat_respond(&pkt->pkt_status, host->arg);
}

static void icmp_status(ICMP_HOST *host, int flag)
{
	int    nok = 0;
	size_t i;
	double Minimun = 65535, Maximum = 0, Total = 0;

	for (i = 0; i < host->npkt; i++) {
		ICMP_PKT *pkt = host->pkts[i];
		if (pkt->pkt_status.status == ICMP_STATUS_OK) {
			nok++;
			if (pkt->pkt_status.rtt < Minimun)
				Minimun = pkt->pkt_status.rtt;
			if (pkt->pkt_status.rtt > Maximum)
				Maximum = pkt->pkt_status.rtt;
			Total += pkt->pkt_status.rtt;
		}
	}
	
	host->icmp_stat.nsent = host->nsent;
	host->icmp_stat.nreceived = nok;
	host->icmp_stat.loss = host->nsent > 0 ?
		((double) host->nsent - nok) * 100/(double) host->nsent : 0;
	host->icmp_stat.tmax = Maximum;
	host->icmp_stat.tmin = Minimun;
	host->icmp_stat.tsum = Total;
	host->icmp_stat.tave = nok > 0 ? host->icmp_stat.tsum/nok : 0;

	if (!flag)
		return;

	acl_msg_info("Ping statistics for %s: %s",
		host->dest_ip, host->domain[0] != 0 ? host->domain : "");

	acl_msg_info("\tPackets: Sent = %d, Received = %d, Lost = %d (%.2f%% loss),",
		(int) host->icmp_stat.nsent, (int) host->icmp_stat.nreceived,
		(int) (host->icmp_stat.nsent - host->icmp_stat.nreceived),
		host->icmp_stat.loss);

	if (nok > 0) {
		acl_msg_info("Approximate round trip times in milli-seconds:");
		acl_msg_info("\tMinimum = %.3f ms, Maximum = %.3f ms, Average = %.3f ms",
			host->icmp_stat.tmin, host->icmp_stat.tmax,
			host->icmp_stat.tave);
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
