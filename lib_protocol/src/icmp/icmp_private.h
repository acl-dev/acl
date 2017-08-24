#ifndef __ICMP_PRIVATE_INCLUDE_H__
#define __ICMP_PRIVATE_INCLUDE_H__

#include "lib_acl.h"
#include "icmp/lib_icmp_type.h"

/* in icmp_stream.c */
void icmp_stream_reopen(ACL_AIO *aio, ICMP_STREAM *is);

/* in icmp_chat_aio.c */
void icmp_chat_aio_add(ICMP_CHAT *chat, ICMP_HOST *host);
void icmp_chat_aio_free(ICMP_CHAT *chat);
void icmp_chat_aio(ICMP_HOST* host);

/* in icmp_chat_sio.c */
void icmp_chat_sio_free(ICMP_CHAT *chat);
void icmp_chat_sio(ICMP_HOST* host);

/* in icmp_pkt.c */
void icmp_pkt_client(ICMP_HOST *host, ICMP_PKT *pkt, unsigned char type,
	unsigned char code, const void *payload, size_t payload_len);

/* in icmp_stat.c */
double stamp_sub(const struct timeval *from, const struct timeval *sub);
void icmp_stat_timeout(ICMP_HOST *host, ICMP_PKT *pkt);
void icmp_stat_unreach(ICMP_HOST *host, ICMP_PKT *pkt);
void icmp_stat_report(ICMP_HOST *host, ICMP_PKT *pkt);
void icmp_stat_finish(ICMP_HOST *host);

/* in icmp_timer.c */
ICMP_TIMER *icmp_timer_new(void);
void icmp_timer_free(ICMP_TIMER* timer);

#endif
