#ifndef	ACL_DNS_INCLUDE_H
#define	ACL_DNS_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"
#include "../stdlib/acl_htable.h"
#include "../stdlib/acl_cache2.h"
#ifdef  ACL_UNIX
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include "../event/acl_events.h"
#include "../aio/acl_aio.h"
#include "acl_sane_inet.h"
#include "acl_netdb.h"

/* DNS 查询时的错误码定义 */

#define	ACL_DNS_OK			    0
#define	ACL_DNS_OK_CACHE		1
#define	ACL_DNS_ERR_FMT			-1
#define	ACL_DNS_ERR_SVR			-2
#define	ACL_DNS_ERR_NO_EXIST	-3
#define	ACL_DNS_ERR_NO_SUPPORT	-4
#define	ACL_DNS_ERR_DENY		-5
#define	ACL_DNS_ERR_YX			-6
#define	ACL_DNS_ERR_YXRR		-7
#define	ACL_DNS_ERR_NXRR		-8
#define	ACL_DNS_ERR_NO_AUTH		-9
#define	ACL_DNS_ERR_NOT_ZONE	-10
#define	ACL_DNS_ERR_UNPACK		-15
#define	ACL_DNS_ERR_TIMEOUT		-16
#define	ACL_DNS_ERR_EXIST		-17
#define	ACL_DNS_ERR_BUILD_REQ	-18

typedef struct ACL_DNS_ADDR {
	char  ip[64];			/* DNS 服务器地址 */
	unsigned short port;	/* DNS 服务器端口 */
	ACL_SOCKADDR   addr;	/* DNS 地址 */
	int   addr_len;			/* addr 大小 */
	int   mask_length;		/* DNS 服务器所在网络的掩码长度(> 0 && < 32) */
	struct in_addr in;		/* addr 的网段地址 */
} ACL_DNS_ADDR;

typedef struct ACL_DNS {
	ACL_AIO *aio;			/* 异步IO句柄 */
	unsigned short qid;		/* 发送请求的ID标识号 */
	ACL_ASTREAM *astream;	/* 异步流 */

	ACL_ARRAY *groups;		/* 域名组列表 */
	ACL_ARRAY *dns_list;	/* DNS 服务器地址列表 */
	unsigned   dns_idx;		/* 当前使用的 dns_list 数组下标 */
	ACL_DNS_ADDR addr_from;		/* 来源 DNS 地址 */
	ACL_HTABLE *lookup_table;	/* 查询对象表 */
	ACL_CACHE2 *dns_cache;		/* 用于缓存DNS查询结果 */
	int   timeout;			/* 每次查询的超时时间值(秒) */
	int   retry_limit;		/* 查询超时时重试的次数限制 */
	unsigned int flag;		/* 标志位 */
#define	ACL_DNS_FLAG_ALLOC	    	(1 << 0)	/* 该异步句柄是动态分配的 */
#define	ACL_DNS_FLAG_CHECK_DNS_IP	(1 << 1)	/* 检查DNS地址是否匹配 */
#define	ACL_DNS_FLAG_CHECK_DNS_NET	(1 << 2)	/* 检查DNS网络是否匹配 */

	/* 该函数指针用来避免动态加载库的访问地址不一致问题 */
	ACL_EVENT_NOTIFY_TIME lookup_timeout;
} ACL_DNS;

typedef struct ACL_DNS_REQ ACL_DNS_REQ;

/**
 * 初始化DNS异步查询对象结构
 * @param dns {ACL_DNS*} DNS异步查询句柄
 * @param aio {ACL_AIO*} 异步句柄
 * @param timeout {int} 每次DNS查询时的超时值
 * @return {int} 初始化是否成功，返回 0 表示成功，-1 表示失败
 */
ACL_API int acl_dns_init(ACL_DNS *dns, ACL_AIO *aio, int timeout);

/**
 * 创建一个DNS异步查询对象并同时进行初始化
 * @param aio {ACL_AIO*} 异步句柄
 * @param timeout {int} 每次DNS查询时的超时值
 * @return {ACL_DNS*} DNS异步查询句柄，返回 NULL 表示创建 DNS 查询对象失败
 */
ACL_API ACL_DNS *acl_dns_create(ACL_AIO *aio, int timeout);

/**
 * 打开DNS缓存机制
 * @param dns {ACL_DNS*} DNS异步查询句柄
 * @param limit {int} DNS 缓存中最大缓存条目
 */
ACL_API void acl_dns_open_cache(ACL_DNS *dns, int limit);

/**
 * 添加一个DNS服务器地址
 * @param dns {ACL_DNS*} DNS异步查询句柄
 * @param dns_ip {const char*} DNS服务器IP地址
 * @param dns_port {unsigned short} DNS服务器端口
 * @param mask_length {int} DNS服务器所在的网段掩码长度(0 < && < 32)
 */
ACL_API void acl_dns_add_dns(ACL_DNS *dns, const char *dns_ip,
	unsigned short dns_port, int mask_length);

