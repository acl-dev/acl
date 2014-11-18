#include "StdAfx.h"
#include <stdio.h>
#include "icmp_struct.h"
#include "icmp_private.h"
#include "icmp/lib_icmp.h"

static double stamp_sub(const struct timeval *from, const struct timeval *sub_by)
{
	struct timeval res;

	memcpy(&res, from, sizeof(struct timeval));

	res.tv_usec -= sub_by->tv_usec;
	if (res.tv_usec < 0) {
		--res.tv_sec;
		res.tv_usec += 1000000;
	}
	res.tv_sec -= sub_by->tv_sec;

	return (res.tv_sec * 1000.0 + res.tv_usec/1000.0);
}

static unsigned short checksum(unsigned short *buffer, size_t size)
{
	unsigned long cksum = 0;

	while (size > 1) {
		cksum += *buffer++;
		size -= sizeof(unsigned short);
	}

	if (size)
		cksum += *(unsigned char *) buffer;

	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >> 16);
	return (unsigned short) (~cksum);
}

static void icmp_hdr_pack(char *icmp_data, unsigned short id)
{ 
	struct ICMP_HDR *icmp_hdr;

	icmp_hdr = (struct ICMP_HDR *) icmp_data;

	icmp_hdr->type = ICMP_ECHO;
	icmp_hdr->code = 0;
	icmp_hdr->id = id;
	icmp_hdr->cksum = 0;
	icmp_hdr->seq = 0;
}

ICMP_PKT *imcp_pkt_pack(size_t dlen, ICMP_HOST *host)
{
	ICMP_PKT *pkt;

	if (dlen > MAX_PACKET)
		dlen = MAX_PACKET;
	if (dlen < MIN_PACKET)
		dlen = MIN_PACKET;

	pkt = (ICMP_PKT*) acl_mycalloc(1, sizeof(ICMP_PKT));
	pkt->dlen = dlen;

	icmp_hdr_pack((char*) pkt, host->chat->pid);
	/* icmp body data */
	memset(pkt->body.data, 'E', dlen - sizeof(struct ICMP_HDR));
	pkt->body.tid = host->chat->tid;

	pkt->pkt_status.status = ICMP_STATUS_UNREACH;
	pkt->pkt_status.rtt = 65535; /* large enough ? */

	pkt->icmp_host = host;
	pkt->write_len = dlen + sizeof(struct ICMP_HDR);
	pkt->read_len = dlen + sizeof(struct ICMP_HDR) + sizeof(struct IP_HDR) - 4;
	return (pkt);
}

void icmp_pkt_build(ICMP_PKT *pkt, unsigned short seq_no)
{
	pkt->hdr.seq = seq_no;
	pkt->pkt_status.seq = pkt->hdr.seq;

	gettimeofday(&pkt->stamp, NULL);
	pkt->hdr.cksum = checksum((unsigned short *) pkt,
		pkt->dlen + sizeof(struct ICMP_HDR));
}

void imcp_pkt_free(ICMP_PKT *ipkt)
{
	acl_myfree(ipkt);
}

void icmp_pkt_save(ICMP_PKT* to, const ICMP_PKT* from)
{
	to->pkt_status.reply_len = from->pkt_status.reply_len;
	to->pkt_status.rtt = stamp_sub(&from->stamp, &to->stamp);
	to->pkt_status.ttl = from->pkt_status.ttl;
	snprintf(to->pkt_status.frome_ip, sizeof(to->pkt_status.frome_ip),
		"%s", from->pkt_status.frome_ip);

	to->pkt_status.status = ICMP_STATUS_OK;
}

int icmp_pkt_unpack(const ICMP_CHAT *chat, const char *buf, int bytes, ICMP_PKT *pkt)
{
	const IP_HDR *iphdr;
	const ICMP_HDR *icmphdr;
	const ICMP_PKT *icmppkt;
	unsigned short iphdrlen;

	iphdr = (const IP_HDR *) buf;
	iphdrlen = iphdr->h_len * 4 ; /* number of 32-bit words *4 = bytes */

	if (bytes < iphdrlen + ICMP_MIN) {
		acl_msg_error("Too few bytes from %s",
			inet_ntoa(chat->is->from.sin_addr));
		return (-1);
	}

	icmppkt = (const ICMP_PKT *) (buf + iphdrlen);
	icmphdr = &icmppkt->hdr;

	if (icmphdr->type != ICMP_ECHOREPLY) {
		return (-1);
	}

	if (icmphdr->id != chat->pid) {
		return (-1);
	}

	pkt->pkt_status.reply_len = bytes - (iphdrlen + sizeof(struct ICMP_HDR));
	if (pkt->pkt_status.reply_len < MIN_PACKET) {
		return (-1);
	}

	pkt->body.tid = icmppkt->body.tid;
	if (chat->check_tid && pkt->body.tid != chat->tid)
		return (-1);

	pkt->hdr.seq = icmphdr->seq;
	gettimeofday(&pkt->stamp, NULL);

	pkt->pkt_status.status = ICMP_STATUS_OK;
	pkt->pkt_status.seq = icmphdr->seq;
	pkt->pkt_status.ttl = iphdr->ttl;
	snprintf(pkt->pkt_status.frome_ip, sizeof(pkt->pkt_status.frome_ip),
		"%s", inet_ntoa(chat->is->from.sin_addr));
	return (0);
}
