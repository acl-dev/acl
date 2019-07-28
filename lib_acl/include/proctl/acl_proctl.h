#ifndef ACL_PROCTL_INCLUDE_H
#define ACL_PROCTL_INCLUDE_H

#include "../stdlib/acl_define.h"
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 获取控制进程的执行程序所在的路径位置
 * @param buf {char*} 存储结果的内存位置, 返回的结果的结尾
 *  不包含 "\" 或 "/",　如："C:\\test_path\\test1_path", 而不是
 *  "C:\\test_path\\test1_path\\"
 * @param size {size_t} buf 的空间大小
 */
ACL_API void acl_proctl_daemon_path(char *buf, size_t size);

/**
 * 初始化进程控制框架（仅 acl_proctl_start 需要）
 * @param progname {const char*} 控制进程进程名
 */
ACL_API void acl_proctl_deamon_init(const char *progname);

/**
 * 控制进程作为后台服务进程运行，监视所有子进程的运行状态，
 * 如果子进程异常退出则会重启该子进程
 */
ACL_API void acl_proctl_daemon_loop(void);

/**
 * 在控制进程启动后，启动一个子进程
 * @param progchild {const char*} 子进程的程序名
 * @param argc {int} argv 数组的长度
 * @param argv {char* []}
 * @return 0: ok; -1: error
 */
ACL_API int acl_proctl_deamon_start_one(const char *progchild, int argc, char *argv[]);

/**
 * 以命令方式启动某个子进程
 * @param progname {const char*} 控制进程进程名
 * @param progchild {const char*} 子进程进程名
 * @param argc {int} argv 数组的长度
 * @param argv {char* []} 传递给子进程的参数
 */
ACL_API void acl_proctl_start_one(const char *progname,
	const char *progchild, int argc, char *argv[]);

/**
 * 以命令方式停止某个子进程
 * @param progname {const char*} 控制进程进程名
 * @param progchild {const char*} 子进程进程名
 * @param argc {int} argv 数组的长度
 * @param argv {char* []} 传递给子进程的参数
 */
ACL_API void acl_proctl_stop_one(const char *progname,
	const char *progchild, int argc, char *argv[]);

/**
 * 以命令方式停止所有的子进程
 * @param progname {const char*} 控制进程进程名
 */
ACL_API void acl_proctl_stop_all(const char *progname);

/**
 * 以命令方式通知控制进程停止所有的子进程，并在子进程退出后控制进程也自动退出
 * @param progname {const char*} 控制进程进程名
 */
ACL_API void acl_proctl_quit(const char *progname);

/**
 * 列出当前所有正在运行的服务进程
 * @param progname {const char*} 控制进程进程名
 */
ACL_API void acl_proctl_list(const char *progname);

/**
 * 探测某个服务进程是否在运行
 * @param progname {const char*} 控制进程进程名
 * @param progchild {const char*} 子进程进程名
 */
ACL_API void acl_proctl_probe(const char *progname, const char *progchild);

/**
 * 子进程调用接口，通过此接口与父进程之间建立控制/被控制关系
 * @param progname {const char*} 子进程进程名
 * @param onexit_fn {void (*)(void*)} 如果非空则当子进程退出时调用的回调函数
 * @param arg {void*} onexit_fn 参数之一
 */
ACL_API void acl_proctl_child(const char *progname, void (*onexit_fn)(void *), void *arg);

#ifdef __cplusplus
}
#endif

#endif
