
#ifndef	__SPOOL_GLOBAL_INCLUDE_H__
#define	__SPOOL_GLOBAL_INCLUDE_H__

#ifdef	__cplusplus
extern "C" {
#endif

#define	VAR_CFG_MAX_THREADS		"max_threads"
#define	VAR_CFG_CLIENT_IDLE_LIMIT	"client_idle_limit"
#define	VAR_CFG_ACCESS_ALLOW		"access_allow"

extern int var_cfg_max_threads;
extern int var_cfg_client_idle_limit;
extern char *var_cfg_access_allow;

#ifdef	__cplusplus
}
#endif

#endif
