#ifndef	ACL_NETDB_INCLUDE_H
#define	ACL_NETDB_INCLUDE_H

#include "../stdlib/acl_define.h"

#ifdef	ACL_UNIX
#include <netinet/in.h>
#endif

#include "../stdlib/acl_array.h"
#include "acl_sane_inet.h"

/**
 * 主机地址结构
 */
typedef struct ACL_HOSTNAME ACL_HOST_INFO;
typedef struct ACL_HOSTNAME {
	char  ip[256];                  /**< the ip addr of the HOST */
	ACL_SOCKADDR saddr;		/**< ip addr in ACL_SOCKADDR */
	unsigned int ttl;               /**< the HOST's ip timeout(second) */
	int   hport;
	unsigned int nrefer;            /**< refer number to this HOST */
	unsigned int type;		/**< the content type in ip buf */
#define	ACL_HOSTNAME_TYPE_IPV4		0
#define	ACL_HOSTNAME_TYPE_IPV6		1
#define	ACL_HOSTNAME_TYPE_CNAME		2
} ACL_HOSTNAME;

/**
 * DNS查询结果集
 */
typedef struct ACL_DNS_DB {
	ACL_ARRAY *h_db;
	int   size;
	char  name[256];
	ACL_SOCKADDR ns_addr;

	/* for acl_iterator */

	/* 取迭代器头函数 */
	const ACL_HOSTNAME *(*iter_head)(ACL_ITER*, struct ACL_DNS_DB*);
	/* 取迭代器下一个函数 */
	const ACL_HOSTNAME *(*iter_next)(ACL_ITER*, struct ACL_DNS_DB*);
	/* 取迭代器尾函数 */
	const ACL_HOSTNAME *(*iter_tail)(ACL_ITER*, struct ACL_DNS_DB*);
	/* 取迭代器上一个函数 */
	const ACL_HOSTNAME *(*iter_prev)(ACL_ITER*, struct ACL_DNS_DB*);
	/* 取迭代器关联的当前容器成员结构对象 */
	const ACL_HOSTNAME *(*iter_info)(ACL_ITER*, struct ACL_DNS_DB*);
} ACL_DNS_DB;

/* in acl_netdb.c */

/**
 * 从结果集中取得某个下标位置的主机地址结构
 * @param h_dns_db {const ACL_DNS_DB*} DNS结果集
 * @param i {int} 下标位置
 * @return {const ACL_HOSTNAME*} 返回相应下标的主机地址结构
 */
ACL_API const ACL_HOSTNAME *acl_netdb_index(const ACL_DNS_DB *h_dns_db, int i);

/**
 * 从结果集中取得某个下标位置的主机IP地址
 * @param h_dns_db {const ACL_DNS_DB*} DNS结果集
 * @param i {int} 下标位置
 * @return {const ACL_SOCKADDR*} IP地址结构, NULL表示失败
 */
ACL_API const ACL_SOCKADDR *acl_netdb_index_saddr(ACL_DNS_DB *h_dns_db, int i);

/**
 * 将结果集中的对应某个下标的主机地址引用增加
 * @param h_dns_db {const ACL_DNS_DB*} DNS结果集
 * @param i {int} 下标位置
 * @param n {int} 需要增加的引用值
 */
ACL_API void acl_netdb_refer_oper(ACL_DNS_DB *h_dns_db, int i, int n);

/**
 * 将结果集中的对应某个下标的主机地址引用加1
 * @param h_dns_db {const ACL_DNS_DB*} DNS结果集
 * @param i {int} 下标位置
 */
ACL_API void acl_netdb_refer(ACL_DNS_DB *h_dns_db, int i);

/**
 * 将结果集中的对应某个下标的主机地址引用减1
 * @param h_dns_db {const ACL_DNS_DB*} DNS结果集
 * @param i {int} 下标位置
 */
ACL_API void acl_netdb_unrefer(ACL_DNS_DB *h_dns_db, int i);

/**
 * 将结果集中的对应某个下标的IP地址，以字符串表示
 * @param h_dns_db {const ACL_DNS_DB*} DNS结果集
 * @param i {int} 下标位置
 * @return {const char*} 查得的结果，NULL 表示失败
 */
