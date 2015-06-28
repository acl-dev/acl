#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include "stdlib/acl_stdlib.h"
#include "proctl/acl_proctl.h"

#endif  /* ACL_PREPARE_COMPILE */

#ifdef ACL_WINDOWS

#include "net/acl_net.h"
#include "thread/acl_thread.h"
#include "proctl_internal.h"
#include <windows.h>

char *var_progname = NULL;

static void proctl_init(const char *progname)
{
	acl_socket_init();
}

static void proctl_start_init(const char *progname)
{
	proctl_init(progname);
	proctl_service_init();
}

void acl_proctl_daemon_path(char *buf, size_t size)
{
	const char *myname = "acl_proctl_daemon_path";

	if (buf == NULL || size <= 0) {
		acl_msg_error("%s(%d): input invalid", myname, __LINE__);
		return;
	}
	get_exec_path(buf, size);
}

void acl_proctl_deamon_init(const char *progname)
{
	acl_pthread_attr_t attr;
	acl_pthread_t tid;

	var_progname = acl_mystrdup(progname);
	proctl_start_init(var_progname);
	acl_pthread_attr_init(&attr);
	(void) acl_pthread_attr_setdetachstate(&attr, 1);
	acl_pthread_create(&tid, &attr, proctl_monitor_thread, NULL);
}

void acl_proctl_daemon_loop()
{
	const char *myname = "acl_proctl_daemon_loop";
	time_t tm_start, tm_end;

	while (1) {
		tm_start = time(NULL);
		proctl_service_wait();
		proctl_service_join();
		tm_end = time(NULL);

		if (tm_end - tm_start <= 1) {
			acl_msg_warn("%s(%d): start process too fast, sleep 2 second",
				myname, __LINE__);
			sleep(2);
		}
	}
}

int acl_proctl_deamon_start_one(const char *progchild, int argc, char *argv[])
{
	const char *myname = "acl_proctl_deamon_start_one";
	PROCTL_SERVICE *service;

	if (proctl_service_exist(progchild)) {
		acl_msg_error("%s(%d): child(%s) maybe be running!",
			myname, __LINE__, progchild);
		return (-1);
	}

	service = proctl_service_new(progchild, argc, argv);
	if (proctl_service_start(service) < 0) {
		proctl_service_free(service);
		return (-1);
	}

	return (0);
}

/* 打开与控制进程的监听线程之间的数据连接 */
static ACL_VSTREAM *proctl_client_open(const char *progname)
{
	const char *myname = "proctl_client_open";
	char  ebuf[256], lock_file[MAX_PATH], addr[256];
	ACL_VSTREAM *client;

	proctl_init(progname);

	get_lock_file(lock_file, sizeof(lock_file));

	if (get_addr_from_file(lock_file, addr, sizeof(addr)) < 0)
		acl_msg_fatal("%s(%d): get addr from file(%s) error(%s)",
		myname, __LINE__, lock_file, acl_last_strerror(ebuf, sizeof(ebuf)));

	client = acl_vstream_connect(addr, ACL_BLOCKING, 10, 10, 1024);
	if (client == NULL)
		acl_msg_fatal("%s(%d): connect addr(%s) error(%s)",
		myname, __LINE__, addr, acl_last_strerror(ebuf, sizeof(ebuf)));

	return (client);
}

/* 关闭与控制进程的监听线程之间的数据连接 */
static void proctl_client_close(ACL_VSTREAM *client)
{
	acl_vstream_close(client);
}

