#include "StdAfx.h"
#include "icmp_struct.h"
#include "icmp_private.h"

static void read_pkt(ICMP_HOST *host, ICMP_PKT *pkt_src)
{
	ICMP_CHAT   *chat   = host->chat;
	ACL_VSTREAM *stream = host->chat->is->vstream;
	ICMP_PKT pkt;
	char  buf[2048];
	int   ret;

	while (1) {
#ifdef ACL_UNIX
		if (acl_read_poll_wait(ACL_VSTREAM_SOCK(stream),
			host->timeout) < 0) {
#else
		if (acl_read_select_wait(ACL_VSTREAM_SOCK(stream),
			host->timeout) < 0) {
#endif
			/* 汇报请求包超时 */
			icmp_stat_timeout(host, pkt_src);
			return;
		}

		ret = acl_vstream_read(stream, buf, sizeof(buf));
		if (ret == ACL_VSTREAM_EOF) {
			acl_msg_error("read error(%s)", acl_last_serror());
			continue;
		}

		if (icmp_pkt_unpack(chat->is->from, buf, ret, &pkt) < 0)
			continue;

		if (pkt.hdr.type != ICMP_ECHOREPLY)
			continue;
		if (pkt.hdr.id != (chat->pid & 0xffff))
			continue;
		if (chat->check_id && pkt.body.id != chat->id)
			continue;
		if (!icmp_pkt_check(host, &pkt))
			continue;

		icmp_pkt_save(pkt_src, &pkt);
		icmp_stat_report(host, pkt_src);
		break;
	}
}

static void send_pkt(ICMP_HOST *host, ICMP_PKT *pkt)
{
	ICMP_CHAT   *chat   = host->chat;
	ACL_VSTREAM *stream = host->chat->is->vstream;
	int   ret;

	/* 指定当前包的目的主机，间接传递给 acl_vstream_writen 中的回调函数 */
	chat->is->curr_host = host;

	/* 组建发送数据包 */
	icmp_pkt_build(pkt, chat->seq_no++);

	/* 采用同步发送的模式 */
	ret = acl_vstream_writen(stream, (const char*) pkt, (int) pkt->wlen);
	host->nsent++;

	if (ret == ACL_VSTREAM_EOF) {
		/* 汇报主机不可达信息 */
		icmp_stat_unreach(host, pkt);
	}
}

void icmp_chat_sio(ICMP_HOST* host)
{
	ICMP_PKT *pkt;

	for (; host->ipkt < host->npkt; host->ipkt++) {
		pkt = host->pkts[host->ipkt];
		send_pkt(host, pkt);
		read_pkt(host, pkt);
		acl_doze(host->delay);
	}

	host->chat->count++;
	if (host->stat_finish)
		host->stat_finish(host, host->arg);
}

void icmp_chat_sio_free(ICMP_CHAT *chat)
{
	icmp_stream_close(chat->is);
	acl_myfree(chat);
}
