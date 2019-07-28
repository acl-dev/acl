
#ifndef	__SERVICE_MAIN_INCLUDE_H__
#define	__SERVICE_MAIN_INCLUDE_H__

#include "lib_acl.h"

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * 初始化函数，服务器模板框架启动后仅调用该函数一次
 * @param init_ctx {void*} 用户自定义类型指针
 */
extern void service_init(void *init_ctx);

/**
 * 进程退出时的回调函数
 * @param exist_ctx {void*} 用户自定义类型指针
 */
extern void service_exit(void *exit_ctx);

/**
 * 当服务器接收到新连接时调用此函数
 * @param stream {ACL_VSTREAM*} 客户端数据连接流
 * @return {int} 如果返回值 < 0， 则表示服务器希望关闭该连接
 */
extern int service_on_accept(void *ctx, ACL_VSTREAM *client);

/**
 * 协议处理函数入口
 * @param stream {ACL_VSTREAM*} 客户端数据连接流
 * @param run_ctx {void*} 用户自定义类型指针
 */
extern int service_main(void *run_ctx, ACL_VSTREAM *stream);

#ifdef	__cplusplus
}
#endif

#endif
