#include "StdAfx.h"
#include <stdio.h>
#include "icmp_struct.h"
#include "icmp_private.h"
#include "icmp/lib_icmp.h"

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
	icmp_hdr->id    = id;
	icmp_hdr->cksum = 0;
	icmp_hdr->seq   = 0;
}

ICMP_PKT *icmp_pkt_new(void)
{
	ICMP_PKT *pkt = (ICMP_PKT*) acl_mycalloc(1, sizeof(ICMP_PKT));

	pkt->pkt_status.pkt  = pkt;
	pkt->pkt_status.data = pkt->body.data;
	return pkt;
}

void icmp_pkt_free(ICMP_PKT *ipkt)
{
	acl_myfree(ipkt);
}

void icmp_pkt_pack(ICMP_PKT *pkt, int type, unsigned short id,
	const void *payload, size_t payload_len)
{
	if (payload_len < MIN_PACKET)
		payload_len = MIN_PACKET;
	if (payload_len > MAX_PACKET)
		payload_len = MAX_PACKET;

	pkt->dlen = payload_len;
	icmp_hdr_pack((char*) pkt, id, type);

	/* icmp body data */
	/* in some mobile router the data in body should be set to 0 ---zsx */
	/* memset(pkt->body.data, 'E', sizeof(pkt->body.data)); */
	memset(pkt->body.data, 0, sizeof(pkt->body.data));

	if (payload) {
		acl_assert(payload_len > sizeof(pkt->body.gid));

		memcpy(pkt->body.data + sizeof(pkt->body.gid),
			payload, payload_len - sizeof(pkt->body.gid));
	}
}

void icmp_pkt_client(ICMP_PKT *pkt, ICMP_HOST *host, int type,
	const void *payload, size_t payload_len)
{
	icmp_pkt_pack(pkt, type, host->chat->pid, payload, payload_len);

	pkt->pkt_status.rtt    = 65535; /* large enough ? */
	pkt->pkt_status.status = ICMP_STATUS_INIT;

	pkt->body.gid = host->chat->gid;
	pkt->host     = host;
}

void icmp_pkt_build(ICMP_PKT *pkt, unsigned short seq)
{
	pkt->hdr.seq   = seq;
	pkt->hdr.cksum  = checksum((unsigned short *) pkt,
			pkt->dlen + sizeof(struct ICMP_HDR));
	pkt->pkt_status.seq = pkt->hdr.seq;
	pkt->wlen = pkt->dlen + sizeof(struct ICMP_HDR);
}

int icmp_pkt_unpack(struct sockaddr_in from, const char *buf,
	int bytes, ICMP_PKT *pkt)
{
	const IP_HDR   *iphdr;
	const ICMP_HDR *icmphdr;
	const ICMP_PKT *icmppkt;
	unsigned short iphdrlen;
	int n;

	iphdr    = (const IP_HDR *) buf;
	iphdrlen = iphdr->h_len * 4 ; /* number of 32-bit words * 4 = bytes */

	if (bytes < iphdrlen + ICMP_MIN) {
		acl_msg_error("Too few bytes from %s",
			inet_ntoa(from.sin_addr));
		return -1;
	}

	icmppkt = (const ICMP_PKT *) (buf + iphdrlen);
	icmphdr = &icmppkt->hdr;

	pkt->dlen = bytes - (iphdrlen + sizeof(ICMP_HDR));
	if (pkt->dlen < MIN_PACKET)
		return -1;

//	printf(">>>pkt dlen: %d, bytes: %d\r\n",
//		(int) pkt->dlen, (int) bytes - (int) iphdrlen);

	pkt->hdr.type  = icmphdr->type;
	pkt->hdr.code  = icmphdr->code;
	pkt->hdr.cksum = icmphdr->cksum;
	pkt->hdr.id    = icmphdr->id;
	pkt->hdr.seq   = icmphdr->seq;
	pkt->body.gid  = icmppkt->body.gid;

	pkt->pkt_status.reply_len = pkt->dlen;
	pkt->pkt_status.status    = ICMP_STATUS_OK;
	pkt->pkt_status.seq       = icmphdr->seq;
	pkt->pkt_status.ttl       = iphdr->ttl;

	snprintf(pkt->pkt_status.from_ip, sizeof(pkt->pkt_status.from_ip),
		"%s", inet_ntoa(from.sin_addr));

	n = bytes - iphdrlen - (int) sizeof(struct ICMP_HDR)
		- (int) sizeof(pkt->body.gid);
	if (n > 0) {
		if (n > MAX_PACKET)
			n = MAX_PACKET;
		memcpy(pkt->body.data + sizeof(pkt->body.gid),
			icmppkt->body.data + sizeof(pkt->body.gid), n);
		pkt->pkt_status.dlen = n;
		return n;
	}

	return 0;
}

