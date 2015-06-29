#ifndef __PROCTL_INTERNAL_INCLUDE_H__
#define __PROCTL_INTERNAL_INCLUDE_H__

#include "stdlib/acl_define.h"

#if defined(_WIN32) || defined(_WIN64)
#include "stdlib/acl_stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PROCTL_SERVICE {
	HANDLE	hProcess;
	char *filepath;
	ACL_VSTRING *cmdline;
	STARTUPINFO start_info;
	PROCESS_INFORMATION	process_info;
} PROCTL_SERVICE;

typedef struct PROCTL_MSG {
	int  msg_type;
#define PROCTL_MSG_NULL		0		/* 空消息 */
#define PROCTL_MSG_START	1		/* 启动某个服务程序 */
#define PROCTL_MSG_STOP		2		/* 停止某个服务程序 */
#define PROCTL_MSG_QUIT		3		/* 停止所有服务程序并退出 */
#define PROCTL_MSG_LIST		4		/* 列出当前正在运行的服务程序 */
#define PROCTL_MSG_CHECK	5		/* 检查某个服务程序是否在运行 */

	PROCTL_SERVICE *service;
	void *arg;
	void (*free_fn)(void *);
} PROCTL_MSG;

/* in acl_proctl_main.cpp */
extern char *var_progname;

/* in proctl_service.cpp */
PROCTL_MSG *proctl_msg_new(int msg_type);
void proctl_msg_free(PROCTL_MSG *msg);
void proctl_service_init(void);
PROCTL_SERVICE *proctl_service_alloc(const char *filepath, ACL_VSTRING *cmdline);
PROCTL_SERVICE *proctl_service_new(const char *filepath, int argc, char *argv[]);
void proctl_service_free(PROCTL_SERVICE *service);
ACL_ARGV *proctl_serivce_get_all(void);
void proctl_service_free_all(ACL_ARGV *argv);
int proctl_service_exist(const char *filepath);
int proctl_service_start(PROCTL_SERVICE *service);
int proctl_service_wait(void);
int proctl_service_join(void);
void proctl_msg_push(PROCTL_MSG *msg);

/* in proctl_monitor.cpp */
void *proctl_monitor_thread(void *arg);

/* in proctl_child.cpp */
void proctl_child_atexit(void (*onexit_fn)(void*), void *arg);
void *proctl_child_thread(void *arg);

/* in proctl_utils.cpp */
void get_lock_file(char *buf, size_t size);
void get_exec_path(char *buf, size_t size);
void get_lock_file2(const char *filepath, char *buf, size_t size);
int get_addr_from_file(const char *filepath, char *buf, size_t size);
ACL_VSTREAM *local_listen(void);

#ifdef __cplusplus
}
#endif

#endif /* _WIN32 */
#endif

