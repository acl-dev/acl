#ifndef __RES_INCLUDE_H__
#define __RES_INCLUDE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>

/**
 * DNS返回结果的存储结构
 */
typedef struct RES {
	char dns_ip[64];                /**< DNS的IP地址 */
	unsigned short dns_port;        /**< DNS的Port */
	unsigned short cur_qid;         /**< 内部变量，数据包的标识 */
	time_t tm_spent;                /**< 查询时间耗费(秒) */
	int   errnum;
#define RES_ERR_SEND	-100    /**< 写出错 */
#define RES_ERR_READ	-101    /**< 读出错 */
#define RES_ERR_RTMO	-102    /**< 读超时 */
#define RES_ERR_NULL	-103    /**< 空结果 */
#define RES_ERR_CONN	-104    /**< TCP方式时连接失败 */

	int transfer;           /**< TCP/UDP 传输模式 */
#define RES_USE_UDP	0       /**< UDP 传输模式 */
#define RES_USE_TCP	1       /**< TCP 传输模式 */

	int   conn_timeout;     /**< TCP 传输时的连接超时时间, 默认为10秒 */
	int   rw_timeout;       /**< TCP/UDP 传输的IO超时时间, 默认为10秒 */
} RES;

/**
 * 创建一个DNS查询对象
 * @param dns_ip {const char*} DNS的IP地址
 * @param dns_port {unsigned short} DNS的Port
 * @return {RES*} 新创建的查询对象
 */
RES *res_new(const char *dns_ip, unsigned short dns_port);

/**
 * 设置DNS查询的超时时间
 * @param conn_timeout {int} TCP 传输时的连接超时时间
 * @param rw_timeout {int} TCP/UDP 传输的IO超时时间
 */
void res_set_timeout(int conn_timeout, int rw_timeout);

/**
 * 释放一个DNS查询对象
 * @param res {RES*} DNS查询对象
 */
void res_free(RES *res);

/**
 * 查询某个域名的IP地址
 * @param res {RES*} DNS查询对象
 * @param domain {const char*} 要查询的域名
 * @return {DNS_DB*} 查询的结果集
 */
DNS_DB *res_lookup(RES *res, const char *domain);

/**
 * 根据错误号获得查询失败的原因
 * @param errnum {int} 错误号
 * @return {const char*} 错误信息
 */
const char *res_strerror(int errnum);

/**
 * 获得当前查询的错误信息
 * @param res {RES*} DNS查询对象
 * @return {const char*} 错误信息
 */
const char *res_errmsg(const RES *res);

#ifdef __cplusplus
}
#endif

#endif

