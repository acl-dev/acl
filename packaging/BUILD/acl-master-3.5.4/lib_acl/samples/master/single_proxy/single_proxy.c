
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "lib_acl.h"

#include "single_proxy.h"

int var_proxy_log_level;
int var_proxy_tmout;
int var_proxy_connect_tmout;
int var_proxy_rw_tmout;
int var_proxy_bufsize;
int var_proxy_retries;

char *var_proxy_banner;
char *var_proxy_backend_addr;
char *var_proxy_in_file;
char *var_proxy_out_file;
char *var_proxy_host_allow;

static ACL_CONFIG_INT_TABLE __conf_int_tab[] = {
	{ VAR_PROXY_LOG_LEVEL, DEF_PROXY_LOG_LEVEL, &var_proxy_log_level, 0, 0 },
	{ VAR_PROXY_TMOUT, DEF_PROXY_TMOUT, &var_proxy_tmout, 0, 0 },
	{ VAR_PROXY_CONNECT_TMOUT, DEF_PROXY_CONNECT_TMOUT, &var_proxy_connect_tmout, 0, 0 },
	{ VAR_PROXY_RW_TMOUT, DEF_PROXY_RW_TMOUT, &var_proxy_rw_tmout, 0, 0 },
	{ VAR_PROXY_BUFSIZE, DEF_PROXY_BUFSIZE, &var_proxy_bufsize, 0, 0 },
	{ VAR_PROXY_RETRIES, DEF_PROXY_RETRIES, &var_proxy_retries, 0, 0},
	{ 0, 0, 0, 0, 0 },
};

static ACL_CONFIG_STR_TABLE __conf_str_tab[] = {
	{ VAR_PROXY_BANNER, DEF_PROXY_BANNER, &var_proxy_banner },
	{ VAR_PROXY_BACKEND_ADDR, DEF_PROXY_BACKEND_ADDR, &var_proxy_backend_addr },
	{ VAR_PROXY_IN_FILE, DEF_PROXY_IN_FILE, &var_proxy_in_file },
	{ VAR_PROXY_OUT_FILE, DEF_PROXY_OUT_FILE, &var_proxy_out_file },
	{ VAR_PROXY_HOST_ALLOW, DEF_PROXY_HOST_ALLOW, &var_proxy_host_allow },
	{ 0, 0, 0 },
};

static ACL_EVENT *__eventp = NULL;
static ACL_VSTREAM *__front_stream = NULL;
static ACL_VSTREAM *__backend_stream = NULL;

static ACL_IPLINK *__host_allow_link = NULL;

/* forward define */
static int __if_host_allow(const char *client_ip);

static void __proxy_handle_fn(int type acl_unused, ACL_EVENT *event acl_unused,
	ACL_VSTREAM *stream, void *context)
{
	const char *myname = "__proxy_handle_fn";
	char  buf[8192];
	int   n, ret;

#undef	OVER_RETURN
#define	OVER_RETURN do { \
	acl_event_disable_readwrite(__eventp, __backend_stream); \
	acl_vstream_close(__backend_stream); \
	__backend_stream = NULL; \
	return; \
} while (0);

	if (context == NULL) {
		acl_msg_error("%s(%d)->%s: input error, null context",
				__FILE__, __LINE__, myname);
		return;
	}

	if (stream == __front_stream) {
		n = acl_vstream_read(stream, buf, sizeof(buf) - 1);
		if (n == ACL_VSTREAM_EOF) {
			if(var_proxy_log_level > 3)
				acl_msg_info("%s(%d)->%s: read over",
					__FILE__, __LINE__, myname);
			OVER_RETURN;
		}

		ret = acl_vstream_writen(__backend_stream, buf, n);
		if (ret != n) {
			if(var_proxy_log_level > 3)
				acl_msg_info("%s(%d)->%s: write error = %s",
					__FILE__, __LINE__, myname,
					strerror(errno));
			OVER_RETURN;
		}
	} else if (stream == __backend_stream) {
		n = acl_vstream_read(stream, buf, sizeof(buf) - 1);
		if (n == ACL_VSTREAM_EOF) {
			if(var_proxy_log_level > 3)
				acl_msg_info("%s(%d)->%s: read over",
					__FILE__, __LINE__, myname);
			OVER_RETURN;
		}

		ret = acl_vstream_writen(__front_stream, buf, n);
		if (ret != n) {
			if(var_proxy_log_level > 3)
				acl_msg_info("%s(%d)->%s: write error = %s",
					__FILE__, __LINE__, myname,
					strerror(errno));
			OVER_RETURN;
		}
	}
}

