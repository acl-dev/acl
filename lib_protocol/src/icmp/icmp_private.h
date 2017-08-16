#ifndef __ICMP_PRIVATE_INCLUDE_H__
#define __ICMP_PRIVATE_INCLUDE_H__

#include "lib_acl.h"
#include "icmp/lib_icmp_type.h"

/* in icmp_stream.c */
void icmp_stream_close(ICMP_STREAM* is);
ICMP_STREAM* icmp_stream_open(ACL_AIO *aio);
void icmp_stream_reopen(ACL_AIO *aio, ICMP_STREAM *is);

/* in icmp_chat_aio.c */
void icmp_chat_aio_add(ICMP_CHAT *chat, ICMP_HOST *host);
void icmp_chat_aio_free(ICMP_CHAT *chat);
void icmp_chat_aio(ICMP_HOST* host);

/* in icmp_chat_sio.c */
void icmp_chat_sio_free(ICMP_CHAT *chat);
void icmp_chat_sio(ICMP_HOST* host);

/* in icmp_pkt.c */
ICMP_PKT *imcp_pkt_pack(size_t dlen, ICMP_HOST *host, int type,
	const void *payload, size_t payload_len);
void icmp_pkt_build(ICMP_PKT *pkt, unsigned short seq_no);
void imcp_pkt_free(ICMP_PKT *ipkt);
void icmp_pkt_save(ICMP_PKT* to, const ICMP_PKT* from);
int icmp_pkt_unpack(struct sockaddr_in from, const char *buf, int bytes,
	ICMP_PKT *pkt);
int icmp_pkt_check(const ICMP_HOST *host, const ICMP_PKT *pkt);

/* in icmp_stat.c */
void icmp_stat_timeout(ICMP_HOST *host, ICMP_PKT *pkt);
void icmp_stat_unreach(ICMP_HOST *host, ICMP_PKT *pkt);
void icmp_stat_report(ICMP_HOST *host, ICMP_PKT *pkt);
void icmp_stat_finish(ICMP_HOST *host);

/* in icmp_timer.c */
ICMP_TIMER *icmp_timer_new(void);
void icmp_timer_free(ICMP_TIMER* timer);

#endif
