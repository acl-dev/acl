#ifndef	ACL_IFCONF_INCLUDE_H
#define	ACL_IFCONF_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"
#include "../stdlib/acl_argv.h"
#include "acl_sane_inet.h"

typedef struct ACL_IFADDR {
	char name[256];		/* 接口名称 */
#if defined(_WIN32) || defined(_WIN64)
	char desc[256];		/* 接口描述 */
#endif
	char addr[128];		/* 以字符串表示的IP地址 */
	ACL_SOCKADDR saddr;	/* 兼容 IPV4 & IPV6 的地址 */
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
 * @return {ACL_IFCONF*} 返回值非 NULL 时，需调用 acl_free_ifaddrs 释放
 */
ACL_API ACL_IFCONF *acl_get_ifaddrs(void);

/**
 * 释放由 acl_get_ifaddrs() 返回的 ACL_IFCONF 内存
 * @param ifconf {ACL_IFCONF*}
 */
ACL_API void acl_free_ifaddrs(ACL_IFCONF *ifconf);

/**
 * 扫描本机所有网卡 IP，将所有匹配指定模式的 IP 地址返回，目前仅支持 IPV4
 * @param pattern {const char *} 指定的匹配模式，格式为：xxx.xxx.xxx.xxx 或
 *  xxx.xxx.xxx.xxx:port，如：192.168.*.*, 192.168.*.8:80，10.*.0.*:81
 * @return {ACL_IFCONF *} 返回条件的结果集，如果 pattern 后面带有端口，则自动
 *  将端口添加在每个 IP 后面，若返回对象非空，通过调用 acl_free_ifaddrs
 *  释放该对象
 */
ACL_API ACL_IFCONF *acl_ifconf_search(const char *pattern);

/**
 * 扫描本机所有网卡 IP, 将匹配的地址集合返回
 * @param patterns {const char*}
 * @param unix_path {const char*} 当所匹配的地址为 unix domain 类型时, 该路径
 *  为全路径的前缀路径
 * @return {ACL_ARGV*} 返回非空时则返回对象中存放以字符串表示的地址集合, 用完
 *  后需调用 acl_argv_free 释放;如果返回 NULL 则表示没有找到匹配的地址
 */
ACL_API ACL_ARGV *acl_search_addrs(const char *patterns, const char *unix_path);

#ifdef	__cplusplus
}
#endif

#endif
