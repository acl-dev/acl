#include "lib_acl.h"

#include "global.h"
#include "http_client.h"
#include "lib_gid.h"
#include "gid.h"

acl_int64 gid_xml_get(int fd, const char *tag, int *errnum)
{
	ACL_VSTREAM *client = acl_vstream_fdopen(fd, 0, 1024,
			var_gid_rw_timeout, ACL_VSTREAM_TYPE_SOCK);
	acl_int64  gid;

	gid = gid_xml_next(client, tag, errnum);
	acl_vstream_free(client);
	return (gid);
}

acl_int64 gid_xml_next(ACL_VSTREAM *client, const char *tag, int *errnum)
{
	char  buf[1204];
	ACL_ITER iter;
	ACL_XML *xml;
	const char *status = NULL, *gid = NULL, *tag_ptr = NULL, *msg = NULL, *err = NULL;
	static __thread ACL_VSTRING *tt = NULL;

	if (tag && *tag)
		snprintf(buf, sizeof(buf), "<request cmd='%s' tag='%s' />\r\n",
			GID_CMD_NEXT, tag);
	else
		snprintf(buf, sizeof(buf), "<request cmd='%s' />\r\n", GID_CMD_NEXT);

	/* 发送 HTTP JSON 请求 */
	if (http_client_post_request(client, var_gid_url, 1,
		"xml", buf, (int) strlen(buf), errnum) < 0)
	{
		if (errnum)
			*errnum = GID_ERR_IO;
		return (-1);
	}

	xml = acl_xml_alloc();  /* 分配 JSON 对象 */

	if (tt == NULL)
		tt = acl_vstring_alloc(100);
	else
		ACL_VSTRING_RESET(tt);

	/* 接收 HTTP JSON 响应 */
	if (http_client_get_respond(client, NULL, xml, errnum, tt) < 0)
	{
		if (errnum)
			*errnum = GID_ERR_IO;
		acl_xml_free(xml);
		return (-1);
	}

#define	STR	acl_vstring_str

	/*
	{
		ACL_VSTRING *tmp = acl_vstring_alloc(128);
		acl_xml_dump2(xml, tmp);
		printf("xml: %s\r\n", STR(tmp));
		acl_vstring_free(tmp);
	}
	*/

	/* 数据格式: <respond status='ok|error' gid=xxx tag='xxx' msg='xxx' err='xxx' /> */

	acl_foreach(iter, xml) {
		ACL_XML_NODE *node = (ACL_XML_NODE*) iter.data;

		/* 找到 respond 结点 */
		if (strcasecmp(STR(node->ltag), "respond") == 0
			&& node->attr_list != NULL)
		{
			ACL_ITER attr_iter;

			/* 遍历结点属性 */
			acl_foreach(attr_iter, node->attr_list) {
				ACL_XML_ATTR *attr = (ACL_XML_ATTR*) attr_iter.data;

				if (strcasecmp(STR(attr->name), "STATUS") == 0) {
					status = STR(attr->value);
				} else if (strcasecmp(STR(attr->name), "GID") == 0) {
					gid = STR(attr->value);
				} else if (strcasecmp(STR(attr->name), "TAG") == 0) {
					tag_ptr = STR(attr->value);
				} else if (strcasecmp(STR(attr->name), "MSG") == 0) {
					msg = STR(attr->value);
				} else if (strcasecmp(STR(attr->name), "ERR") == 0) {
					err = STR(attr->value);
				}
			}
		}
	}

	if (status == NULL) {
		if (errnum)
			*errnum = GID_ERR_PROTO;
		acl_xml_free(xml);
		return (-1);
	} else if (strcasecmp(status, "OK") != 0) {
		if (errnum) {
			if (err)
				*errnum = atoi(err);
			else
				*errnum = GID_ERR_SERVER;
		}
		acl_xml_free(xml);
		return (-1);
	} else if (gid == NULL) {
		if (errnum)
			*errnum = GID_ERR_PROTO;
		acl_xml_free(xml);
		return (-1);
	} else {
		acl_int64 ngid = atoll(gid);
		acl_xml_free(xml);
		return (ngid);
	}
}
