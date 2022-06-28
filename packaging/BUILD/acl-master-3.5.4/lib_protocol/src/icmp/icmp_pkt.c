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

ICMP_PKT *icmp_pkt_alloc(void)
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

static void icmp_hdr_pack(struct ICMP_HDR *hdr, unsigned short id,
	unsigned char type, unsigned char code)
{ 
	hdr->type  = type;
	hdr->code  = code;
	hdr->id    = id;
	hdr->cksum = 0;
	hdr->seq   = 0;
}

void icmp_pkt_pack(ICMP_PKT *pkt, unsigned char type, unsigned char code,
	unsigned short id, const void *payload, size_t payload_len)
{
	if (payload_len < ICMP_MIN_PACKET)
		payload_len = ICMP_MIN_PACKET;
	if (payload_len > ICMP_MAX_PACKET)
		payload_len = ICMP_MAX_PACKET;

	pkt->dlen = payload_len;
	icmp_hdr_pack(&pkt->hdr, id, type, code);

	/* icmp body data */
	/* in some mobile router the data in body should be set to 0 ---zsx */
	/* memset(pkt->body.data, 'E', payload_len); */

	if (payload) {
		memset(pkt->body.data, 0, payload_len);
		memcpy(pkt->body.data, payload, payload_len);
	}
}

size_t icmp_pkt_set_extra(ICMP_PKT *pkt, const void *data, size_t len)
{
	size_t n  = pkt->dlen;
	char *ptr = pkt->body.data;

	if (pkt->hdr.code != ICMP_CODE_EXTRA)
		return 0;
	if (n <= sizeof(pkt->body.gid))
		return 0;

	n   -= sizeof(pkt->body.gid);
	ptr += sizeof(pkt->body.gid);

	if (n > len)
		n = len;
	memcpy(ptr, data, n);
	return n;
}

void icmp_pkt_client(ICMP_HOST *host, ICMP_PKT *pkt, unsigned char type,
	unsigned char code, const void *payload, size_t payload_len)
{
	icmp_pkt_pack(pkt, type, code, host->chat->pid,
		payload, payload_len);

	pkt->pkt_status.rtt    = 65535; /* large enough ? */
	pkt->pkt_status.status = ICMP_STATUS_INIT;

	pkt->body.gid = (unsigned int) host->chat->gid;
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
	unsigned short  iphdrlen;
	int n;

	iphdr    = (const IP_HDR *) buf;
	iphdrlen = iphdr->h_len * 4 ; /* number of 32-bit words * 4 = bytes */

	if (bytes < iphdrlen + ICMP_MIN) {
		acl_msg_error("Too few bytes from %s", inet_ntoa(from.sin_addr));
		return -1;
	}

	icmppkt = (const ICMP_PKT *) (buf + iphdrlen);
	icmphdr = &icmppkt->hdr;

	pkt->dlen = bytes - (iphdrlen + sizeof(ICMP_HDR));
	if (pkt->dlen < ICMP_MIN_PACKET)
		return -1;

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

	n = bytes - iphdrlen - (int) sizeof(struct ICMP_HDR);
	if (n > 0) {
		if (n > ICMP_MAX_PACKET)
			n = ICMP_MAX_PACKET;
		memcpy(pkt->body.data, icmppkt->body.data, n);
	}
	pkt->pkt_status.dlen = n;
	return n;
}

void icmp_pkt_save_status(ICMP_PKT* to, const ICMP_PKT* from)
{
	to->pkt_status.reply_len = from->pkt_status.reply_len;
	to->pkt_status.ttl       = from->pkt_status.ttl;
	to->pkt_status.dlen      = from->pkt_status.dlen;

	snprintf(to->pkt_status.from_ip, sizeof(to->pkt_status.from_ip),
		"%s", from->pkt_status.from_ip);
}

ICMP_PKT *icmp_pkt_check(const ICMP_HOST *host, const ICMP_PKT *pkt)
{
	int seq = pkt->hdr.seq;
	if (seq < 0 || (size_t) seq >= host->npkt) {
		acl_msg_warn("invalid seq %d, discard!", seq);
		return NULL;
	}

	if (host->pkts[seq]->pkt_status.status == ICMP_STATUS_INIT)
		return host->pkts[seq];
	return NULL;
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

size_t icmp_pkt_payload(const ICMP_PKT *pkt, char *buf, size_t size)
{
	size_t dlen = pkt->dlen;
	const char *ptr = pkt->body.data;

	if (dlen == 0 || size == 0)
		return 0;

	/* 如果检测 code 值是私有值，则仅取除 git 外的数据 */
	if (pkt->hdr.code == ICMP_CODE_EXTRA) {
		if (dlen <= sizeof(pkt->body.gid))
			return 0;
		dlen -= sizeof(pkt->body.gid);
		ptr  += sizeof(pkt->body.gid);
	}

	if (dlen > size)
		dlen = size;
	memcpy(buf, ptr, dlen);
	return dlen;
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
	if (size > ICMP_MAX_PACKET)
		size = ICMP_MAX_PACKET;
	memcpy(pkt->body.data, data, size);
}
