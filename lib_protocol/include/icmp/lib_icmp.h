#ifndef __LIB_ICMP_INCLUDE_H__
#define __LIB_ICMP_INCLUDE_H__

/* #include "lib_acl.h" */
#include "lib_icmp_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ICMP_LIB
# ifndef ICMP_API
#  define ICMP_API
# endif
#elif defined(ICMP_DLL) // || defined(_WINDLL)
# if defined(ICMP_EXPORTS) || defined(protocol_EXPORTS)
#  ifndef ICMP_API
#   define ICMP_API __declspec(dllexport)
#  endif
# elif !defined(ICMP_API)
#  define ICMP_API __declspec(dllimport)
# endif
#elif !defined(ICMP_API)
# define ICMP_API
#endif

/* in icmp_chat.c */
/**
 * 创建ICMP会话对象
 * @param aio {ACL_AIO*} 如果该项不为空，则内部在通信过程中采用非阻塞模式，
 *  否则采用阻塞模式
 * @param check_tid {int} 是否在校验响应包时检查数据中的线程号字段
 * @return {ICMP_CHAT*} ICMP会话对象句柄
 */
ICMP_API ICMP_CHAT *icmp_chat_create(ACL_AIO *aio, int check_tid);

/**
 * 释放ICMP会话对象
 * @param chat {ICMP_CHAT*} ICMP会话对象句柄
 */
ICMP_API void icmp_chat_free(ICMP_CHAT *chat);

/**
 * 开始与某个目的主机进行会话
 * @param host {ICMP_HOST*} 调用 icmp_host_new 返回的对象
 */
ICMP_API void icmp_chat(ICMP_HOST* host);

/**
 * 当前的ICMP会话对象中被探测的主机个数
 * @param chat {ICMP_CHAT*} 会话对象句柄
 * @return {int} 被探测主机个数
 */
ICMP_API int icmp_chat_size(ICMP_CHAT *chat);

/**
 * 当前的ICMP会话对象中已经完成的探测的主机个数
 * @param chat {ICMP_CHAT*} 会话对象句柄
 * @return {int} 已完成的被探测主机个数
 */
ICMP_API int icmp_chat_count(ICMP_CHAT *chat);

/**
 * 判断当前的ICMP会话对象中所有探测任务是否已经完成
 * @param chat {ICMP_CHAT*} 会话对象句柄
 * @return {int} != 0: 表示完成; 0: 表示未完成
 */
ICMP_API int icmp_chat_finish(ICMP_CHAT *chat);

/**
 * 取得当前ICMP会话对象中的当前会话序列号值
 * @param chat {ICMP_CHAT*} 会话对象句柄
 * @return {unsigned short} 会话序列号值
 */
ICMP_API unsigned short icmp_chat_seqno(ICMP_CHAT *chat);

/* in icmp_stat.c */
/**
 * 输出当前ICMP的会话状态
 * @param chat {ICMP_CHAT*} 会话对象句柄
 */
ICMP_API void icmp_stat(ICMP_CHAT *chat);

/**
 * 计算某个主机的ICMP会话状态
 * @param host {ICMP_HOST*} 被探测主机对象
 * @param show_flag {int} 是否输出结果至日志文件
 */
ICMP_API void icmp_stat_host(ICMP_HOST *host, int show_flag);

/* in icmp_host.c */
/**
 * 创建一个新的被探测的主机对象
 * @param chat {ICMP_CHAT*} 会话对象句柄
 * @param domain {const char*} 域名标识字符串，可以为空
 * @param ip {const char*} 主机IP地址，不能为空
 * @param npkt {size_t} 对该主机发送的数据包个数
 * @param dlen {size_t} 每个探测数据包的长度
 * @param delay {int} 发送探测数据包的时间间隔(毫秒)
 * @param timeout {int} 被探测主机的响应包超时时间(毫秒)
 * @return {ICMP_HOST*} 被探测主机对象, 如果为空则表示出错
 */
ICMP_API ICMP_HOST* icmp_host_new(ICMP_CHAT *chat, const char *domain,
	const char *ip, size_t npkt, size_t dlen, int delay, int timeout);

/**
 * 释放一个被探测主机对象
 * @param host {ICMP_HOST*} 被探测主机对象
 */
ICMP_API void icmp_host_free(ICMP_HOST *host);

/**
 * 设置探测结果的回调函数
 * @param host {ICMP_HOST*} 被探测主机对象
 * @param arg {void*} 回调函数的参数之一
 * @param stat_respond {void (*)(ICMP_PKT_STATUS*)} 正常响应时的回调函数
 * @param stat_timeout {void (*)(ICMP_PKT_STATUS*)} 超时响应时的回调函数
 * @param stat_unreach {void (*)(ICMP_PKT_STATUS*}} 主机不可达时的回调函数
 * @param stat_finish {void (*)(ICMP_HOST*)} 针对该主机的探测任务时的回调函数
 */
