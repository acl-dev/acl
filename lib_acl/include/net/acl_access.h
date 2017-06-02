#ifndef	ACL_ACCESS_INCLUDE_H
#define	ACL_ACCESS_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../stdlib/acl_stdlib.h"

/**
 * 向访问列表中添加一个允许的 ip 地址段
 * @param data 多个 IP 地址段数据字符串. 如: 10.0.0.1:10.0.250.1, 192.168.0.1:192.168.0.255
 * @param sep1 每个 IP 地址段之间的分隔符, 如上例中的 "," 分隔符
 * @param sep2 每个 IP 地址段高地址与低地址之间的分隔符, 如上例中的 ":" 分隔符
 * @return 添加　结果. 0: 成功; < 0: 失败
 * 注: 该函数是线程不安全的
 */
ACL_API int acl_access_add(const char *data, const char *sep1, const char *sep2);

/**
 * 从配置文件中读取 IP 地址字符串, 并自动生成 IP 地址访问列表
 * @param xcp 已经成功分析了配置文件的结果句柄
 * @param name xcp 结果句柄中与 IP 地址访问相关的变量名
 * @return 是否添加成功. 0: 成功; < 0: 失败.
 * 注: 该函数是线程不安全的
 */
ACL_API int acl_access_cfg(ACL_XINETD_CFG_PARSER *xcp, const char *name);

/**
 * 用户可以设置自己的日志记录函数, 如果不调用此函数, 则本库自动使用 aclMsg.c中的库.
 * @param log_fn 用户自己的日志记录函数.
 * 注: 该函数是线程不安全的
 */
ACL_API void acl_access_setup_logfn(void (*log_fn)(const char *fmt, ...));

/**
 * 判定给定 IP 地址是否在允许的访问 IP 地址列表中.
 * @param ip 格式: 192.168.0.1
 * @return 是否在允许的访问列表中, != 0: 是; == 0: 不是.
 */
ACL_API int acl_access_permit(const char *ip);

/**
 * 将访问地址表表打印出来.
 */
ACL_API void acl_access_debug(void);

#ifdef	__cplusplus
}
#endif

#endif
