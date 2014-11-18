
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "lib_acl.h"

#include "multi_proxy.h"

#define	VSTREAM_PROXY_BACKEND_FLAG	1
#define	VSTREAM_PROXY_FRONT_FLAG	2

typedef struct VSTREAM_PROXY_OBJ {
	ACL_VSTREAM *stream;
	struct VSTREAM_PROXY_OBJ *peer;
	int   flag;
} VSTREAM_PROXY_OBJ;

/* int values */
int var_proxy_log_level;
int var_proxy_tmout;
int var_proxy_connect_tmout;
int var_proxy_rw_tmout;
int var_proxy_bufsize;

/* bool values */
int var_proxy_debug_request;
int var_proxy_debug_respond;

/* char values */
char *var_proxy_banner;
char *var_proxy_backend_addr;
char *var_proxy_request_file;
char *var_proxy_respond_file;

static ACL_CONFIG_INT_TABLE __conf_int_tab[] = {
	{ VAR_PROXY_LOG_LEVEL, DEF_PROXY_LOG_LEVEL, &var_proxy_log_level, 0, 0 },
	{ VAR_PROXY_TMOUT, DEF_PROXY_TMOUT, &var_proxy_tmout, 0, 0 },
	{ VAR_PROXY_CONNECT_TMOUT, DEF_PROXY_CONNECT_TMOUT, &var_proxy_connect_tmout, 0, 0 },
	{ VAR_PROXY_RW_TMOUT, DEF_PROXY_RW_TMOUT, &var_proxy_rw_tmout, 0, 0 },
	{ VAR_PROXY_BUFSIZE, DEF_PROXY_BUFSIZE, &var_proxy_bufsize, 0, 0 },
	{ 0, 0, 0, 0, 0 },
};

static ACL_CONFIG_BOOL_TABLE __conf_bool_tab[] = {
	{ VAR_PROXY_DEBUG_REQUEST, DEF_PROXY_DEBUG_REQUEST, &var_proxy_debug_request },
	{ VAR_PROXY_DEBUG_RESPOND, DEF_PROXY_DEBUG_RESPOND, &var_proxy_debug_respond },
	{ 0, 0, 0 },
};

static ACL_CONFIG_STR_TABLE __conf_str_tab[] = {
	{ VAR_PROXY_BANNER, DEF_PROXY_BANNER, &var_proxy_banner },
	{ VAR_PROXY_BACKEND_ADDR, DEF_PROXY_BACKEND_ADDR, &var_proxy_backend_addr },
	{ VAR_PROXY_REQUEST_FILE, DEF_PROXY_REQUEST_FILE, &var_proxy_request_file },
	{ VAR_PROXY_RESPOND_FILE, DEF_PROXY_RESPOND_FILE, &var_proxy_respond_file },
	{ 0, 0, 0 },
};

static char *__data_buf;
static ACL_VSTREAM *__request_stream;
static ACL_VSTREAM *__respond_stream;

static void __pre_disconnect_fn(ACL_VSTREAM *stream, char *unused_name, char **unused_argv)
{
	char  myname[] = "__pre_disconnect_fn";
	VSTREAM_PROXY_OBJ *vpobj, *peer;

	unused_name = unused_name;
	unused_argv = unused_argv;

	if (stream == NULL) {
		acl_msg_error("%s(%d)->%s: stream null",
				__FILE__, __LINE__, myname);
		return;
	}

	vpobj = (VSTREAM_PROXY_OBJ *) stream->context;
	if (vpobj == NULL) {
		acl_msg_error("%s(%d)->%s: stream's context null",
				__FILE__, __LINE__, myname);
		return;
	}

	peer = vpobj->peer;
	if (peer == NULL) {
		;  /* do nothing */
	} else if (peer->stream == NULL) {
		acl_msg_error("%s(%d)->%s: peer's stream null",
				__FILE__, __LINE__, myname);
	} else {
		peer->peer  = NULL;
		acl_multi_server_disconnect(peer->stream);
	}

	acl_myfree(vpobj);
	stream->context = NULL;
}

static void __service(ACL_VSTREAM *stream, char *service, char **argv)
{
	char  myname[] = "__service";
	int   n, ret;
	VSTREAM_PROXY_OBJ *vpobj, *peer;

	/*
	 * Sanity check. This service takes no command-line arguments.
	 */
	if (argv[0])
		acl_msg_fatal("%s(%d)->%s: unexpected command-line argument: %s",
				__FILE__, __LINE__, myname, argv[0]);

	if (stream == NULL)
		acl_msg_fatal("%s(%d)->%s: stream null",
				__FILE__, __LINE__, myname);
	vpobj = (VSTREAM_PROXY_OBJ *) stream->context;
	if (vpobj == NULL)
		acl_msg_fatal("%s(%d)->%s: stream's context null",
				__FILE__, __LINE__, myname);

	if (acl_msg_verbose > 3)
		acl_msg_info("%s(%d)->%s: service name = %s, rw_timeout = %d",
			__FILE__, __LINE__, myname, service, stream->rw_timeout);

	acl_watchdog_pat();

	peer = vpobj->peer;
	if (peer == NULL || peer->stream == NULL || peer->peer != vpobj) {
		acl_multi_server_disconnect(stream);
		return;
	}

	n = acl_vstream_read(stream, __data_buf, var_proxy_bufsize);
	if (n == ACL_VSTREAM_EOF) {
		acl_multi_server_disconnect(stream);
		if (acl_msg_verbose > 3)
			acl_msg_info("%s(%d)->%s: read over",
				__FILE__, __LINE__, myname);
		return;
	}

	if (vpobj->flag == VSTREAM_PROXY_FRONT_FLAG && __request_stream) {
		ret = acl_vstream_writen(__request_stream, __data_buf, n);
		if (ret != n) {
			acl_msg_error("%s(%d)->%s: writen to %s, serr = %s",
					__FILE__, __LINE__, myname,
					var_proxy_request_file, strerror(errno));
			acl_vstream_close(__request_stream);
			__request_stream = NULL;
		}
	} else if (vpobj->flag == VSTREAM_PROXY_BACKEND_FLAG && __respond_stream) {
		ret = acl_vstream_writen(__respond_stream, __data_buf, n);
		if (ret != n) {
			acl_msg_error("%s(%d)->%s: writen to %s, serr = %s",
					__FILE__, __LINE__, myname,
					var_proxy_request_file, strerror(errno));
			acl_vstream_close(__respond_stream);
			__respond_stream = NULL;
		}
	}

	ret = acl_vstream_writen(peer->stream, __data_buf, n);
	if (ret != n) {
		acl_multi_server_disconnect(peer->stream);
		if (acl_msg_verbose > 3)
			acl_msg_info("%s(%d)->%s: write error = %s",
				__FILE__, __LINE__, myname,
				strerror(errno));
		return;
	}
}

