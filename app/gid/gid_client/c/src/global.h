#ifndef	__GLOBAL_INCLUDE_H__
#define	__GLOBAL_INCLUDE_H__

extern char  var_gid_server_addr[];
extern int   var_gid_conn_timeout;
extern int   var_gid_rw_timeout;
extern int   var_gid_retry_limit;
extern int   var_gid_proto;
extern int   var_gid_keepalive;
extern char  var_gid_url[];

#define	GID_JSON_URL	"/gid_json"
#define	GID_XML_URL	"/gid_xml"

#endif
