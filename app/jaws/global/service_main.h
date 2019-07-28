#ifndef	__SERVICE_MAIN_INCLUDE_H__
#define	__SERVICE_MAIN_INCLUDE_H__

#include "lib_acl.h"

#ifdef	__cplusplus
extern "C" {
#endif

/* 全局变量 */
extern ACL_MEM_SLICE *var_mem_slice;

/**
 * 初始化函数，服务器模板框架启动后仅调用该函数一次
 * @param aio {ACL_AIO*} 异步框架句柄
 */
extern void service_init(ACL_AIO *aio, ACL_FIFO *modules);

/**
 * 进程退出时的回调函数
 */
extern void service_exit(void);

/**
 * 协议处理函数入口
 * @param fd {ACL_SOCKET} 客户端数据连接
 * @param aio {ACL_AIO*} 异步框架句柄
 */
extern int service_main(ACL_SOCKET fd, ACL_AIO *aio);

#ifdef	__cplusplus
}
#endif

#endif
