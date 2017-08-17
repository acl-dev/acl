#include "StdAfx.h"
#include <stdio.h>
#include "icmp_struct.h"
#include "icmp_private.h"
#include "icmp/lib_icmp.h"

static double stamp_sub(const struct timeval *from, const struct timeval *sub)
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

static void icmp_hdr_pack(char *icmp_data, unsigned short id, int type)
{ 
	struct ICMP_HDR *icmp_hdr;

	icmp_hdr = (struct ICMP_HDR *) icmp_data;

	icmp_hdr->type  = type;
	icmp_hdr->code  = 0;
	icmp_hdr->id    = id & 0xffff;
	icmp_hdr->cksum = 0;
	icmp_hdr->seq   = 0;
}

ICMP_PKT *imcp_pkt_pack(size_t dlen, ICMP_HOST *host, int type,
	const void *payload, size_t payload_len)
{
	ICMP_PKT *pkt;

	if (dlen > MAX_PACKET)
		dlen = MAX_PACKET;
	if (dlen < MIN_PACKET)
		dlen = MIN_PACKET;

	pkt = (ICMP_PKT*) acl_mycalloc(1, sizeof(ICMP_PKT));
	pkt->dlen = dlen;

	icmp_hdr_pack((char*) pkt, host->chat->pid, type);

	/* icmp body data */
	/* in some mobile router the data in body should be set to 0 ---zsx */
	/* memset(pkt->body.data, 'E', sizeof(pkt->body.data)); */
	memset(pkt->body.data, 0, sizeof(pkt->body.data));

	if (payload && payload_len > 0) {
		size_t n = dlen - sizeof(pkt->body.id);
		if (payload_len > n)
			payload_len = n;
		memcpy(pkt->body.data, payload, payload_len);
	}

	pkt->pkt_status.rtt    = 65535; /* large enough ? */
	pkt->pkt_status.status = ICMP_STATUS_INIT;

	pkt->body.id = host->chat->id;
	pkt->host    = host;
	pkt->wlen    = dlen + sizeof(struct ICMP_HDR);

	return pkt;
}

void icmp_pkt_build(ICMP_PKT *pkt, unsigned short seq_no)
{
	pkt->hdr.seq        = seq_no;
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
	snprintf(to->pkt_status.from_ip, sizeof(to->pkt_status.from_ip),
		"%s", from->pkt_status.from_ip);

	to->pkt_status.status = ICMP_STATUS_OK;
}

int icmp_pkt_unpack(struct sockaddr_in from, const char *buf,
	int bytes, ICMP_PKT *pkt)
{
	const IP_HDR   *iphdr;
	const ICMP_HDR *icmphdr;
	const ICMP_PKT *icmppkt;
	unsigned short iphdrlen;
	int n;

	gettimeofday(&pkt->stamp, NULL);

	iphdr    = (const IP_HDR *) buf;
	iphdrlen = iphdr->h_len * 4 ; /* number of 32-bit words * 4 = bytes */

	if (bytes < iphdrlen + ICMP_MIN) {
		acl_msg_error("Too few bytes from %s",
			inet_ntoa(from.sin_addr));
		return -1;
	}

	icmppkt = (const ICMP_PKT *) (buf + iphdrlen);
	icmphdr = &icmppkt->hdr;

	pkt->pkt_status.reply_len =
		bytes - (iphdrlen + sizeof(struct ICMP_HDR));
	if (pkt->pkt_status.reply_len < MIN_PACKET)
		return -1;

	pkt->hdr.type  = icmphdr->type;
	pkt->hdr.code  = icmphdr->code;
	pkt->hdr.cksum = icmphdr->cksum;
	pkt->hdr.id    = icmphdr->id;
	pkt->hdr.seq   = icmphdr->seq;
	pkt->body.id   = icmppkt->body.id;

	pkt->pkt_status.status = ICMP_STATUS_OK;
	pkt->pkt_status.seq    = icmphdr->seq;
	pkt->pkt_status.ttl    = iphdr->ttl;

	snprintf(pkt->pkt_status.from_ip, sizeof(pkt->pkt_status.from_ip),
		"%s", inet_ntoa(from.sin_addr));

	n = bytes - iphdrlen - (int) sizeof(struct ICMP_HDR)
		- (int) sizeof(pkt->body.id);
	if (n > 0) {
		if (n > MAX_PACKET)
			n = MAX_PACKET;
		memcpy(pkt->body.data, icmppkt->body.data, n);
		return n;
	}

	return 0;
}

int icmp_pkt_check(const ICMP_HOST *host, const ICMP_PKT *pkt)
{
	int seq = pkt->hdr.seq;
	if (seq < 0 || (size_t) seq > host->npkt) {
		acl_msg_warn("invalid seq %d, discard!", seq);
		return 0;
	}

	return host->pkts[seq]->pkt_status.status == ICMP_STATUS_INIT;
}
