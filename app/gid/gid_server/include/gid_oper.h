#ifndef	__GID_OPER_INCLUDE_H__
#define	__GID_OPER_INCLUDE_H__

#ifdef	__cplusplus
extern "C" {
#endif

#include "lib_acl.h"

#define	GID_OK			200	/* 正常 */
#define	GID_ERR_SID		500	/* 会话 ID 号不对 */
#define	GID_ERR_OVERRIDE	501	/* 达到最大分配值 */
#define	GID_ERR_SAVE		502	/* 存储至磁盘时出错 */

/**
 * 根据错误号获得字符串描述
 * @param errnum {int} 错误号
 * @return {const char*} 错误描述
 */
const char *gid_serror(int errnum);

/**
 * 获取下一个GID号
 * @param path {const char*} 文件存储路径
 * @param tag {const char*} ID标识号
 * @param step {unsigned int} 每次的步进值
 * @param errnum {int*} 若非空，则记录出错原因
 * @return {acl_int64}, 如果返回值 < 0，则表示分配失败，
 *  出错原因参考 errnum 的返回值
 */
acl_int64 gid_next(const char *path, const char *tag,
	unsigned int step, int *errnum);

/**
 * 初始化，程序启动后应调用此函数初始化内部库
 * @param fh_limit {int} 缓存的文件句柄的最大个数
 * @param sync_gid {int} 每产生一个新的 gid 后是否同时同步至磁盘
 * @param debug_section {int} 调用用的标签值
 */
void gid_init(int fh_limit, int sync_gid, int debug_section);

/**
 * 程序退出前必须调用此函数，以使内存中数据刷新至磁盘
 */
void gid_finish(void);

#ifdef	__cplusplus
}
#endif

#endif