void acl_proctl_start_one(const char *progname,
	const char *progchild, int argc, char *argv[])
{
	const char *myname = "acl_proctl_start_one";
	char  ebuf[256], buf[1024];
	ACL_VSTREAM *client;
	ACL_VSTRING *child_args = NULL;
	int   n;

	if (argc > 0) {
		int   i;

		child_args = acl_vstring_alloc(256);
		for (i = 0; i < argc; i++) {
			if (i > 0)
				acl_vstring_strcat(child_args, " ");
			acl_vstring_strcat(child_args, "\"");
			acl_vstring_strcat(child_args, argv[i]);
			acl_vstring_strcat(child_args, "\"");
		}

	}

	/* 打开与控制进程之间的连接 */
	client = proctl_client_open(progname);
	if (child_args) {
		/* 向控制进程发送消息，带有控制参数 */
		n = acl_vstream_fprintf(client, "%s|-d|START|-f|%s|-a|%s\r\n",
				progname, progchild, acl_vstring_str(child_args));
	} else {
		/* 向控制进程发送消息，不带控制参数 */
		n = acl_vstream_fprintf(client, "%s|-d|START|-f|%s\r\n",
				progname, progchild);
	}

	if (child_args != NULL)
		acl_vstring_free(child_args);

	if (n == ACL_VSTREAM_EOF)
		acl_msg_fatal("%s(%d): fprintf to acl_proctl error(%s)",
			myname, __LINE__, acl_last_strerror(ebuf, sizeof(ebuf)));

	/* 接收所有来自于控制进程的消息响应结果 */
	while (1) {
		n = acl_vstream_gets_nonl(client, buf, sizeof(buf));
		if (n == ACL_VSTREAM_EOF)
			break;
		acl_msg_info("%s(%d): %s", myname, __LINE__, buf);
	}

	proctl_client_close(client);
}

void acl_proctl_stop_one(const char *progname,
	const char *progchild, int argc, char *argv[])
{
	const char *myname = "acl_proctl_stop_one";
	char  ebuf[256], buf[1024];
	ACL_VSTREAM *client;
	ACL_VSTRING *child_args = NULL;
	int   n;

	if (argc > 0) {
		int   i;

		child_args = acl_vstring_alloc(256);
		for (i = 0; i < argc; i++) {
			if (i > 0)
				acl_vstring_strcat(child_args, " ");
			acl_vstring_strcat(child_args, "\"");
			acl_vstring_strcat(child_args, argv[i]);
			acl_vstring_strcat(child_args, "\"");
		}
		
	}

	client = proctl_client_open(progname);
	if (child_args)
		n = acl_vstream_fprintf(client, "%s|-d|STOP|-f|%s|-a|%s\r\n",
				progname, progchild, acl_vstring_str(child_args));
	else
		n = acl_vstream_fprintf(client, "%s|-d|STOP|-f|%s\r\n",
				progname, progchild);

	if (child_args != NULL)
		acl_vstring_free(child_args);

	if (n == ACL_VSTREAM_EOF)
		acl_msg_fatal("%s(%d): fprintf to acl_proctl error(%s)",
			myname, __LINE__, acl_last_strerror(ebuf, sizeof(ebuf)));

	while (1) {
		n = acl_vstream_gets_nonl(client, buf, sizeof(buf));
		if (n == ACL_VSTREAM_EOF)
			break;
		acl_msg_info("%s(%d): %s", myname, __LINE__, buf);
	}

	proctl_client_close(client);
}

void acl_proctl_stop_all(const char *progname)
{
	acl_proctl_stop_one(progname, "all", 0, NULL);
}

void acl_proctl_quit(const char *progname)
{
	const char *myname = "acl_proctl_quit";
	char  ebuf[256], buf[1024];
	ACL_VSTREAM *client;
	int   n;

	client = proctl_client_open(progname);
	n = acl_vstream_fprintf(client, "%s|-d|QUIT\r\n", progname);
	if (n == ACL_VSTREAM_EOF)
		acl_msg_fatal("%s(%d): fprintf to acl_proctl error(%s)",
			myname, __LINE__, acl_last_strerror(ebuf, sizeof(ebuf)));

	while (1) {
		n = acl_vstream_gets_nonl(client, buf, sizeof(buf));
		if (n == ACL_VSTREAM_EOF)
			break;
		acl_debug(ACL_DEBUG_PROCTL, 2) ("%s(%d): buf(%s)",
			myname, __LINE__, buf);
	}

	proctl_client_close(client);
}