static void __service(void *ctx acl_unused, ACL_VSTREAM *stream)
{
	const char *myname = "__service";
	char  buf[64];
	int   connect_retries = var_proxy_retries;

	__front_stream = stream;

	if (var_proxy_log_level > 3)
		acl_msg_info("%s(%d)->%s: rw_timeout = %d",
			__FILE__, __LINE__, myname, stream->rw_timeout);

	(void) acl_getpeername(ACL_VSTREAM_SOCK(__front_stream),
			buf, sizeof(buf));
	acl_vstream_set_peer(__front_stream, buf);

	if (__if_host_allow(buf) < 0)
		return;

	if (var_proxy_log_level > 2)
		acl_msg_info("%s(%d)->%s: connected from addr=%s",
				__FILE__, __LINE__, myname,
				ACL_VSTREAM_PEER(__front_stream));

	while (connect_retries) {
		__backend_stream =
			acl_vstream_connect(var_proxy_backend_addr,
						ACL_BLOCKING,
						var_proxy_connect_tmout,
						var_proxy_rw_tmout,
						var_proxy_bufsize);
		if (__backend_stream != NULL)
			break;

		acl_msg_warn("%s(%d)->%s: can't connect server = %s,"
				" serr = %s",
				__FILE__, __LINE__, myname,
				var_proxy_backend_addr, strerror(errno));
		connect_retries--;
	}

	if (__backend_stream == NULL) {
		acl_msg_error("%s(%d)->%s: can't connect server=%s, retries=%d",
				__FILE__, __LINE__, myname,
				var_proxy_backend_addr, var_proxy_retries);
		return;
	}

	if (var_proxy_log_level > 2)
		acl_msg_info("%s(%d)->%s: ok, connect ip=%s, retries=%d",
				__FILE__, __LINE__, myname,
				ACL_VSTREAM_PEER(__backend_stream),
				var_proxy_retries - connect_retries);

	acl_event_enable_read(__eventp,
				__front_stream,
				0,
				__proxy_handle_fn,
				__front_stream);
	acl_event_enable_read(__eventp,
				__backend_stream,
				0,
				__proxy_handle_fn,
				__backend_stream);

	while (1) {
		acl_watchdog_pat();
		acl_event_loop(__eventp);
		if (__backend_stream == NULL)
			break;
	}

	acl_event_disable_readwrite(__eventp, __front_stream);
	__front_stream = NULL;
}

static void __add_one_host_item(ACL_IPLINK *allow_link, const char *pitem)
{
	char  myname[] = "__add_one_host_item";
	char *pdata = NULL;
	char *from, *to, *ptr;

#undef	RETURN
#define	RETURN do {  \
	if (pdata != NULL)  \
		acl_myfree(pdata);  \
	return;  \
} while (0);

	if (pitem == NULL || *pitem == 0) {
		acl_msg_error("%s(%d)->%s: pitem invalid",
				__FILE__, __LINE__, myname);
		RETURN;
	}

	pdata = acl_mystrdup(pitem);

	from = pdata;

	/* skip when: '[' || ',' || ' ' || '\t' */
	while (*from == '[' || *from == ',' || *from == ' ' || *from == '\t')
		from++;
	if (*from == 0) {
		acl_msg_error("%s(%d)->%s: invalid item: %s",
				__FILE__, __LINE__, myname,
				pitem);
		RETURN;
	}

	ptr = from;
	/* the "from" end with: ' ' || '\t' || ',' */
	while (*ptr) {
		if (*ptr == ' ' || *ptr == '\t' || *ptr == ',') {
			*ptr++ = 0;
			break;
		}
		ptr++;
	}
	if (*from == 0) {
		acl_msg_error("%s(%d)->%s: invalid item: %s",
				__FILE__, __LINE__, myname,
				pitem);
		RETURN;
	}

	/* skip when: ' ' || '\t' || ',' */
	while (*ptr == ' ' || *ptr == '\t' || *ptr == ',')
		ptr++;
	if (*ptr == 0) {
		acl_msg_error("%s(%d)->%s: invalid item: %s",
				__FILE__, __LINE__, myname,
				pitem);
		RETURN;
	}

	to = ptr;
	/* the "to" end with: ' ' || '\t' || ',' || ']' */
	while (*ptr) {
		if (*ptr == ' ' || *ptr == '\t' || *ptr == ',' || *ptr == ']') {
			*ptr = 0;
			break;
		}
		ptr++;
	}
	if (*to == 0) {
		acl_msg_error("%s(%d)->%s: invalid item: %s",
				__FILE__, __LINE__, myname,
				pitem);
		RETURN;
	}

	if (acl_iplink_insert(allow_link, from, to) == NULL) {
		acl_msg_error("%s(%d)->%s: call acl_iplink_insert error,"
				" invalid item: %s",
				__FILE__, __LINE__, myname, pitem);
		RETURN;
	}

	/* else: OK */
	if (var_proxy_log_level > 1)
		acl_msg_info("%s(%d)->%s: from=%s, to=%s",
				__FILE__, __LINE__, myname,
				from, to);
	RETURN;
}

