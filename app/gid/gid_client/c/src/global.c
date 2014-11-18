#include "lib_acl.h"

#include "global.h"
#include "lib_gid.h"

char  var_gid_server_addr[64] = "";
int   var_gid_conn_timeout;
int   var_gid_rw_timeout;
int   var_gid_retry_limit;
int   var_gid_keepalive;
int   var_gid_proto;
char  var_gid_url[1024];

void gid_client_init(int proto, const char *server_addr)
{
	acl_assert(server_addr && *server_addr);

	ACL_SAFE_STRNCPY(var_gid_server_addr, server_addr,
		sizeof(var_gid_server_addr));
	if (proto == GID_PROTO_JSON)
		ACL_SAFE_STRNCPY(var_gid_url, GID_JSON_URL,
			sizeof(var_gid_url));
	else if (proto == GID_PROTO_XML)
		ACL_SAFE_STRNCPY(var_gid_url, GID_XML_URL,
			sizeof(var_gid_url));
	else if (proto != GID_PROTO_CMDLINE)
		acl_assert(0);

	var_gid_proto = proto;
	var_gid_conn_timeout = 20;
	var_gid_rw_timeout = 20;
	var_gid_retry_limit = 1;
	var_gid_keepalive = 1;
}

void gid_client_set_url(const char *url)
{
	if (url == NULL || *url == 0)
		return;
	ACL_SAFE_STRNCPY(var_gid_url, url, sizeof(var_gid_url));
}

void gid_client_set_keepalive(int keepalive)
{
	var_gid_keepalive = keepalive;
}

void gid_client_set_retry_limit(int nretry)
{
	var_gid_retry_limit = nretry;
}

void gid_client_set_conn_timeout(int timeout)
{
	var_gid_conn_timeout = timeout;
}

void gid_client_set_rw_timeout(int timeout)
{
	var_gid_rw_timeout = timeout;
}
