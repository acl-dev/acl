#include "lib_acl.h"
#include "lib_protocol.h"

#include "global.h"
#include "gid.h"
#include "lib_gid.h"

/* 使用线程局部变量，但注意此库不得编译成动态库XXX */

static __thread ACL_VSTREAM *__client = NULL;

acl_int64 gid_next(const char *tag, int *errnum)
{
	acl_int64 gid = 0;
	int   err, nretry = 0;

	if (var_gid_server_addr[0] == 0) {
		if (errnum)
			*errnum = GID_ERR_INIT;
		return (-1);
	}

	while (1) {
		if (__client == NULL)
			__client = acl_vstream_connect(var_gid_server_addr,
					ACL_BLOCKING, var_gid_conn_timeout,
					var_gid_rw_timeout, 1024);
		if (__client == NULL) {
			if (errnum)
				*errnum = GID_ERR_CONN;
			return (-1);
		}

		if (var_gid_proto == GID_PROTO_JSON) {
			gid = gid_json_next(__client, tag, &err);
		} else if (var_gid_proto == GID_PROTO_XML) {
			gid = gid_xml_next(__client, tag, &err);
		} else {
			gid = gid_cmdline_next(__client, tag, &err);
		}

		if (gid >= 0) {
			if (errnum)
				*errnum = GID_OK;
			break;
		} else if (err != GID_ERR_IO) {
			if (errnum)
				*errnum = err;
			break;
		} else if (nretry++ >= var_gid_retry_limit) {
			if (errnum)
				*errnum = err;
			break;
		}
	}

	if (var_gid_keepalive == 0) {
		acl_vstream_close(__client);
		__client = NULL;
	}

	return (gid);
}

/*
#define GID_OK			200
#define GID_ERR_INIT		400
#define GID_ERR_CONN		401
#define GID_ERR_IO		402
#define GID_ERR_PROTO		403       
#define GID_ERR_SERVER		404

#define GID_ERR_SID		500
#define GID_ERR_OVERRIDE	501
#define GID_ERR_SAVE		502
*/

const char *gid_client_serror(int errnum)
{
	static const struct {
		int  err;
		const char *str;
	} errors[] = {
		{ GID_OK, "ok" },

		/* 客户端相关错误 */
		{ GID_ERR_INIT, "gid_client_init should called first" },
		{ GID_ERR_CONN, "connect server error" },
		{ GID_ERR_IO, "readwrite from server error" },
		{ GID_ERR_PROTO, "gid protocol error" },
		{ GID_ERR_SERVER, "gid server internal error" },

		/* 服务端返回的错误 */
		{ GID_ERR_SID, "sid invalid" },
		{ GID_ERR_OVERRIDE, "gid override" },
		{ GID_ERR_SAVE, "gid save error" },
		{ 0, 0 }
	};
	static const char *unknown = "unknown error number";
	int   i;

	for (i = 0; errors[i].str != NULL; i++) {
		if (errnum == errors[i].err)
			return (errors[i].str);
	}
	return (unknown);
}