static void __add_host_allow(const char *host_allow)
{
	char  myname[] = "__add_host_allow";
	char *pdata, *ptr, *pitem;

	if (host_allow == NULL)
		host_allow = DEF_PROXY_HOST_ALLOW;

	pdata = acl_mystrdup(host_allow);
	if (pdata == NULL)
		acl_msg_fatal("%s(%d)->%s: call acl_mystrdup error=%s",
				__FILE__, __LINE__, myname,
				strerror(errno));

	__host_allow_link = acl_iplink_create(10);
	if (__host_allow_link == NULL)
		acl_msg_fatal("%s(%d)->%s: call acl_iplink_create error=%s",
				__FILE__, __LINE__, myname,
				strerror(errno));
	ptr = pdata;
	while (1) {
		pitem = acl_mystrtok(&ptr, "]");
		if (pitem == NULL)
			break;
		__add_one_host_item(__host_allow_link, pitem);
	}

	acl_myfree(pdata);
}

static int __if_host_allow(const char *client_ip)
{
	char  myname[] = "__if_host_allow";
	
	if (client_ip == NULL || *client_ip == 0) {
		acl_msg_info("%s(%d)->%s: invalid input",
				__FILE__, __LINE__, myname);
		return (-1);
	}

	if (__host_allow_link == NULL) {
		if (var_proxy_log_level > 3)
			acl_msg_info("%s(%d)->%s: host allow, ip=%s",
					__FILE__, __LINE__, myname,
					client_ip);
		return (0);
	}

	if (acl_iplink_lookup_str(__host_allow_link, client_ip) != NULL) {
		if (var_proxy_log_level > 3)
			acl_msg_info("%s(%d)->%s: host allow, ip=%s",
					__FILE__, __LINE__, myname,
					client_ip);
		return (0);
	}

	if (var_proxy_log_level > 3)
		acl_msg_warn("%s(%d)->%s: host deny, ip = %s",
				__FILE__, __LINE__, myname,
				client_ip);
	return (-1);
}

static void __pre_jail_init(char *name acl_unused, char **argv acl_unused)
{
	char  myname[] = "__pre_jail_init";

	acl_msg_info("%s(%d)->%s: var_proxy_log_level=%d",
			__FILE__, __LINE__, myname, var_proxy_log_level);

	acl_msg_info("host-allow:[%s]", var_proxy_host_allow); /* zsx test */
	__add_host_allow(var_proxy_host_allow);

	if (var_proxy_log_level > 3)
		acl_msg_info("%s(%d)->%s: test only",
				__FILE__, __LINE__, myname);
	if (var_proxy_retries <= 0) {
		acl_msg_warn("%s(%d)->%s: proxy_tetries = %d, use default=%d",
				__FILE__, __LINE__, myname,
				var_proxy_retries, DEF_PROXY_RETRIES);
		var_proxy_retries = DEF_PROXY_RETRIES;
	}

	if (var_proxy_backend_addr == NULL
	    || strcmp(var_proxy_backend_addr, DEF_PROXY_BACKEND_ADDR) == 0) {
		acl_msg_fatal("%s(%d)->%s: proxy_backend_addr invalid",
				__FILE__, __LINE__, myname);
	}

	__eventp = acl_event_new_select(acl_var_single_delay_sec,
				acl_var_single_delay_usec);
	if (__eventp == NULL)
		acl_msg_fatal("%s(%d)->%s: acl_event_new error",
				__FILE__, __LINE__, myname);
}

static void __post_jail_init(char *name acl_unused, char **argv acl_unused)
{
	char  myname[] = "__post_jail_init";

	if (var_proxy_log_level > 3)
		acl_msg_info("%s(%d)->%s: test only",
				__FILE__, __LINE__, myname);
}

int main(int argc, char *argv[])
{
	acl_msg_info("%s: start...", __FILE__);
	printf("%s: starting...", __FILE__);

	acl_single_server_main(argc, argv, __service,
				ACL_MASTER_SERVER_INT_TABLE, __conf_int_tab,
				ACL_MASTER_SERVER_STR_TABLE, __conf_str_tab,
				ACL_MASTER_SERVER_PRE_INIT, __pre_jail_init,
				ACL_MASTER_SERVER_POST_INIT, __post_jail_init,
				0);
	exit (0);
}

