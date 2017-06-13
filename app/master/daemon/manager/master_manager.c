/* System libraries. */
#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#endif

#ifdef ACL_UNIX

#include <unistd.h>
#include <errno.h>
#include <string.h>

/* Utility library. */

#include "stdlib/acl_msg.h"
#include "stdlib/acl_iostuff.h"
#include "event/acl_events.h"

/* Application-specific. */

#include "../../master_proto.h"
#include "../../master_params.h"
#include "../master.h"

static ACL_AIO *acl_var_master_manager_aio = NULL;
static ACL_ASTREAM *acl_var_master_manager_listener = NULL;

static MASTER_CONN *master_conn_create(ACL_ASTREAM *conn)
{
	MASTER_CONN *mc = (MASTER_CONN *) acl_mycalloc(1, sizeof(MASTER_CONN));
	mc->conn = conn;
	return mc;
}

static void master_conn_free(MASTER_CONN *conn)
{
	acl_myfree(conn);
}

static int http_head_line(MASTER_CONN *mc, const char *data, int dlen)
{
}

static int read_callback(ACL_ASTREAM *conn, void *ctx,
	const char *data, int dlen)
{
	MASTER_CONN *mc = (MASTER_CONN *) ctx;

	switch (mc->status) {
	case HTTP_S_HEAD_LINE:
		return http_head_line(mc, data, dlen);
	case HTTP_S_HEAD_ENTRY:
		break;
	case HTTP_S_BODY:
		break;
	default:
		break;
	}
}

static int close_callback(ACL_ASTREAM *conn acl_unused, void *ctx)
{
	MASTER_CONN *mc = (MASTER_CONN *) ctx;

	master_conn_free(mc);
	return -1;
}

static int timeout_callback(ACL_ASTREAM *conn acl_unused, void *ctx acl_unused)
{
	return -1;
}

static int accept_callback(ACL_ASTREAM *conn, void *ctx acl_unused)
{
	MASTER_CONN *mc = master_conn_create(conn);

	mc->status = HTTP_S_HEAD_LINE;

	acl_aio_ctl(conn, ACL_AIO_CTL_READ_HOOK_ADD, read_callback, mc,
		ACL_AIO_CTL_CLOSE_HOOK_ADD, close_callback, mc,
		ACL_AIO_CTL_TIMEO_HOOK_ADD, timeout_callback, mc,
		ACL_AIO_CTL_TIMEOUT, 60,
		ACL_AIO_CTL_LINE_LENGTH, 8192,
		ACL_AIO_CTL_END);

	acl_aio_gets(conn);
	return 0;
}

void acl_master_manager_init(void)
{
	ACL_VSTREAM *sstream  =
		acl_vstream_listen(acl_var_master_manager_addr, 128);

	if (sstream == NULL) {
		acl_msg_error("%s(%d), %s: listen %s error %s"
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
		return;
	}

	if (acl_var_master_global_event == NULL)
		acl_msg_fatal("%s(%d), %s: global event null",
			__FILE__, __LINE__, __FUNCTION__);
	if (acl_var_master_manager_aio == NULL)
		acl_var_master_manager_aio =
			acl_aio_create3(acl_var_master_global_event);
	acl_var_master_manager_listener = acl_aio_open(
			acl_var_master_manager_aio, sstream);

	acl_aio_ctl(acl_var_master_manager_listener,
		ACL_AIO_CTL_ACCEPT_FN, accept_callback,
		ACL_AIO_CTL_END);
	acl_aio_accept(acl_var_master_manager_listener);
}

#endif /* ACL_UNIX */
