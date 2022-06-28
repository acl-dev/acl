#include "lib_acl.h"

#include "global.h"
#include "lib_gid.h"
#include "gid.h"

acl_int64 gid_cmdline_get(int fd, const char *tag, int *errnum)
{
	acl_int64  gid;
	ACL_VSTREAM *client = acl_vstream_fdopen(fd, 0, 1024,
			var_gid_rw_timeout, ACL_VSTREAM_TYPE_SOCK);

	gid = gid_cmdline_next(client, tag, errnum);
	acl_vstream_free(client);
	return (gid);
}

acl_int64 gid_cmdline_next(ACL_VSTREAM *client, const char *tag, int *errnum)
{
	char  buf[1204];
	ACL_ARGV *tokens;
	ACL_ITER iter;
	const char *status = NULL, *gid = NULL, *tag_ptr = NULL, *msg = NULL, *err = NULL;

	if (tag && *tag)
		snprintf(buf, sizeof(buf), "CMD^%s|TAG^%s\r\n", GID_CMD_NEXT, tag);
	else
		snprintf(buf, sizeof(buf), "CMD^%s\r\n", GID_CMD_NEXT);

	if (acl_vstream_writen(client, buf, strlen(buf)) == ACL_VSTREAM_EOF) {
		if (errnum)
			*errnum = GID_ERR_IO;
		return (-1);
	} else if (acl_vstream_gets_nonl(client, buf, sizeof(buf)) == ACL_VSTREAM_EOF)
	{
		if (errnum)
			*errnum = GID_ERR_IO;
		return (-1);
	}

	tokens = acl_argv_split(buf, "|");
	acl_foreach(iter, tokens) {
		const char *ptr = (const char*) iter.data;

		if (strncasecmp(ptr, "STATUS^", sizeof("STATUS^") - 1) == 0) {
			status = ptr + sizeof("STATUS^") - 1;
		} else if (strncasecmp(ptr, "GID^", sizeof("GID^") - 1) == 0) {
			gid = ptr + sizeof("GID^") - 1;
		} else if (strncasecmp(ptr, "TAG^", sizeof("TAG^") - 1) == 0) {
			tag_ptr = ptr + sizeof("TAG^") - 1;
		} else if (strncasecmp(ptr, "MSG^", sizeof("MSG^") - 1) == 0) {
			msg = ptr + sizeof("MSG^") - 1;
		} else if (strncasecmp(ptr, "ERR^", sizeof("ERR^") - 1) == 0) {
			err = ptr + sizeof("ERR^");
		}
	}

	if (status == NULL) {
		if (errnum)
			*errnum = GID_ERR_PROTO;
		acl_argv_free(tokens);
		return (-1);
	} else if (strcasecmp(status, "OK") != 0) {
		if (errnum) {
			if (err)
				*errnum = atoi(err);
			else
				*errnum = GID_ERR_SERVER;
		}
		acl_argv_free(tokens);
		return (-1);
	} else if (gid == NULL) {
		if (errnum)
			*errnum = GID_ERR_PROTO;
		acl_argv_free(tokens);
		return (-1);
	} else {
		acl_int64 ngid = atoll(gid);
		acl_argv_free(tokens);
		return (ngid);
	}
}
