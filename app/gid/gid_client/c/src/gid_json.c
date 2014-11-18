#include "lib_acl.h"

#include "global.h"
#include "http_client.h"
#include "lib_gid.h"
#include "gid.h"

acl_int64 gid_json_get(int fd, const char *tag, int *errnum)
{
	ACL_VSTREAM *client = acl_vstream_fdopen(fd, 0, 1024,
			var_gid_rw_timeout, ACL_VSTREAM_TYPE_SOCK);
	acl_int64  gid;

	gid = gid_json_next(client, tag, errnum);
	acl_vstream_free(client);
	return (gid);
}

acl_int64 gid_json_next(ACL_VSTREAM *client, const char *tag, int *errnum)
{
	char  buf[1204];
	ACL_ITER iter;
	ACL_JSON *json;
	const char *status = NULL, *gid = NULL, *tag_ptr = NULL, *msg = NULL, *err = NULL;

	if (tag && *tag)
		snprintf(buf, sizeof(buf), "{ cmd: '%s', tag: '%s' }\r\n",
			GID_CMD_NEXT, tag);
	else
		snprintf(buf, sizeof(buf), "{ cmd: '%s' }\r\n", GID_CMD_NEXT);

	/* 发送 HTTP JSON 请求 */
	if (http_client_post_request(client, var_gid_url, 1,
		"json", buf, (int) strlen(buf), errnum) < 0)
	{
		if (errnum)
			*errnum = GID_ERR_IO;
		return (-1);
	}

	json = acl_json_alloc();  /* 分配 JSON 对象 */

	/* 接收 HTTP JSON 响应 */
	if (http_client_get_respond(client, json, NULL, errnum, NULL) < 0)
	{
		if (errnum)
			*errnum = GID_ERR_IO;
		acl_json_free(json);
		return (-1);
	}

#define	STR	acl_vstring_str

	/* 数据格式: { status: 'ok|error', gid: xxx, tag: 'xxx', msg: 'xxx', err: 'xxx' } */

	acl_foreach(iter, json) {
		ACL_JSON_NODE *node = (ACL_JSON_NODE*) iter.data;

		if (STR(node->ltag) == 0 || STR(node->text) == 0)
			continue;
		if (strcasecmp(STR(node->ltag), "STATUS") == 0) {
			status = STR(node->text);
		} else if (strcasecmp(STR(node->ltag), "GID") == 0) {
			gid = STR(node->text);
		} else if (strcasecmp(STR(node->ltag), "TAG") == 0) {
			tag_ptr = STR(node->text);
		} else if (strcasecmp(STR(node->ltag), "MSG") == 0) {
			msg = STR(node->text);
		} else if (strcasecmp(STR(node->ltag), "ERR") == 0) {
			err = STR(node->text);
		}
	}

	if (status == NULL) {
		if (errnum)
			*errnum = GID_ERR_PROTO;
		acl_json_free(json);
		return (-1);
	} else if (strcasecmp(status, "OK") != 0) {
		if (errnum) {
			if (err)
				*errnum = atoi(err);
			else
				*errnum = GID_ERR_SERVER;
		}
		acl_json_free(json);
		return (-1);
	} else if (gid == NULL) {
		if (errnum)
			*errnum = GID_ERR_PROTO;
		acl_json_free(json);
		return (-1);
	} else {
		acl_int64 ngid = atoll(gid);
		acl_json_free(json);
		return (ngid);
	}
}
