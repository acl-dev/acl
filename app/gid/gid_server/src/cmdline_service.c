#include "lib_acl.h"
#include <string.h>
#include "gid_oper.h"
#include "global.h"
#include "cmdline_service.h"

static int send_respond_gid(ACL_VSTREAM *stream,
	const char *tag, acl_int64 gid)
{
#if 0
	if (acl_vstream_fprintf(stream, "STATUS^OK|GID^%lld|TAG^%s\r\n",
		gid, tag) == ACL_VSTREAM_EOF)
#else
	char  buf[1024];

	snprintf(buf, sizeof(buf), "STATUS^OK|GID^%lld|TAG^%s\r\n", gid, tag);
	if (acl_vstream_writen(stream, buf, strlen(buf)) == ACL_VSTREAM_EOF)
#endif
	{
		acl_msg_info("%s(%d): respond to client error",
			__FILE__, __LINE__);
		return (-1);
	}
	else
		return (1);  /* 返回1表示保持长连接 */
}

static int send_respond_error(ACL_VSTREAM *stream,
	const char *tag, const char *msg)
{
	if (acl_vstream_fprintf(stream, "STATUS^ERR|TAG^%s|MSG^%s\r\n",
		tag, msg) == ACL_VSTREAM_EOF)
	{
		acl_msg_info("%s(%d): respond to client error",
			__FILE__, __LINE__);
		return (-1);
	}
	else
		return (0);
}

static int proto_new_gid(ACL_VSTREAM *stream, const char *tag)
{
	acl_int64 gid;
	int   errnum = 0;

	gid = gid_next(var_cfg_gid_path, tag, var_cfg_gid_step, &errnum);
	if (gid >= 0)
		return (send_respond_gid(stream, tag, gid));
	else
		return (send_respond_error(stream, tag, gid_serror(errnum)));
}

static int proto_test_gid(ACL_VSTREAM *stream,
	const char *tag, acl_int64 test_id)
{
	return (send_respond_gid(stream, tag, test_id));
}

static int proto_get_test_gid(ACL_VSTREAM *stream, const char *tag)
{
	return (proto_test_gid(stream, tag, (acl_int64) var_cfg_gid_test));
}

/*--------------------------------------------------------------------------*/

typedef struct PROTO_CMDLINE {
	const char *cmd;  /* 命令字 */
	int (*handle)(ACL_VSTREAM *, const char*);  /* 协议处理函数句柄 */
} PROTO_CMDLINE;

/* 协议命令处理函数映射表 */
static PROTO_CMDLINE __proto_cmdline_tab[] = {
	{ CMD_NEW_GID, proto_new_gid },
	{ CMD_TEST_GID, proto_get_test_gid },
	{ NULL, NULL },
};

/* 解析数据头 */

/* 协议格式:
 * 请求格式: CMD^xxx|tag^xxx:sid\r\n
 * 响应格式: STATUS^[OK|ERR]|[GID^xxx|INFO^xxx]|tag^%s\r\n
 */
int cmdline_service(ACL_VSTREAM *client)
{       
	const char *myname = "parse_req_hdr";
	const char *cmd = NULL, *tag = "default";
	char  buf[1024];
	ACL_ARGV *argv;
	int   i, ret;
	ACL_ITER iter;

	/* 先读取数据头 */
	ret = acl_vstream_gets_nonl(client, buf, sizeof(buf) - 1);
	if (ret == ACL_VSTREAM_EOF)
		return (-1);

	argv = acl_argv_split(buf, "|");
	acl_foreach(iter, argv) {
		const char *ptr = (const char*) iter.data;
		if (strncasecmp(ptr, "CMD^", sizeof("CMD^") - 1) == 0)
			cmd = ptr + sizeof("CMD^") - 1;
		else if (strncasecmp(ptr, "TAG^", sizeof("TAG^") - 1) == 0)
			tag = ptr + sizeof("TAG^") - 1;
	}

	if (cmd == NULL || *tag == 0) {
		acl_msg_error("%s(%d): invalid request(%s)",
			myname, __LINE__, buf);
		acl_argv_free(argv);
		return (-1);
	}
	
	for (i = 0; __proto_cmdline_tab[i].cmd != NULL; i++) {
		if (strcasecmp(cmd, __proto_cmdline_tab[i].cmd) == 0) {
			ret = __proto_cmdline_tab[i].handle(client, tag);
			acl_argv_free(argv);
			return (ret);
		}
	}                       

	acl_msg_error("%s(%d): invalid request cmd(%s), no cmd found.",
		myname, __LINE__, cmd);

	acl_argv_free(argv);
	return (-1);
}
