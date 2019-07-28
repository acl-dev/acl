#include "lib_acl.h"
#include <string.h>
#include "lib_tpl.h"
#include "service_var.h"
#include "notify.h"

/*
连接9010端口，之后发送xml格式为<send_sms phone="手机号码" message="短信内容" />
成功则返回<send_sms result="succeeded" />
失败则返回<send_sms result="failed" />

支持多个连接，每个连接可以连续发送，长连接可不断
122.49.0.234
*/

static int sms_send(ACL_VSTREAM *client, const char *phone, const char *proc,
	int pid, const char *info, const char *ip)
{
	ACL_VSTRING *buf = acl_vstring_alloc(256);
	ACL_XML *xml = acl_xml_alloc();
	char  res[1024];
	int   ret;

	acl_vstring_sprintf(buf, "<send_sms phone=\"%s\" message=\"proc:%s, pid:%d, ip:%s, info:%s\" />",
			phone, proc, pid, ip, info);
	if (acl_vstream_writen(client, acl_vstring_str(buf), ACL_VSTRING_LEN(buf))
		== ACL_VSTREAM_EOF)
	{
		acl_msg_error("write to sms server error, msg: %s",
			acl_vstring_str(buf));
		acl_vstring_free(buf);
		return (-1);
	}

	acl_msg_info(">>send: %s", acl_vstring_str(buf));
	acl_vstring_free(buf);

	while (1) {
		ret = acl_vstream_read(client, res, sizeof(res) - 1);
		if (ret == ACL_VSTREAM_EOF)
			return (-1);
		res[ret] = 0;
		acl_xml_parse(xml, res);
		if (acl_xml_is_complete(xml, "send_sms")) {
			acl_msg_info("send ok!(%s)", res);
			break;
		}
	}

	return (0);
}

int sms_notify(const char *proc, ACL_ARGV *rcpts,
	int pid, const char *info)
{
	ACL_ITER iter;
	ACL_VSTREAM *client;
	char  ip[64], *ptr;

	if (var_cfg_sms_addr == NULL || *var_cfg_sms_addr == 0) {
		acl_msg_info("%s(%d): var_cfg_sms_addr null",
			__FUNCTION__, __LINE__);
		return (0);
	}

	client = acl_vstream_connect(var_cfg_sms_addr, ACL_BLOCKING,
			30, 30, 4096);
	if (client == NULL) {
		acl_msg_error("%s(%d): connect %s error(%s)",
			__FUNCTION__, __LINE__,
			var_cfg_sms_addr, acl_last_serror());
		return (-1);
	}

	if (var_cfg_host_ip && *var_cfg_host_ip) {
		snprintf(ip, sizeof(ip), "%s", var_cfg_host_ip);
	} else if (acl_getsockname(ACL_VSTREAM_SOCK(client), ip, sizeof(ip)) < 0) {
		acl_msg_warn("%s(%d): get local ip error: %s",
			__FUNCTION__, __LINE__, acl_last_serror());
		snprintf(ip, sizeof(ip), "127.0.0.1");
	} else {
		ptr = strchr(ip, ':');
		if (ptr)
			*ptr = 0;
	}

	acl_foreach(iter, rcpts) {
		const char *phone = (const char*) iter.data;
		if (sms_send(client, phone, proc, pid, info, ip) < 0)
			break;
	}

	acl_vstream_close(client);
	return (0);
}
