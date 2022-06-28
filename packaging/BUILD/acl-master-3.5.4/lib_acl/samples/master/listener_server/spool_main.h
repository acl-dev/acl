#ifndef	__SPOOL_MAIN_INCLUDE_H__
#define	__SPOOL_MAIN_INCLUDE_H__

#ifdef	__cplusplus
extern "C" {
#endif

#include "lib_util.h"
#include "global.h"

typedef struct SPOOL {
	ACL_SPOOL *h_spool;
} SPOOL;
/*----------------------------------------------------------------------------*/
/* in spool_main.c */
/**
 * 创建一个服务器框架
 * @param max_threads 最大线程数
 * @param idle_timeout 每个线程空闲超时时间
 * @return SPOOL* 服务器连接池句柄
 */
extern SPOOL *spool_create(int max_threads, int idle_timeout);

/**
 * 启动任务工作池
 * @param spool 服务器任务池句柄
 */
extern int spool_start(const SPOOL *spool);

/**
 * 向任务池中添加一个工作任务
 * @param spool 服务器任务池句柄
 * @param cstream 客户端数据流指针
 * 注: cstream 数据流会在该函数内部的回调函数中进行关闭, 所以该函数的调用者不要
 *     关闭该流.
 */
extern void spool_add_worker(SPOOL *spool, ACL_VSTREAM *cstream);
/*----------------------------------------------------------------------------*/

#ifdef	__cplusplus
}
#endif

#endif
