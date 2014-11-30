#include <stdio.h>
#include <stdlib.h>

#include "lib_acl.h"

#include "service_main.h"

typedef struct {
	ACL_ASTREAM *client;
	ACL_ASTREAM *server;
} STREAM_PIPE;

/* configure info */

/* TODO: you can add configure items here */

static int   var_cfg_debug_mem;
static int   var_cfg_read_line;

ACL_CONFIG_BOOL_TABLE service_conf_bool_tab[] = {
        /* TODO: you can add configure variables of int type here */
        { "debug_mem", 0, &var_cfg_debug_mem },
	{ "read_line", 1, &var_cfg_read_line },

        { 0, 0, 0 },
};

static int   var_cfg_io_idle_limit;
static int   var_cfg_debug_mem;

ACL_CONFIG_INT_TABLE service_conf_int_tab[] = {
	/* TODO: you can add configure variables of int type here */

	{ "io_idle_limit", 60, &var_cfg_io_idle_limit, 0, 0 },

	{ 0, 0, 0, 0, 0 },
};

static char *var_cfg_backend_addr;
static char *var_cfg_request_file;
static char *var_cfg_respond_file;

ACL_CONFIG_STR_TABLE service_conf_str_tab[] = {

	/* TODO: you can add configure variables of (char *) type here */
	/* example: { "mysql_dbaddr", "127.0.0.1:3306", &var_cfg_mysql_dbaddr }, */

	{ "backend_addr", "127.0.0.1:3306", &var_cfg_backend_addr },
	{ "request_file", "", &var_cfg_request_file },
	{ "respond_file", "", &var_cfg_respond_file },

	{ 0, 0, 0 },
};

static ACL_FILE *request_fp, *respond_fp;

void service_init(void *init_ctx acl_unused)
{
	const char *myname = "service_init";

	if (var_cfg_request_file && *var_cfg_request_file) {
		request_fp = acl_fopen(var_cfg_request_file, "a+");
		if (request_fp == NULL)
			acl_msg_error("%s(%d): open %s error(%s)",
				myname, __LINE__, var_cfg_request_file,
				acl_last_serror());
	}

	if (var_cfg_respond_file && *var_cfg_respond_file) {
		respond_fp = acl_fopen(var_cfg_respond_file, "a+");
		if (respond_fp == NULL)
			acl_msg_error("%s(%d): open %s error(%s)",
				myname, __LINE__, var_cfg_respond_file,
				acl_last_serror());
	}
}

void service_exit(void *exist_ctx acl_unused)
{
	if (request_fp)
		acl_fclose(request_fp);
	if (respond_fp)
		acl_fclose(respond_fp);
}

static int close_callback(ACL_ASTREAM *stream, void *ctx)
{
	STREAM_PIPE *sp = (STREAM_PIPE*) ctx;

	if (sp->client == stream) {
		if (sp->server)
			acl_aio_iocp_close(sp->server);
		sp->client = NULL;
	} else if (sp->server == stream) {
		if (sp->client)
			acl_aio_iocp_close(sp->client);
		sp->server = NULL;
	}
	if (sp->client == NULL && sp->server == NULL)
		acl_myfree(sp);
	return (0);
}

static int read_callback(ACL_ASTREAM *stream, void *ctx, char *data, int dlen)
{
	const char *myname = "read_callback";
	STREAM_PIPE *sp = (STREAM_PIPE*) ctx;

	if (sp->client == stream) {
		if (request_fp) {
			if (var_cfg_read_line) {
				if (acl_fprintf(request_fp, ">>>%s", data) == EOF) {
					acl_msg_error("%s(%d): write to %s error(%s)",
						myname, __LINE__, var_cfg_request_file,
						acl_last_serror());
					acl_fclose(request_fp);
					request_fp = NULL;
				}
			} else {
				if (acl_fwrite(data, dlen, 1, request_fp) == (size_t) EOF) {
					acl_msg_error("%s(%d): write to %s error(%s)",
						myname, __LINE__, var_cfg_request_file,
						acl_last_serror());
					acl_fclose(request_fp);
					request_fp = NULL;
				}
			}
		}
		if (sp->server == NULL)
			return (-1);
		acl_aio_writen(sp->server, data, dlen);
	} else if (sp->server == stream) {
		if (respond_fp) {
			if (var_cfg_read_line) {
				if (acl_fprintf(respond_fp, "<<<%s", data) == EOF) {
					acl_msg_error("%s(%d): write to %s error(%s)",
						myname, __LINE__, var_cfg_respond_file,
						acl_last_serror());
					acl_fclose(respond_fp);
					respond_fp = NULL;
				}
			} else {
				if (acl_fwrite(data, dlen, 1, respond_fp) == (size_t) EOF) {
					acl_msg_error("%s(%d): write to %s error(%s)",
						myname, __LINE__, var_cfg_respond_file,
						acl_last_serror());
					acl_fclose(respond_fp);
					respond_fp = NULL;
				}
			}
		}
		if (sp->client == NULL)
			return (-1);
		acl_aio_writen(sp->client, data, dlen);
	} else
		return (-1);

	return (0);
}

static int connect_callback(ACL_ASTREAM *stream, void *ctx)
{
	const char *myname = "connect_callback";
	STREAM_PIPE *sp = (STREAM_PIPE*) ctx;

	if (sp->server != stream)
		acl_msg_fatal("%s(%d): sp->server != stream", myname, __LINE__);

	if (var_cfg_read_line)
		acl_aio_gets(sp->server);
	else
		acl_aio_read(sp->server);
	if (sp->client == NULL)
		return (-1);
	if (var_cfg_read_line)
		acl_aio_gets(sp->client);
	else
		acl_aio_read(sp->client);
	return (0);
}

void service_main(ACL_ASTREAM *astream, void *run_ctx acl_unused)
{
	const char *myname = "service_main";
	STREAM_PIPE *sp = (STREAM_PIPE*) acl_mycalloc(1, sizeof(STREAM_PIPE));

	acl_msg_info("%s: begin connect %s, client: %s", myname,
		var_cfg_backend_addr, ACL_VSTREAM_PEER(acl_aio_vstream(astream)));

	sp->client = astream;
	sp->server = acl_aio_connect(acl_aio_handle(astream),
			var_cfg_backend_addr, 10);
	if (sp->server == NULL) {
		acl_msg_error("%s(%d): connect %s error(%s)", myname,
			__LINE__, var_cfg_backend_addr, acl_last_serror());
		acl_aio_iocp_close(astream);
		acl_myfree(sp);
		return;
	}

	acl_aio_ctl(sp->server, ACL_AIO_CTL_CONNECT_HOOK_ADD, connect_callback, sp,
			ACL_AIO_CTL_CLOSE_HOOK_ADD, close_callback, sp,
			ACL_AIO_CTL_READ_HOOK_ADD, read_callback, sp,
			ACL_AIO_CTL_TIMEOUT, 300,
			ACL_AIO_CTL_END);
	acl_aio_ctl(sp->client, ACL_AIO_CTL_CLOSE_HOOK_ADD, close_callback, sp,
			ACL_AIO_CTL_READ_HOOK_ADD, read_callback, sp,
			ACL_AIO_CTL_TIMEOUT, 300,
			ACL_AIO_CTL_END);
}
