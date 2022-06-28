#include "lib_acl.h"
#include "notify.h"
#include "service_var.h"
#include "service_main.h"

static ACL_CACHE *var_smtp_notify_cache;
static ACL_CACHE *var_sms_notify_cache;

static void cache_free(const ACL_CACHE_INFO *info acl_unused, void *ctx)
{
	acl_myfree(ctx);
}

/* 初始化函数 */
void service_init(void *init_ctx acl_unused)
{
	const char *myname = "service_init";
	ACL_IFCONF *ifconf;
	ACL_IFADDR *ifaddr;
	ACL_ITER iter;

	service_var_init();
	var_smtp_notify_cache = acl_cache_create(10000, var_cfg_smtp_notify_cache_timeout,
			cache_free);
	var_sms_notify_cache = acl_cache_create(10000, var_cfg_sms_notify_cache_timeout,
			cache_free);
	acl_msg_info("%s: init ok ...", myname);
	if (var_cfg_host_ip && *var_cfg_host_ip)
		return;

	if (var_cfg_host_ip)
		acl_myfree(var_cfg_host_ip);
	var_cfg_host_ip = NULL;

	ifconf = acl_get_ifaddrs();

	acl_foreach(iter, ifconf) {
		ifaddr = (ACL_IFADDR*) iter.data;

		if (strcmp(ifaddr->ip, "127.0.0.1") == 0)
			continue;

		acl_msg_info(">>>ip: %s", ifaddr->ip);
		/* 外网IP优先 */
		if (strncmp(ifaddr->ip, "10.", 3) != 0
			&& strncmp(ifaddr->ip, "192.", 4) != 0)
		{
			if (var_cfg_host_ip)
				acl_myfree(var_cfg_host_ip);
			var_cfg_host_ip = acl_mystrdup(ifaddr->ip);
		} else if (var_cfg_host_ip == NULL) {
			var_cfg_host_ip = acl_mystrdup(ifaddr->ip);
		}
	}
}

/* 进程退出前调用的函数 */
void service_exit(void *arg acl_unused)
{
	const char *myname = "service_exit";

	acl_cache_free(var_smtp_notify_cache);
	acl_cache_free(var_sms_notify_cache);

	service_var_end();

	acl_msg_info("%s: exit now ...", myname);
}

/* 协议处理函数入口 */
int service_main(void *run_ctx acl_unused, ACL_VSTREAM *client)
{
#if 0
	int   ret, ready;
	ACL_VSTRING *buf = acl_vstring_alloc(1024);

	while (1) {
		ready = 0;
		ACL_VSTRING_RESET(buf);
		ret = acl_vstream_gets_peek(client, buf, &ready);
		if (ret == ACL_VSTREAM_EOF) {
			acl_vstring_free(buf);
			return (-1);  /* 返回负值以使框架内部关闭 client 数据流 */
		} else if (!ready)
			break;
		if (notify(acl_vstring_str(buf)) < 0) {
			acl_vstring_free(buf);
			return (-1);
		}
	}
	acl_vstring_free(buf);
	return (0);
#else
	int   ret;
	char  line[1024];

	ret = acl_vstream_gets_nonl(client, line, sizeof(line));
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): gets error(%s)",
			__FUNCTION__, __LINE__, acl_last_serror());
		return (-1);
	}

	if (notify(var_smtp_notify_cache, var_sms_notify_cache, line) < 0)
		return (-1);

	return (0);
#endif
}