static int __on_accept_fn(ACL_VSTREAM *front_stream)
{
	char  myname[] = "__on_accept_fn";
	ACL_VSTREAM *backend_stream;
	VSTREAM_PROXY_OBJ *backend_obj;
	VSTREAM_PROXY_OBJ *front_obj;

	backend_stream = acl_vstream_connect(var_proxy_backend_addr,
					ACL_BLOCKING,
					var_proxy_connect_tmout,
					var_proxy_rw_tmout,
					var_proxy_bufsize);
	if (backend_stream == NULL) {
		acl_msg_error("%s(%d)->%s: connect backend_addr = %s, serr = %s",
				__FILE__, __LINE__, myname,
				var_proxy_backend_addr, strerror(errno));
		return (-1);
	}

	backend_obj = acl_mymalloc(sizeof(VSTREAM_PROXY_OBJ));
	front_obj   = acl_mymalloc(sizeof(VSTREAM_PROXY_OBJ));

	backend_obj->stream     = backend_stream;
	backend_obj->flag       = VSTREAM_PROXY_BACKEND_FLAG;
	backend_obj->peer       = front_obj;
	backend_stream->context = (void *) backend_obj;

	front_obj->stream       = front_stream;
	front_obj->flag         = VSTREAM_PROXY_FRONT_FLAG;
	front_obj->peer         = backend_obj;
	front_stream->context   = (void *) front_obj;

	acl_multi_server_enable_read(backend_stream);

	return (0);
}

static void __pre_jail_init_fn(char *unused_name, char **unused_argv)
{
	char  myname[] = "__pre_jail_init_fn";

	unused_name = unused_name;
	unused_argv = unused_argv;

	if (acl_msg_verbose)
		acl_msg_info("%s(%d)->%s: test only",
				__FILE__, __LINE__, myname);
}

static void __post_jail_init_fn(char *unused_name, char **unused_argv)
{
	char  myname[] = "__post_jail_init_fn";
	ACL_VSTRING *why = acl_vstring_alloc(100);

	unused_name = unused_name;
	unused_argv = unused_argv;

	if (acl_msg_verbose)
		acl_msg_info("%s(%d)->%s: test only",
				__FILE__, __LINE__, myname);

	__data_buf = acl_mymalloc(var_proxy_bufsize);
	if (__data_buf == NULL)
		acl_msg_fatal("%s(%d)->%s: malloc data_buf, serr = %s",
				__FILE__, __LINE__, myname,
				strerror(errno));

	if (var_proxy_debug_request) {
		__request_stream = acl_safe_open(var_proxy_request_file,
						O_CREAT | O_RDWR | O_APPEND, 0600,
						(struct stat *) 0, (uid_t)-1,
						(uid_t )-1, why);
		if (__request_stream == NULL)
			acl_msg_fatal("%s(%d)->%s: can't open %s, err = %s",
					__FILE__, __LINE__, myname,
					var_proxy_request_file, acl_vstring_str(why));
	}

	if (var_proxy_debug_respond) {
		__respond_stream = acl_safe_open(var_proxy_respond_file,
						O_CREAT | O_RDWR | O_APPEND, 0600,
						(struct stat *) 0, (uid_t)-1,
						(uid_t )-1, why);
		if (__respond_stream == NULL)
			acl_msg_fatal("%s(%d)->%s: can't open %s, err = %s",
					__FILE__, __LINE__, myname,
					var_proxy_respond_file, acl_vstring_str(why));
	}

	acl_vstring_free(why);
}

int main(int argc, char *argv[])
{
	acl_msg_verbose = 0;

	acl_msg_info("%s: start...", __FILE__);
	printf("%s: starting...", __FILE__);

	acl_multi_server_main(argc, argv, __service,
				ACL_MASTER_SERVER_INT_TABLE, __conf_int_tab,
				ACL_MASTER_SERVER_BOOL_TABLE, __conf_bool_tab,
				ACL_MASTER_SERVER_STR_TABLE, __conf_str_tab,
				ACL_MASTER_SERVER_PRE_INIT, __pre_jail_init_fn,
				ACL_MASTER_SERVER_POST_INIT, __post_jail_init_fn,
				ACL_MASTER_SERVER_ON_ACCEPT, __on_accept_fn,
				ACL_MASTER_SERVER_ON_CLOSE, __pre_disconnect_fn,
				0);
	exit (0);
}