ACL_API const char *acl_netdb_index_ip(const ACL_DNS_DB *h_dns_db, int i);

/**
 * 取得结果集中主机地址的个数
 * @param h_dns_db {const ACL_DNS_DB*} DNS结果集
 * @return {int} 主机地址个数 > 0, -1 表示参数输入有误
 */
ACL_API int acl_netdb_size(const ACL_DNS_DB *h_dns_db);

/**
 *  释放结果集内存资源
 * @param h_dns_db {ACL_DNS_DB*} DNS结果集
 */
ACL_API void acl_netdb_free(ACL_DNS_DB *h_dns_db);

/**
 * 根据域名创建一个查询结果集的结构，但并不进行DNS查询
 * @param domain {const char*} 要查询的域名
 * @return {ACL_DNS_DB*} 创建的结果集对象
 */
ACL_API ACL_DNS_DB *acl_netdb_new(const char *domain);

/**
 * 设置该 DNS 查询对象所绑定的 DNS 服务器地址
 * @param db {ACL_DNS_DB*} 由 acl_netdb_new 或 acl_netdb_clone 创建
 * @param sa {ACL_SOCKADDR*} DNS 服务器地址
 */
ACL_API void acl_netdb_set_ns(ACL_DNS_DB *db, ACL_SOCKADDR *sa);

/**
 * 向结果集中添加IP地址
 * @param h_dns_db {ACL_DNS_DB*} 查询结果集对象
 * @param ip {const char*} 要添加的IP地址
 */
ACL_API void acl_netdb_addip(ACL_DNS_DB *h_dns_db, const char *ip);

/**
 * 向结果集中添加IP地址及端口号
 * @param h_dns_db {ACL_DNS_DB*} 查询结果集对象
 * @param ip {const char*} 要添加的IP地址
 * @param port {int} 要添加的端口号
 */
ACL_API void acl_netdb_add_addr(ACL_DNS_DB *h_dns_db, const char *ip, int port);

/**
 * 克隆一个查询结果集对象
 * @param h_dns_db {const ACL_DNS_DB*} 源结果集对象
 * @return {ACL_DNS_DB*} 新克隆的结果集对象
 */
ACL_API ACL_DNS_DB *acl_netdb_clone(const ACL_DNS_DB *h_dns_db);

/**
 * 查询某个域名的IP地址集
 * @param name {const char*} 域名
 * @param h_error {int*} 如果查询失败存储出错原因
 * @return {ACL_DNS_DB*} 查询结果集, 如果为NULL则查询失败, 另外，即使返回不为空，
 *  也得需要通过 acl_netdb_size()/1 获得结果集的数组长度
 */
ACL_API ACL_DNS_DB *acl_gethostbyname(const char *name, int *h_error);

/**
 * 根据错误号获得出错提示信息
 * @param errnum {int} 错误号
 * @return {const char*} 出错信息
 */ 
ACL_API const char *acl_netdb_strerror(int errnum);

/* in acl_netdb_cache.c */
/**
 * 向DNS缓存中添加缓存数据
 * @param h_dns_db {const ACL_DNS_DB*} DNS查询结果集
 * @param timeout {int} 该结果集被缓存的超时时间，如果 <= 0, 则采用默认的值，
 *  该默认值是在 acl_netdb_cache_init()/2 中的设置值, 单位为秒
 */
ACL_API void acl_netdb_cache_push(const ACL_DNS_DB *h_dns_db, int timeout);

/**
 * 从DNS缓存中取得DNS查询结果集
 * @param name {const char*} 域名
 * @return {ACL_DNS_DB*} DNS查询结果集
 */
ACL_API ACL_DNS_DB *acl_netdb_cache_lookup(const char *name);

/**
 * 从DNS缓存中删除某个DNS查询结果集
 * @param name {const char*} 域名
 */
ACL_API void acl_netdb_cache_del_host(const char *name);

/**
 * 初始化DNS缓存区
 * @param timeout {int} DNS结果集的默认缓存时间(秒)
 * @param thread_safe {int} 是否需要DNS缓存区线程安全, 0: 表示不需要,
 *  1: 表示需要线程安全
 */
ACL_API void acl_netdb_cache_init(int timeout, int thread_safe);

#endif