void acl_proctl_list(const char *progname)
{
	const char *myname = "acl_proctl_list";
	char  ebuf[256], buf[1024];
	ACL_VSTREAM *client;
	int   n;

	client = proctl_client_open(progname);
	n = acl_vstream_fprintf(client, "%s|-d|LIST\r\n", progname);
	if (n == ACL_VSTREAM_EOF)
		acl_msg_fatal("%s(%d): fprintf to acl_proctl error(%s)",
			myname, __LINE__, acl_last_strerror(ebuf, sizeof(ebuf)));

	while (1) {
		if (acl_vstream_gets_nonl(client, buf, sizeof(buf)) == ACL_VSTREAM_EOF)
			break;
		acl_debug(ACL_DEBUG_PROCTL, 2) ("%s(%d): buf(%s)",
			myname, __LINE__, buf);
		printf("%s\r\n", buf);
	}

	proctl_client_close(client);
}

void acl_proctl_probe(const char *progname, const char *progchild)
{
	const char *myname = "acl_proctl_list";
	char  ebuf[256], buf[1024];
	ACL_VSTREAM *client;
	int   n;

	client = proctl_client_open(progname);
	n = acl_vstream_fprintf(client, "%s|-d|PROBE|-f|%s\r\n",
			progname, progchild);
	if (n == ACL_VSTREAM_EOF)
		acl_msg_fatal("%s(%d): fprintf to acl_proctl error(%s)",
		myname, __LINE__, acl_last_strerror(ebuf, sizeof(ebuf)));

	while (1) {
		if (acl_vstream_gets_nonl(client, buf, sizeof(buf)) == ACL_VSTREAM_EOF)
			break;
		acl_debug(ACL_DEBUG_PROCTL, 2) ("%s(%d): buf(%s)",
			myname, __LINE__, buf);
		printf("%s\r\n", buf);
	}

	proctl_client_close(client);
}

void acl_proctl_child(const char *progname, void (*onexit_fn)(void *), void *arg)
{
	acl_pthread_attr_t attr;
	acl_pthread_t tid;

	proctl_child_atexit(onexit_fn, arg);
	var_progname = acl_mystrdup(progname);
	acl_pthread_attr_init(&attr);
	(void) acl_pthread_attr_setdetachstate(&attr, 1);
	acl_pthread_create(&tid, &attr, proctl_child_thread, NULL);
}
#else
void acl_proctl_deamon_init(const char *progname acl_unused)
{
	const char *myname = "acl_proctl_deamon_init";

	acl_msg_fatal("%s(%d): not support!", myname, __LINE__);
}

void acl_proctl_daemon_loop()
{
	const char *myname = "acl_proctl_daemon_loop";

	acl_msg_fatal("%s(%d): not support!", myname, __LINE__);
}

int acl_proctl_deamon_start_one(const char *progchild acl_unused,
	int argc acl_unused, char *argv[] acl_unused)
{
	const char *myname = "acl_proctl_deamon_start_one";

	acl_msg_fatal("%s(%d): not support!", myname, __LINE__);

	return (-1);
}

void acl_proctl_start_one(const char *progname acl_unused,
	const char *progchild acl_unused,
	int argc acl_unused, char *argv[] acl_unused)
{
	const char *myname = "acl_proctl_start_one";

	acl_msg_fatal("%s(%d): not support!", myname, __LINE__);
}

void acl_proctl_stop_one(const char *progname acl_unused,
	const char *progchild acl_unused,
	int argc acl_unused, char *argv[] acl_unused)
{
	const char *myname = "acl_proctl_stop_one";

	acl_msg_fatal("%s(%d): not support!", myname, __LINE__);
}

void acl_proctl_stop_all(const char *progname acl_unused)
{
	const char *myname = "acl_proctl_stop_all";

	acl_msg_fatal("%s(%d): not support!", myname, __LINE__);
}

void acl_proctl_quit(const char *progname acl_unused)
{
	const char *myname = "acl_proctl_quit";

	acl_msg_fatal("%s(%d): not support!", myname, __LINE__);
}

void acl_proctl_child(const char *progname acl_unused,
	void (*onexit_fn)(void *) acl_unused, void *arg acl_unused)
{
	const char *myname = "acl_proctl_child";

	acl_msg_fatal("%s(%d): not support!", myname, __LINE__);
}
#endif  /* ACL_WINDOWS */

