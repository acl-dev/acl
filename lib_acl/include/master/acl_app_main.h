#ifndef	__ACL_APP_MAIN_INCLUDE_H__
#define	__ACL_APP_MAIN_INCLUDE_H__

#include "lib_acl.h"

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef ACL_UNIX

/* 客户端读写超时时间值 */
extern int acl_app_var_client_idle_limit;

/* 用户级的运行函数类型, 当该函数返回值 != 0 时, 框架会自动关闭客户端流 */
typedef int (*ACL_IOCTL_RUN_FN)(ACL_VSTREAM *stream, void *run_ctx);

/* 非阻塞IO服务器模板的运行函数类型 */
typedef int (*ACL_AIO_RUN_FN)(ACL_ASTREAM *stream, void *run_ctx);
typedef int (*ACL_AIO_RUN2_FN)(ACL_SOCKET fd, void *run_ctx);

/* 用户级的初始化函数类型 */
typedef void (*ACL_APP_INIT_FN)(void*);
typedef void (*ACL_APP_EXIT_FN)(void*);
typedef void (*ACL_APP_PRE_JAIL)(void*);

/* 使用用户自己的日志管理函数 */
typedef void (*ACL_APP_OPEN_LOG)(void);
typedef void (*ACL_APP_CLOSE_LOG)(void);

typedef ACL_MASTER_SERVER_THREAD_INIT_FN ACL_APP_THREAD_INIT_FN  /* void (*)(void*) */;
typedef ACL_MASTER_SERVER_THREAD_EXIT_FN ACL_APP_THREAD_EXIT_FN;  /* void (*)(void*) */

#define	ACL_APP_CTL_END			0	/* 参数控制结束标志 */
#define	ACL_APP_CTL_INIT_FN		1	/* 初始化函数 */
#define	ACL_APP_CTL_INIT_CTX		2	/* 初始化函数所用的参数 */
#define	ACL_APP_CTL_CFG_BOOL		3	/* 整数类型的配置参数表 */
#define	ACL_APP_CTL_CFG_INT		4	/* 整数类型的配置参数表 */
#define	ACL_APP_CTL_CFG_STR		5	/* 字符串类型的配置参数表 */
#define	ACL_APP_CTL_EXIT_FN		6	/* 当进程退出时的回调函数 */
#define	ACL_APP_CTL_EXIT_CTX		7	/* 进程退出时回调函数的参数 */
#define	ACL_APP_CTL_THREAD_INIT		8	/* 每个线程启动时的回调函数 */
#define	ACL_APP_CTL_THREAD_INIT_CTX	9	/* 线程启动时回调函数的参数 */
#define	ACL_APP_CTL_THREAD_EXIT		10	/* 线程退出时的回调函数 */
#define	ACL_APP_CTL_THREAD_EXIT_CTX	11	/* 线程退出时回调函数的参数 */
#define	ACL_APP_CTL_DENY_INFO		12	/* 当非法客户端访问时给出的提示信息 */
#define	ACL_APP_CTL_OPEN_LOG		13	/* 日志初始化，可以通过此参数设置用户自己的日志函数 */
#define	ACL_APP_CTL_CLOSE_LOG		14	/* 关闭用户自己的日志函数 */
#define	ACL_APP_CTL_CFG_INT64		15	/* 64 位整数类型的配置参数表 */
#define	ACL_APP_CTL_ON_ACCEPT		16	/* 设置回调函数来处理新接收到的远程连接 */
#define	ACL_APP_CTL_PRE_JAIL		17	/* 设置进程切换身份前的回调函数 */
#define	ACL_APP_CTL_PRE_JAIL_CTX	18	/* 设置进程切换身份前回调函数的参数 */
#define	ACL_APP_CTL_ON_TIMEOUT		19	/* 当 IO 读写超时时控制的回调函数 */
#define	ACL_APP_CTL_ON_CLOSE		20	/* 当 IO 关闭时控制的回调函数 */
/*----------------------------------------------------------------------------*/
/* in app_main.c */

/**
 * 主函数入口, 用户级的初始化函数指针及运行函数指针通过控制参数进行注册, 主函数内部
 * 会在初始化时自动调用用户级初始化函数(ACL_APP_INIT_FN 类型), 当接收到允许访问的客户端
 * 连接时会自动调用用户(ACL_APP_RUN_FN 类型).
 * 级的运行函数.
 * @param argc "int main(int argc, char *argv[])" 中的 argc
 * @param argv "int main(int argc, char *argv[])" 中的 argv
 * @param run_fn 用户级运行主函数
 * @param run_ctx run_fn() 运行时的参数之一
 * @param name 控制参数中的第一个控制类型, 所支持的类型如上定义: ACL_APP_CTL_XXX
 *        调用方式: ACL_APP_CTL_XXX, xxx; 其中 ACL_APP_CTL_END 为特殊的控制参数, 表示控制参数
 *        结束.
 * @example:
 *   acl_xxx_app_main(argc, argv, {run_fn}, {run_ctx},
 *		ACL_APP_CTL_INIT_FN, {run_init_fn},
 *		ACL_APP_CTL_INIT_CTX, {run_init_ctx},
 *		ACL_APP_CTL_END);
 * 注: acl_xxx_app_main() 的所有参数中, argc, argv, run_fn, run_ctx(可以为NULL), ACL_APP_CTL_END
 *     都是必需的.
 */

void acl_ioctl_app_main(int argc, char *argv[], ACL_IOCTL_RUN_FN run_fn, void *run_ctx, int name, ...);
void acl_aio_app_main(int argc, char *argv[], ACL_AIO_RUN_FN run_fn, void *run_ctx, int name, ...);
void acl_aio_app2_main(int argc, char *argv[], ACL_AIO_RUN2_FN run2_fn, void *run_ctx, int name, ...);

/*----------------------------------------------------------------------------*/

#endif

#ifdef	__cplusplus
}
#endif

#endif
