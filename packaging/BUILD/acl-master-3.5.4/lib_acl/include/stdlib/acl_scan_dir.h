#ifndef ACL_SCAN_DIR_INCLUDE_H
#define ACL_SCAN_DIR_INCLUDE_H

#include <sys/stat.h>

#ifdef  __cplusplus
extern "C" {
#endif

#include "acl_define.h"
/**
 * 目录扫描句柄类型定义
 */
typedef struct ACL_SCAN_DIR ACL_SCAN_DIR;

/**
 * 目录扫描过程中用户可以设置的回调函数类型定义
 * @param scan {ACL_SCAN_DIR*} 目录扫描指针
 * @param ctx {void*} 用户参数指针
 */
typedef int (*ACL_SCAN_DIR_FN)(ACL_SCAN_DIR *scan, void *ctx);

/**
 * 目录扫描过程中，如果遇到空目录且用户设置了自动删除空目录标记，则回调此方法
 * 通知用户删除指定的空目录
 * @param scan {ACL_SCAN_DIR*} 目录扫描指针
 * @param ctx {void*} 用户参数指针
 */
typedef int (*ACL_SCAN_RMDIR_FN)(ACL_SCAN_DIR *scan, const char *path, void *ctx);

/**
 * 打开扫描路径, 为整个 acl_scan_dir 函数库的初始化函数
 * @param path {const char*} 要打开的路径名称
 * @param recursive {int} 是否需要递归扫描子目录
 * @return {ACL_SCAN_DIR*} NULL: Err; != NULL, OK
 */
ACL_API ACL_SCAN_DIR *acl_scan_dir_open(const char *path, int recursive);

/**
 * 打开扫描路径, 为整个 acl_scan_dir 函数库的初始化函数
 * @param path {const char*} 要打开的路径名称
 * @param flags {unsigned} 标志位, 见 ACL_SCAN_FLAG_XXX
 * @return {ACL_SCAN_DIR*} NULL: Err; != NULL, OK
 */
ACL_API ACL_SCAN_DIR *acl_scan_dir_open2(const char *path, unsigned flags);
#define ACL_SCAN_FLAG_RECURSIVE	(1 << 0)	/* 是否做递归扫描 */
#define ACL_SCAN_FLAG_RMDIR	(1 << 1)	/* 是否自动删除空目录 */

/**
 * 关闭扫描句柄
 * @param scan {ACL_SCAN_DIR*} 类型指针
 */
ACL_API void acl_scan_dir_close(ACL_SCAN_DIR *scan);

/**
 * 将目录扫描句柄的与统计信息相关的变量置0
 * @param scan {ACL_SCAN_DIR*} 类型指针
 */
ACL_API void acl_scan_dir_reset(ACL_SCAN_DIR *scan);

/**
 * 通过此接口设置扫描句柄的回调函数、参数等，当最后的一个控制标志
 * 为 ACL_SCAN_CTL_END 时表示控制参数列表结束
 * @param scan {ACL_SCAN_DIR*} 类型指针
 * @param name {int} 第一个控制项， ACL_SCAN_CTL_XXX
 */
ACL_API void acl_scan_dir_ctl(ACL_SCAN_DIR *scan, int name, ...);
#define ACL_SCAN_CTL_END	0  /**< 控制结束标志 */
#define ACL_SCAN_CTL_FN		1  /**< 设置 ACL_SCAN_DIR_FN 标志 */
#define ACL_SCAN_CTL_CTX	2  /**< 设置用户参数 */
#define ACL_SCAN_CTL_RMDIR_FN	3  /**< 设置删除目录回调函数 */

/**
 * 获得当前状态下的相对路径(相对于程序调用 acl_scan_dir_open
 * 函数时的程序运行路径)
 * @param scan {ACL_SCAN_DIR*} 类型指针
 * @return {const char*} 相对路径, == NULL: err; != NULL, OK
 */
ACL_API const char *acl_scan_dir_path(ACL_SCAN_DIR *scan);

/**
 * 当前所扫描的文件名，如果扫描的对象不是文件，则返回 "\0"
 * @param scan {ACL_SCAN_DIR*} 类型指针
 * @return {const char*} 文件名
 */
ACL_API const char *acl_scan_dir_file(ACL_SCAN_DIR *scan);

/**
 * 当前已经扫描的目录总个数
 * @param scan {ACL_SCAN_DIR*}
 * @return {unsigned} 目录总个数
 */
ACL_API unsigned acl_scan_dir_ndirs(ACL_SCAN_DIR *scan);

/**
 * 当前已经扫描的文件总个数
 * @param scan {ACL_SCAN_DIR*}
 * @return {unsigned} 文件总个数
 */
ACL_API unsigned acl_scan_dir_nfiles(ACL_SCAN_DIR *scan);

/**
 * 当前已经扫描的文件大小总和
 * @param scan {ACL_SCAN_DIR*}
 * @return {acl_int64} -1: Error; >= 0: Ok
 */
ACL_API acl_int64 acl_scan_dir_nsize(ACL_SCAN_DIR *scan);

/**
 * 取得当前扫描到的文件或目录的属性信息，类似于标准的 stat() 函数
 * @param scan {ACL_SCAN_DIR*} 类型指针
 * @param sbuf {struct acl_stat*} 类型指针
 * @return {int} 0: Ok, -1: Error
 */
ACL_API int acl_scan_stat(ACL_SCAN_DIR *scan, struct acl_stat *sbuf);

/**
 * 取得当前正在扫描的目录节点的属性信息，该 API 不同于 acl_scan_stat
 * @param scan {ACL_SCAN_DIR*} 类型指针
 * @param sbuf {struct acl_stat*} 类型指针
 * @return {int} 0: Ok, -1: Error
 */
ACL_API int acl_scan_dir_stat(ACL_SCAN_DIR *scan, struct acl_stat *sbuf);

/**
 * 目录是否扫描完毕
 * @param scan {ACL_SCAN_DIR*} 类型指针
 * @return {int} 0: 表示未扫描完毕； !=0: 表示扫描完毕
 */
ACL_API int acl_scan_dir_end(ACL_SCAN_DIR *scan);

/**
 * 将需要进行扫描的相对路径压栈
 * @param scan {ACL_SCAN_DIR*} 类型指针
 * @param path {const char*} 需要压栈的相对路径
 * @return {int} 0: OK; -1: Err
 */
ACL_API int acl_scan_dir_push(ACL_SCAN_DIR *scan, const char *path);

/**
 * 弹出下一个路径
 * @param scan {ACL_SCAN_DIR*} 类型指针
 * @return {ACL_SCAN_DIR*} 返回堆栈中的下一个对象, == NULL: 结束; != NULL, OK
 */
ACL_API ACL_SCAN_DIR *acl_scan_dir_pop(ACL_SCAN_DIR *scan);

/**
 * 获得 scan 当前所在路径中下一个路径名或文件名, 注意，该函数内部不会递归扫描,
 * 即 acl_scan_dir_open 中的参数 recursive 对该函数无效
 *  1、 ".." 与 "." 不包含在内
 *  2、 仅返回名称, 不包括路径, 路径可以由 acl_scan_dir_path 获得
 * @param scan {ACL_SCAN_DIR*} 指针地址
 * @return {const char*} 为目录名称或文件名称, != NULL: OK; == NULL, 扫描完毕
 */
ACL_API const char *acl_scan_dir_next(ACL_SCAN_DIR *scan);

/**
 * 获得下一个文件名(不包含路径名, 相对路径名可以通过 acl_scan_dir_path 获得),
 * 该函数内部支持递归扫描目录功能, acl_scan_dir_open 中的参数 recursive 对该函数有效
 * @param scan {ACL_SCAN_DIR*} 类型指针
 * @return {const char*} 返回下一个扫描的文件名: !NULL, OK; NULL 扫描完毕，应结束扫描
 */
ACL_API const char *acl_scan_dir_next_file(ACL_SCAN_DIR *scan);

/**
 * 获得下一个目录名(不包含路径名, 相对路径名可以通过 acl_scan_dir_path 获得),
 * 该函数内部支持递归扫描目录功能, acl_scan_dir_open 中的参数 recursive 对该函数有效
 * @param scan {ACL_SCAN_DIR*} 类型指针
 * @return {const char*} 返回下一个扫描的目录名: !NULL, OK; NULL 扫描完毕, 应结束扫描
 */
ACL_API const char *acl_scan_dir_next_dir(ACL_SCAN_DIR *scan);

/**
 * 获得下一个目录名或文件名(不包含路径名, 相对路径名可以通过 acl_scan_dir_path 获得),
 * 该函数内部支持递归扫描目录功能, acl_scan_dir_open 中的参数 recursive 对该函数有效
 * @param scan {ACL_SCAN_DIR*} 类型指针
 * @param is_file {int*} 当返回结果非空时，该地址存储的值表示所扫描到的是否是
 *  文件，如果为 true 则为文件，否则为目录
 * @return {const char*} 返回下一个扫描的目录名或文件名: !NULL, OK; NULL 扫描完毕,
 *  应结束扫描
 */
ACL_API const char *acl_scan_dir_next_name(ACL_SCAN_DIR *scan, int *is_file);

/**
 * 取得当前目录下所占磁盘空间大小(以字节计算)
 * 该函数内部支持递归扫描目录功能, acl_scan_dir_open 中的参数 recursive 对该函数有效
 * @param scan {ACL_SCAN_DIR*} 打开目录时的扫描句柄
 * @param nfile {int*} 扫描完后记录所扫描的文件总数
 * @param ndir {int*} 扫描完后记录所扫描的目录总数
 * @return {acl_int64} -1: Error; >= 0: Ok
 */
ACL_API acl_int64 acl_scan_dir_size2(ACL_SCAN_DIR *scan, int *nfile, int *ndir);

/**
 * 取得当前目录下所占磁盘空间大小(以字节计算)
 * @param pathname {const char*} 目录路径名
 * @param recursive {int} 是否要递归扫描该目录下的所有子目录
 * @param nfile {int*} 扫描完后记录所扫描的文件总数
 * @param ndir {int*} 扫描完后记录所扫描的目录总数
 * @return {acl_int64} -1: Error, >= 0: Ok
 */
ACL_API acl_int64 acl_scan_dir_size(const char *pathname, int recursive,
		int *nfile, int *ndir);

/**
 * 删除所给路径下所有的文件及目录
 * @param nfile {int*} 扫描完后记录所扫描的文件总数
 * @param ndir {int*} 扫描完后记录所扫描的目录总数
 * 该函数内部支持递归扫描目录功能, acl_scan_dir_open 中的参数 recursive 对该函数有效
 * @param scan {ACL_SCAN_DIR*} 打开目录时的扫描句柄
 * @return {acl_int64} >= 0: 实际删除的文件数与目录数的尺寸大小之和(字节); < 0: 出错.
 */
ACL_API acl_int64 acl_scan_dir_rm2(ACL_SCAN_DIR *scan, int *nfile, int *ndir);

/**
 * 删除所给路径下所有的文件及目录
 * @param pathname {const char*} 路径名
 * @param recursive {int} 是否递归删除所有子目录及子目录下的文件
 * @param ndir {int*} 若该参数非空，过程结束后 *ndir 等于总共删除的目录数目
 * @param nfile {int*} 若该参数非空，过程结束后 *nfile 等于总共删除的文件数目
 * @return {acl_int64} >= 0: 实际删除的文件数与目录数的尺寸大小之和(字节); < 0: 出错.
 */
ACL_API acl_int64 acl_scan_dir_rm(const char *pathname, int recursive,
		int *ndir, int *nfile);

#ifdef  __cplusplus
}
#endif

#endif