ICMP_API void icmp_host_set(ICMP_HOST *host, void *arg,
	void (*stat_respond)(ICMP_PKT_STATUS*, void*),
	void (*stat_timeout)(ICMP_PKT_STATUS*, void*),
	void (*stat_unreach)(ICMP_PKT_STATUS*, void*),
	void (*stat_finish)(ICMP_HOST*, void*));

/* in icmp_ping.c */
/**
 * ping 一台主机(内部默认每个探测包长度为64个字节)
 * @param chat {ICMP_CHAT*} 会话对象句柄
 * @param domain {const char*} 域名标识字符串，可以为空
 * @param ip {const char*} 主机IP地址，不能为空
 * @param npkt {size_t} 对该主机发送的数据包个数
 * @param delay {int} 发送探测数据包的时间间隔(毫秒)
 * @param timeout {int} 被探测主机的响应包超时时间(毫秒)
 */
ICMP_API void icmp_ping_one(ICMP_CHAT *chat, const char *domain,
	const char *ip, size_t npkt, int delay, int timeout);

/*--------------------------------------------------------------------------*/

/**
 * low level interface for operating ICMP.
 */

ICMP_API ICMP_STREAM* icmp_stream_open(ACL_AIO *aio);
ICMP_API void icmp_stream_close(ICMP_STREAM* is);
ICMP_API ACL_VSTREAM *icmp_vstream(ICMP_STREAM *is);
ICMP_API void icmp_stream_from(ICMP_STREAM *is, struct sockaddr_in *addr);
ICMP_API void icmp_stream_dest(ICMP_STREAM *is, struct sockaddr_in *addr);
ICMP_API void icmp_stream_set_dest(ICMP_STREAM *is, struct sockaddr_in addr);

ICMP_API ICMP_HOST *icmp_host_alloc(ICMP_CHAT *chat, const char *domain,
		const char *ip);
ICMP_API void icmp_host_init(ICMP_HOST *host, unsigned char type,
		unsigned char code, size_t npkt, size_t dlen,
		int delay, int timeout);

ICMP_API ICMP_PKT *icmp_pkt_alloc(void);
ICMP_API void icmp_pkt_free(ICMP_PKT *ipkt);
ICMP_API void icmp_pkt_pack(ICMP_PKT *pkt, unsigned char type,
		unsigned char code, unsigned short id,
		const void *payload, size_t payload_len);
ICMP_API void icmp_pkt_build(ICMP_PKT *pkt, unsigned short seq);
ICMP_API void icmp_pkt_save_status(ICMP_PKT* to, const ICMP_PKT* from);
ICMP_API int  icmp_pkt_unpack(struct sockaddr_in from, const char *buf,
		int bytes, ICMP_PKT *pkt);
ICMP_API ICMP_PKT* icmp_pkt_check(const ICMP_HOST *host, const ICMP_PKT *pkt);

ICMP_API unsigned char  icmp_pkt_type(const ICMP_PKT *pkt);
ICMP_API unsigned char  icmp_pkt_code(const ICMP_PKT *pkt);
ICMP_API unsigned short icmp_pkt_cksum(const ICMP_PKT *pkt);
ICMP_API unsigned short icmp_pkt_id(const ICMP_PKT *pkt);
ICMP_API unsigned short icmp_pkt_seq(const ICMP_PKT *pkt);
ICMP_API unsigned int   icmp_pkt_gid(const ICMP_PKT *pkt);
ICMP_API const ICMP_PKT *icmp_pkt_peer(const ICMP_PKT *pkt);
ICMP_API const ICMP_PKT_STATUS *icmp_pkt_status(const ICMP_PKT *pkt);
ICMP_API size_t icmp_pkt_len(const ICMP_PKT *pkt);
ICMP_API size_t icmp_pkt_wlen(const ICMP_PKT *pkt);
ICMP_API size_t icmp_pkt_payload(const ICMP_PKT *pkt, char *buf, size_t size);

ICMP_API size_t icmp_pkt_set_extra(ICMP_PKT *pkt,
		const void *data, size_t len);
ICMP_API void icmp_pkt_set_type(ICMP_PKT *pkt, unsigned char type);
ICMP_API void icmp_pkt_set_code(ICMP_PKT *pkt, unsigned char code);
ICMP_API void icmp_pkt_set_cksum(ICMP_PKT *pkt, unsigned short cksum);
ICMP_API void icmp_pkt_set_id(ICMP_PKT *pkt, unsigned short id);
ICMP_API void icmp_pkt_set_seq(ICMP_PKT *pkt, unsigned short seq);
ICMP_API void icmp_pkt_set_data(ICMP_PKT *pkt, void *data, size_t size);

#ifdef __cplusplus
}
#endif

#endif
