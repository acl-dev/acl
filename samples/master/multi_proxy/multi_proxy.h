
#ifndef	__TEST_MULTI_PROXY_INCLUDE_H_
#define	__TEST_MULTI_PROXY_INCLUDE_H_

#ifdef	__cplusplus
extern "C" {
#endif

#define	VAR_PROXY_TMOUT			"proxy_timeout"
#define	DEF_PROXY_TMOUT			60
extern int var_proxy_tmout;

#define	VAR_PROXY_BANNER		"proxy_banner"
#define	DEF_PROXY_BANNER		"hello, welcome!"
extern char *var_proxy_banner;

#define	VAR_PROXY_CONNECT_TMOUT		"proxy_connect_timeout"
#define	DEF_PROXY_CONNECT_TMOUT		60
extern int var_proxy_connect_tmout;

#define	VAR_PROXY_RW_TMOUT		"proxy_rw_timeout"
#define	DEF_PROXY_RW_TMOUT		60
extern int var_proxy_rw_tmout;

#define	VAR_PROXY_BUFSIZE		"proxy_bufsize"
#define	DEF_PROXY_BUFSIZE		4096
extern int var_proxy_bufsize;

#define	VAR_PROXY_BACKEND_ADDR		"proxy_backend_addr"
#define	DEF_PROXY_BACKEND_ADDR		"none"
extern char *var_proxy_backend_addr;

#define	VAR_PROXY_LOG_LEVEL		"proxy_log_level"
#define	DEF_PROXY_LOG_LEVEL		0
extern int   var_proxy_log_level;

#define	VAR_PROXY_REQUEST_FILE		"proxy_request_file"
#define	DEF_PROXY_REQUEST_FILE		"request.log"
extern char *var_proxy_request_file;

#define	VAR_PROXY_RESPOND_FILE		"proxy_respond_file"
#define	DEF_PROXY_RESPOND_FILE		"respond.log"
extern char *var_proxy_respond_file;

#define	VAR_PROXY_DEBUG_REQUEST		"proxy_debug_request"
#define	DEF_PROXY_DEBUG_REQUEST		0
extern int var_proxy_debug_request;

#define	VAR_PROXY_DEBUG_RESPOND		"proxy_debug_respond"
#define	DEF_PROXY_DEBUG_RESPOND		0
extern int var_proxy_debug_respond;

#ifdef	__cplusplus
}
#endif

#endif