/**
 * 清除 DNS 服务器地址列表
 * @param dns {ACL_DNS*} DNS异步查询句柄
 */
ACL_API void acl_dns_clear_dns(ACL_DNS *dns);

/**
 * 获得 DNS 对象数组列表
 * @param dns {ACL_DNS*} DNS异步查询句柄
 * @return {ACL_ARRAY*} 容纳 ACL_DNS_ADDR 对象的数组对象，返回值永远非 NULL
 */
ACL_API ACL_ARRAY *acl_dns_list(ACL_DNS *dns);

/**
 * 获得 DNS 服务器地址列表的数量
 * @param dns {ACL_DNS*} DNS异步查询句柄
 * @return {size_t}
 */
ACL_API size_t acl_dns_size(ACL_DNS *dns);

/**
 * 判断 DNS 服务器地址列表是否为空
 * @param dns {ACL_DNS*} DNS异步查询句柄
 * @retrn {int} 返回值非 0 表示为空
 */
ACL_API int acl_dns_empty(ACL_DNS *dns);

/**
 * 删除一个DNS服务器地址
 * @param dns {ACL_DNS*} DNS异步查询句柄
 * @param ip {const char*} DNS服务器IP地址
 * @param port {unsigned short} DNS服务器端口
 */
ACL_API void acl_dns_del_dns(ACL_DNS *dns, const char *ip, unsigned short port);

/**
 * 关闭异步查询句柄同时释放所有资源
 * @param dns {ACL_DNS*} DNS异步查询句柄
 */
ACL_API void acl_dns_close(ACL_DNS *dns);

/**
 * 设置标志位，检查DNS来源IP地址是否与目标地址相同，若不同则丢弃所读的
 * 数据包，主要是为了防止DNS查询时的UDP攻击
 * @param dns {ACL_DNS*} DNS异步查询句柄
 */
ACL_API void acl_dns_check_dns_ip(ACL_DNS *dns);

/**
 * 设置标志位，检查DNS来源IP所在网段是否与目标网段相同，若不同则丢弃
 * 所读的数据包，主要是为了防止DNS查询时的UDP攻击
 * @param dns {ACL_DNS*} DNS异步查询句柄
 */
ACL_API void acl_dns_check_dns_net(ACL_DNS *dns);

/**
 * 设置DNS查询超时时重试次数
 * @param dns {ACL_DNS*} DNS异步查询句柄
 * @param retry_limit {int} 重试次数
 */
ACL_API void acl_dns_set_retry_limit(ACL_DNS *dns, int retry_limit);

/**
 * 异步查询一个域所对应的A记录IP地址集合
 * @param dns {ACL_DNS*} DNS异步查询句柄
 * @param domain {const char*} 域名
 * @param callback {void (*)(ACL_DNS_DB*, void*)} 查询成功或失败的回调函数,
 *  若返回给 callback 的 ACL_DNS_DB 为空则表示查询失败, 第二个参数为用户设置
 *  的参数, 第三个参数为查询失败时的错误号
 * @param ctx {void*} callback 的参数之一
 */
ACL_API void acl_dns_lookup(ACL_DNS *dns, const char *domain,
	void (*callback)(ACL_DNS_DB*, void*, int), void *ctx);

/**
 * 向DNS查询对象中添加静态主机信息
 * @param dns {ACL_DNS*} DNS异步查询句柄
 * @param domain {const char*} 域名
 * @param ip_list {const char*} IP地址列表，分隔符为 ';'，如: 192.168.0.1;192.168.0.2
 */
ACL_API void acl_dns_add_host(ACL_DNS *dns, const char *domain, const char *ip_list);

/**
 * 向DNS查询对象中添加查询域名组信息
 * @param dns {ACL_DNS*} DNS异步查询句柄
 * @param group {const char*} 域名组名，如: .test.com, 则 a.test.com, b.test.com
 *  都属于 .test.com 域名组
 * @param ip_list {const char*} 如果非空则采用静态方式添加IP地址列表
 * @param refer {const char*} 域名组的代表域名, 将会用此域名代表整个域名组去做DNS查询
 * @param excepts {ACL_ARGV*} 虽然这些域名属于 group 的子域名但却不属于其域名组的
 *  成员集合
 */
ACL_API void acl_dns_add_group(ACL_DNS *dns, const char *group, const char *refer,
		const char *ip_list, const char *excepts);
/**
 * 取消某个查询事件对象
 * @param handle {ACL_DNS_REQ*} 某次域名查询事件
 */
ACL_API void acl_dns_cancel(ACL_DNS_REQ *handle);

/**
 * 根据错误号得到错误描述信息
 * @param errnum {int} DNS查询时返回的错误号
 * @return {const char*} 错误描述信息
 */
ACL_API const char *acl_dns_serror(int errnum);

#ifdef	__cplusplus
}
#endif

#endif

