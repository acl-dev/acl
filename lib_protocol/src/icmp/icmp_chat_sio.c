#include "StdAfx.h"
#include "icmp_struct.h"
#include "icmp_private.h"

static void read_pkt(ACL_VSTREAM *stream, ICMP_PKT *pkt_src)
{
	ICMP_PKT pkt;
	char  buf[2048];
	int   ret;

	while (1) {
		if (acl_read_wait(ACL_VSTREAM_SOCK(stream), pkt_src->icmp_host->timeout) < 0) {
			/* 汇报请求包超时 */
			icmp_stat_timeout(pkt_src);
			return;
		}

		ret = acl_vstream_read(stream, buf, sizeof(buf));
		if (ret == ACL_VSTREAM_EOF) {
			char tbuf[256];
			acl_msg_error("read error(%s)", acl_last_strerror(tbuf, sizeof(tbuf)));
			continue;
		}

		if (icmp_pkt_unpack(pkt_src->pkt_chat, buf, ret, &pkt) < 0)
			continue;

		icmp_pkt_save(pkt_src, &pkt);
		icmp_stat_report(pkt_src);
		break;
	}
}

static void send_pkt(ACL_VSTREAM *stream, ICMP_PKT *pkt)
{
	int   ret;

	/* 指定当前包的目的主机地址，间接传递给 acl_vstream_writen 中的回调函数 */
	pkt->pkt_chat->is->curr_host = pkt->icmp_host;

	/* 组建发送数据包 */
	icmp_pkt_build(pkt, pkt->pkt_chat->seq_no++);

	/* 采用同步发送的模式 */
	ret = acl_vstream_writen(stream, (const char*) pkt, (int) pkt->write_len);
	pkt->icmp_host->nsent++;

	if (ret == ACL_VSTREAM_EOF) {
		/* 汇报主机不可达信息 */
		icmp_stat_unreach(pkt);
	}
}

void icmp_chat_sio(ICMP_HOST* host)
{
	ICMP_PKT *pkt;
	ACL_VSTREAM *stream;

	pkt = ICMP_PKT_NEXT(&host->pkt_head, &host->pkt_head);
	stream = host->chat->is->vstream;

	while (pkt != NULL) {
		send_pkt(stream, pkt);
		read_pkt(stream, pkt);
		sleep(host->delay);
		pkt = ICMP_PKT_NEXT(&host->pkt_head, &pkt->pkt_ring);
	}

	host->chat->count++;
	if (host->stat_finish)
		host->stat_finish(host, host->arg);
}

void icmp_chat_sio_init(ICMP_CHAT *chat acl_unused)
{
}

void icmp_chat_sio_free(ICMP_CHAT *chat)
{
	icmp_stream_close(chat->is);
	acl_myfree(chat);
}
