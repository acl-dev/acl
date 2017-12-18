#ifndef	__NETDB_INCLUDE_H__
#define	__NETDB_INCLUDE_H__

/**
 * 主机地址结构
 */
typedef struct HOSTNAME HOST_INFO;
typedef struct HOSTNAME {
	char  ip[64];                   /**< the ip addr of the HOST */
	struct sockaddr_in saddr;	/**< ip addr in sockaddr_in */
	unsigned int ttl;               /**< the HOST's ip timeout(second) */
	int   hport;
	unsigned int nrefer;            /**< refer number to this HOST */
} HOSTNAME;

/**
 * DNS查询结果集
 */
typedef struct DNS_DB {
	ARRAY *h_db;
	int   size;
	char  name[256];

	/* for iterator */

	/* 取迭代器头函数 */
	const HOST_INFO *(*iter_head)(ITER*, struct DNS_DB*);
	/* 取迭代器下一个函数 */
	const HOST_INFO *(*iter_next)(ITER*, struct DNS_DB*);
	/* 取迭代器尾函数 */
	const HOST_INFO *(*iter_tail)(ITER*, struct DNS_DB*);
	/* 取迭代器上一个函数 */
	const HOST_INFO *(*iter_prev)(ITER*, struct DNS_DB*);
	/* 取迭代器关联的当前容器成员结构对象 */
	const HOST_INFO *(*iter_info)(ITER*, struct DNS_DB*);
} DNS_DB;

/**
 *  释放结果集内存资源
 * @param h_dns_db {DNS_DB*} DNS结果集
 */
void netdb_free(DNS_DB *h_dns_db);

/**
 * 根据域名创建一个查询结果集的结构，但并不进行DNS查询
 * @param domain {const char*} 要查询的域名
 * @return {DNS_DB*} 创建的结果集对象
 */
DNS_DB *netdb_new(const char *domain);

#endif
