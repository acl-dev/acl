#include "stdafx.h"

#include "master_params.h"
#include "master.h"

typedef struct WARN_INFO {
	char *notify_addr;
	char *notify_recipients;
	char *path;
	char *conf;
	char *ver;
	int   pid;
	char *desc;
} WARN_INFO;

static WARN_INFO *warn_info_new(const char *notify_addr, const char *recipients,
	const char *path, const char *conf, const char *ver, int pid, const char *desc)
{
	WARN_INFO *info = (WARN_INFO*) acl_mycalloc(1, sizeof(WARN_INFO));

	info->notify_addr = acl_mystrdup(notify_addr);
	info->notify_recipients = acl_mystrdup(recipients);
	info->path = acl_mystrdup(path);
	info->conf = acl_mystrdup(conf);
	if (ver && *ver)
		info->ver = acl_mystrdup(ver);
	else
		info->ver = acl_mystrdup("none");
	info->pid = pid;
	info->desc = acl_mystrdup(desc);
	return (info);
}

static void warn_info_free(WARN_INFO *info)
{
	acl_myfree(info->notify_addr);
	acl_myfree(info->notify_recipients);
	acl_myfree(info->path);
	acl_myfree(info->conf);
	acl_myfree(info->ver);
	acl_myfree(info->desc);
	acl_myfree(info);
}

static void add_str(ACL_VSTRING *buf, const char *name, const char *value)
{
	if (ACL_VSTRING_LEN(buf) > 0)
		acl_vstring_strcat(buf, "&");
	acl_vstring_strcat(buf, name);
	acl_vstring_strcat(buf, "=");

	if (value && *value) {
		char *tmp = acl_url_encode(value, NULL);
		acl_vstring_strcat(buf, tmp);
		acl_myfree(tmp);
	}
}

static void add_num(ACL_VSTRING *buf, const char *name, int value)
{
	char tmp[32];

	snprintf(tmp, sizeof(tmp), "%d", value);
	add_str(buf, name, tmp);
}

static void notify_thread(void *arg)
{
	const char *myname = "notify_thread";
	WARN_INFO *info = (WARN_INFO*) arg;
	ACL_VSTREAM *client;
	ACL_VSTRING *buf;
	int   ret;

	buf = acl_vstring_alloc(256);
	add_str(buf, "path", info->path);
	add_str(buf, "conf", info->conf);
	add_str(buf, "ver", info->ver);
	add_num(buf, "pid", info->pid);
	add_str(buf, "rcpt", info->notify_recipients);
	add_str(buf, "info", info->desc);
	acl_vstring_strcat(buf, "\r\n");

	client = acl_vstream_connect(info->notify_addr,
		ACL_BLOCKING, 60, 60, 1024);
	if (client == NULL) {
		acl_msg_error("%s(%d): connect %s error, info(%s)", myname,
			__LINE__, info->notify_addr, acl_vstring_str(buf));
		acl_vstring_free(buf);
		warn_info_free(info);
		return;
	}

	/* 禁止将该句柄传递给子进程 */
	acl_close_on_exec(ACL_VSTREAM_SOCK(client), ACL_CLOSE_ON_EXEC);

	//acl_msg_info(">>>>Notify=[%s]<<<<<", acl_vstring_str(buf));
	ret = acl_vstream_writen(client, acl_vstring_str(buf),
		ACL_VSTRING_LEN(buf));
	if (ret == ACL_VSTREAM_EOF)
		acl_msg_error("%s(%d): write to notify server error, info(%s)",
			myname, __LINE__, acl_vstring_str(buf));
	else
		acl_msg_info("%s(%d): notify %s ok!",
			myname, __LINE__, info->notify_addr);

	acl_vstream_close(client);
	acl_vstring_free(buf);
	warn_info_free(info);
}

void master_warning(const char *notify_addr, const char *recipients,
	const char *path, const char *conf, const char *ver,
	int pid, const char *desc)
{
	const char *myname = "master_warning";
	WARN_INFO *info;

	if (notify_addr == NULL || *notify_addr == 0) {
		acl_msg_warn("%s(%d): notify_addr invalid", myname, __LINE__);
		return;
	}
	if (path == NULL || *path == 0) {
		acl_msg_warn("%s(%d): path invalid", myname, __LINE__);
		return;
	}
	if (conf == NULL || *conf == 0) {
		acl_msg_warn("%s(%d): conf invalid", myname, __LINE__);
		return;
	}
	if (desc == NULL || *desc == 0) {
		acl_msg_warn("%s(%d): desc invalid", myname, __LINE__);
		return;
	}
	if (recipients == NULL || *recipients == 0)
		recipients = "admin@root.domain";

	info = warn_info_new(notify_addr, recipients, path, conf, ver, pid, desc);
	acl_pthread_pool_add(acl_var_master_thread_pool, notify_thread, info);
}
