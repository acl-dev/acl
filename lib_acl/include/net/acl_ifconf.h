#ifndef	ACL_IFCONF_INCLUDE_H
#define	ACL_IFCONF_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "stdlib/acl_define.h"

typedef struct ACL_IFADDR {
	char *name;		/* 接口名称 */
#ifdef WIN32
	char *desc;		/* 接口描述 */
#endif
	char  ip[32];		/* 以字符串表示的IP地址 */
	unsigned int addr;	/* 网络字节序的 32 位 IP 地址 */
} ACL_IFADDR;

typedef struct ACL_IFCONF {
	ACL_IFADDR *addrs;	/* ACL_IFADDR 数组 */
	int  length;		/* ACL_IFADDR 数组长度 */

	/* for acl_iterator */

	/* 取迭代器头函数 */
	const ACL_IFADDR *(*iter_head)(ACL_ITER*, struct ACL_IFCONF*);
	/* 取迭代器下一个函数 */
	const ACL_IFADDR *(*iter_next)(ACL_ITER*, struct ACL_IFCONF*);
	/* 取迭代器尾函数 */
	const ACL_IFADDR *(*iter_tail)(ACL_ITER*, struct ACL_IFCONF*);
	/* 取迭代器上一个函数 */
	const ACL_IFADDR *(*iter_prev)(ACL_ITER*, struct ACL_IFCONF*);
} ACL_IFCONF;

/**
 * 获得主机的所有网络地址及网络接口名称
 * @return {ACL_IFCONF*}
 */
ACL_API ACL_IFCONF *acl_get_ifaddrs(void);

/**
 * 释放由 acl_get_ifaddrs() 返回的 ACL_IFCONF 内存
 * @param ifconf {ACL_IFCONF*}
 */
ACL_API void acl_free_ifaddrs(ACL_IFCONF *ifconf);

#ifdef	__cplusplus
}
#endif

#endif
