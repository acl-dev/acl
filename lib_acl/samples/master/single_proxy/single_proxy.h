
#ifndef	__SINGLE_PROXY_INCLUDE_H_
#define	__SINGLE_PROXY_INCLUDE_H_

#ifdef	__cplusplus
extern "C" {
#endif

#define	VAR_PROXY_TMOUT			"proxy_timeout"
#define	DEF_PROXY_TMOUT			60
extern int var_proxy_tmout;

#define	VAR_PROXY_CONNECT_TMOUT		"proxy_connect_timeout"
#define	DEF_PROXY_CONNECT_TMOUT		60
extern int var_proxy_connect_tmout;

#define	VAR_PROXY_RW_TMOUT		"proxy_rw_timeout"
#define	DEF_PROXY_RW_TMOUT		60
extern int var_proxy_rw_tmout;

#define	VAR_PROXY_BUFSIZE		"proxy_bufsize"
#define	DEF_PROXY_BUFSIZE		4096
extern int var_proxy_bufsize;

#define	VAR_PROXY_RETRIES		"proxy_retries"
#define	DEF_PROXY_RETRIES		1
extern int var_proxy_retries;

#define	VAR_PROXY_HOST_ALLOW		"proxy_host_allow"
#define	DEF_PROXY_HOST_ALLOW		"[1.0.0.1, 255.255.255.0]"
extern char *var_proxy_host_allow;

#define	VAR_PROXY_BANNER		"proxy_banner"
#define	DEF_PROXY_BANNER		"hello, welcome!"
extern char *var_proxy_banner;

#define	VAR_PROXY_BACKEND_ADDR		"proxy_backend_addr"
#define	DEF_PROXY_BACKEND_ADDR		"none"
extern char *var_proxy_backend_addr;

#define	VAR_PROXY_LOG_LEVEL		"proxy_log_level"
#define	DEF_PROXY_LOG_LEVEL		0
extern int   var_proxy_log_level;

#define	VAR_PROXY_IN_FILE		"proxy_in_file"
#define	DEF_PROXY_IN_FILE		"in.log"
extern char *var_proxy_in_file;

#define	VAR_PROXY_OUT_FILE		"proxy_out_file"
#define	DEF_PROXY_OUT_FILE		"out.log"
extern char *var_proxy_out_file;

#ifdef	__cplusplus
}
#endif

#endif