void icmp_pkt_save_status(ICMP_PKT* to, const ICMP_PKT* from)
{
	to->pkt_status.reply_len = from->pkt_status.reply_len;
	to->pkt_status.ttl       = from->pkt_status.ttl;
	to->pkt_status.dlen      = from->pkt_status.dlen;

	snprintf(to->pkt_status.from_ip, sizeof(to->pkt_status.from_ip),
		"%s", from->pkt_status.from_ip);
}

int icmp_pkt_check(const ICMP_HOST *host, const ICMP_PKT *pkt)
{
	int seq = pkt->hdr.seq;
	if (seq < 0 || (size_t) seq > host->npkt) {
		acl_msg_warn("invalid seq %d, discard!", seq);
		return 0;
	}

	if (host->pkts[seq]->pkt_status.status == ICMP_STATUS_INIT)
		return 1;
//	printf(">>>seq: %d, status: %d\r\n",
//		seq, host->pkts[seq]->pkt_status.status);
	return 0;
}

unsigned char icmp_pkt_type(const ICMP_PKT *pkt)
{
	return pkt->hdr.type;
}

unsigned char icmp_pkt_code(const ICMP_PKT *pkt)
{
	return pkt->hdr.code;
}

unsigned short icmp_pkt_cksum(const ICMP_PKT *pkt)
{
	return pkt->hdr.cksum;
}

unsigned short icmp_pkt_id(const ICMP_PKT *pkt)
{
	return pkt->hdr.id;
}

unsigned short icmp_pkt_seq(const ICMP_PKT *pkt)
{
	return pkt->hdr.seq;
}

unsigned int icmp_pkt_gid(const ICMP_PKT *pkt)
{
	return pkt->body.gid;
}

size_t icmp_pkt_len(const ICMP_PKT *pkt)
{
	return pkt->dlen;
}

size_t icmp_pkt_wlen(const ICMP_PKT *pkt)
{
	return pkt->wlen;
}

const ICMP_PKT *icmp_pkt_peer(const ICMP_PKT *pkt)
{
	return pkt->peer;
}

const ICMP_PKT_STATUS *icmp_pkt_status(const ICMP_PKT *pkt)
{
	return &pkt->pkt_status;
}

size_t icmp_pkt_data(const ICMP_PKT *pkt, char *buf, size_t size)
{
	size_t n;

	acl_assert(size > MIN_PACKET);

	size--; /* one byte for '\0' */

	if (pkt->dlen < MIN_PACKET)  /* xxx ? */
		return 0;

	n = pkt->dlen - sizeof(pkt->body.gid);

	if (n >= MAX_PACKET) /* xxx? */
		return 0;

	if (n > size)
		n = size;

	memcpy(buf, pkt->body.data + sizeof(pkt->body.gid), n);
	buf[n] = 0;
	return n;
}

void icmp_pkt_set_type(ICMP_PKT *pkt, unsigned char type)
{
	pkt->hdr.type = type;
}

void icmp_pkt_set_code(ICMP_PKT *pkt, unsigned char code)
{
	pkt->hdr.code = code;
}

void icmp_pkt_set_cksum(ICMP_PKT *pkt, unsigned short cksum)
{
	pkt->hdr.cksum = cksum;
}

void icmp_pkt_set_id(ICMP_PKT *pkt, unsigned short id)
{
	pkt->hdr.id = id;
}

void icmp_pkt_set_seq(ICMP_PKT *pkt, unsigned short seq)
{
	pkt->hdr.seq = seq;
}

void icmp_pkt_set_data(ICMP_PKT *pkt, void *data, size_t size)
{
	if (size > MAX_PACKET)
		size = MAX_PACKET;
	memcpy(pkt->body.data, data, size);
}
