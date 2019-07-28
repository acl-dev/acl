#ifndef	ACL_PROCESS_INCLUDE_H
#define	ACL_PROCESS_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * 程序运行过程中获得可执行程序存储于文件系统中的全路径
 * @return {const char*} NULL: 无法获得; != NULL: 返回值即是程序在
 *    文件系统上的存储全路径
 */
ACL_API const char *acl_process_path(void);

/**
 * 程序运行过程中获得其运行路径
 * @return {const char*} NULL: 无法获得; != NULL: 返回值即为程序的运行路径
 */
ACL_API const char *acl_getcwd(void);

#ifdef	__cplusplus
}
#endif

#endif
