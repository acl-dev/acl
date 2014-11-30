#ifndef	__PROBE_INCLUDE_H__
#define	__PROBE_INCLUDE_H__

#ifdef	__cplusplus
extern "C" {
#endif

#include "lib_acl.h"
#include "lib_protocol.h"

/* 错误号 */
#define	PROBE_HTTP_STAT_MAX	599
#define	PROBE_HTTP_STAT_200	200
#define	PROBE_HTTP_STAT_404	404
#define	PROBE_HTTP_STAT_500	500
#define	PROBE_HTTP_STAT_503	503
#define	PROBE_HTTP_STAT_504	504
#define	PROBE_HTTP_STAT_505	505

#define	PROBE_ERR_NONE		0		/* 无错误 */
#define	PROBE_ERR_CONN		-1		/* 连接主机错误 */
#define	PROBE_ERR_HTTP		-2		/* HTTP协议返回错误 */
#define	PROBE_ERR_URL		-3		/* 所请求的URL返回错误 */
#define	PROBE_ERR_ARG		-4		/* 输入参数错误 */
#define	PROBE_ERR_DSEARCH	-5		/* 所查询的域结点不存在 */
#define	PROBE_ERR_HSEARCH	-6		/* 所查询的主机结点不存在 */
#define	PROBE_ERR_HDEAD		-7		/* 所查询的主机结点已经死掉 */
#define	PROBE_ERR_SOPEN		-8		/* 打开流错误 */
#define	PROBE_ERR_WRITE		-9		/* 写错误 */
#define	PROBE_ERR_READ		-10		/* 读错误 */
#define	PROBE_ERR_TOO_MANY_ITEM	-11		/* 头部回复行数太多 */

#define	HOST_STATUS_ALIVE	0		/* 该主机处于存活状态 */
#define	HOST_STATUS_DEAD	1		/* 该主机已经死掉 */

#define	HOST_FLAG_PROBE_FREE	0		/* 该主机未处于探测状态 */
#define	HOST_FLAG_PROBE_BUSY	1		/* 该主机已经处于探测状态了 */


/* 常量定义 */
#define	HTTP_HEADER_MAX_SIZE		8192
#define	HTTP_HEADER_MAX_NUM		20


typedef struct HTTP_HEADER_ITEM {
	char *name;
	char *value;
} HTTP_HEADER_ITEM;

typedef struct PROBE_STAT {
	time_t time_begin;
	time_t time_end;
	time_t time_cost;
	char http_status[32];
	int    error_num;
} PROBE_STAT;

typedef struct PROBE_SERVER {
	/* come from configure file */
	char *name;
	char *addr;
	char *url;
	int   connect_timeout;
	int   rw_timeout;
	int   retry_inter;
	int   probe_inter;
	char *http_status_errors;
	char *logfile;
	int   warn_time;

	ACL_VSTREAM *logfp;

	char  http_request_header[HTTP_HEADER_MAX_SIZE];
	int   http_request_len;

	time_t time_begin;
	time_t time_end;
	time_t time_total_cost;

	ACL_AIO      *aio;
	ACL_ASTREAM  *stream;
	HTTP_HDR_RES *hdr_res;
	HTTP_RES     *res;
} PROBE_SERVER;


#define	VAR_CFG_SERVER			"probed_server"
#define	VAR_CFG_SERVER_NAME		"server_name"
#define	VAR_CFG_SERVER_ADDR		"addr"
#define	VAR_CFG_SERVER_URL		"url"
#define	VAR_CFG_SERVER_CONNECT_TIMEOUT	"connect_timeout"
#define	VAR_CFG_SERVER_RW_TIMEOUT	"rw_timeout"
#define	VAR_CFG_SERVER_RETRY_INTER	"retry_inter"
#define	VAR_CFG_SERVER_PROBE_INTER	"probe_inter"
#define	VAR_CFG_SERVER_HTTP_ERRORS	"http_errors"
#define	VAR_CFG_SERVER_LOG		"log"
#define	VAR_CFG_SERVER_WARN_TIME	"warn_time"

#define	VAR_CFG_LOGFILE			"logfile"
#define	VAR_CFG_LOGLEVEL		"loglevel"
#define	VAR_CFG_STATLEN			"statlen"
#define	VAR_CFG_FORK_SLEEP		"fork_sleep"
#define	VAR_CFG_DEBUG_FILE		"debug_file"

/* in probe_cfg.c */
extern ACL_ARRAY *var_probe_server_link;
extern const char *var_probe_logfile;
extern int   var_probe_loglevel;
extern int   var_probe_statlen;
extern int   var_probe_fork_sleep;
extern ACL_VSTREAM *var_probe_debug_fp;

extern void probe_cfg_load(void);

/* in probe_run.c */
extern void probe_run(void);

/* in main.c */
extern char *var_cfg_file;

#ifdef	__cplusplus
}
#endif

#endif

