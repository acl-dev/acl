#ifndef	_TLS_PARAMS_INCLUDED_
#define	_TLS_PARAMS_INCLUDED_

#include "tls.h"

#define VAR_TLS_DAEMON_RAND_BYTES	"tls_daemon_random_bytes"
#define DEF_TLS_DAEMON_RAND_BYTES	0
/*#define DEF_TLS_DAEMON_RAND_BYTES	32*/
extern TLS_API int var_tls_daemon_rand_bytes;

#define VAR_SERVER_TLS_PROTO		"server_tls_protocols"
#define DEF_SERVER_TLS_PROTO		""
extern TLS_API char *var_server_tls_proto;

#define VAR_SERVER_TLS_MAND_PROTO	"server_tls_mandatory_protocols"
#define DEF_SERVER_TLS_MAND_PROTO	"SSLv3, TLSv1"
extern TLS_API char *var_server_tls_mand_proto;

 /*
  * TLS cipherlists
  */
#define VAR_TLS_HIGH_CLIST		"tls_high_cipherlist"
#define DEF_TLS_HIGH_CLIST		"ALL:!EXPORT:!LOW:!MEDIUM:+RC4:@STRENGTH"
extern TLS_API char *var_tls_high_clist;

#define VAR_TLS_MEDIUM_CLIST		"tls_medium_cipherlist"
#define DEF_TLS_MEDIUM_CLIST		"ALL:!EXPORT:!LOW:+RC4:@STRENGTH"
extern TLS_API char *var_tls_medium_clist;

#define VAR_TLS_LOW_CLIST		"tls_low_cipherlist"
#define DEF_TLS_LOW_CLIST		"ALL:!EXPORT:+RC4:@STRENGTH"
extern TLS_API char *var_tls_low_clist;

#define VAR_TLS_EXPORT_CLIST		"tls_export_cipherlist"
#define DEF_TLS_EXPORT_CLIST		"ALL:+RC4:@STRENGTH"
extern TLS_API char *var_tls_export_clist;

#define VAR_TLS_NULL_CLIST		"tls_null_cipherlist"
#define DEF_TLS_NULL_CLIST		"eNULL:!aNULL"
extern TLS_API char *var_tls_null_clist;

#define VAR_TLS_EECDH_STRONG		"tls_eecdh_strong_curve"
#define DEF_TLS_EECDH_STRONG		"prime256v1"
extern TLS_API char *var_tls_eecdh_strong;

#define VAR_TLS_EECDH_ULTRA		"tls_eecdh_ultra_curve"
#define DEF_TLS_EECDH_ULTRA		"secp384r1"
extern TLS_API char *var_tls_eecdh_ultra;

 /*
  * How long an intra-mail command may take before we assume the mail system
  * is in deadlock (should never happen).
  */
#define VAR_IPC_TIMEOUT			"ipc_timeout"
#define DEF_IPC_TIMEOUT			3600
extern TLS_API int var_ipc_timeout;

 /*
  * Any subsystem: default amount of time a mail subsystem keeps an internal
  * IPC connection before closing it because it is idle for too much time.
  */
#define VAR_IPC_IDLE			"ipc_idle"
#define DEF_IPC_IDLE			5
extern TLS_API int var_ipc_idle_limit;

 /*
  * Any subsystem: default amount of time a mail subsystem keeps an internal
  * IPC connection before closing it because the connection has existed for
  * too much time.
  */
#define VAR_IPC_TTL			"ipc_ttl"
#define DEF_IPC_TTL			1000
extern TLS_API int var_ipc_ttl_limit;

/*
 * Global
 */
#ifndef	VAR_TLS_MGR_SERVICE
#define	VAR_TLS_MGR_SERVICE
#endif
extern TLS_API char  var_tlsmgr_service[256];

extern TLS_API int   var_tlsmgr_stand_alone;

#define VAR_TLS_RAND_SOURCE		"tls_random_source"
#ifdef PREFERRED_RAND_SOURCE
#define DEF_TLS_RAND_SOURCE		PREFERRED_RAND_SOURCE
#else
#define DEF_TLS_RAND_SOURCE		""
#endif
extern TLS_API char *var_tls_rand_source;

#define VAR_TLS_RAND_EXCH_NAME		"tls_random_exchange_name"
/*
#define DEF_TLS_RAND_EXCH_NAME		"${data_directory}/prng_exch"
*/
#define DEF_TLS_RAND_EXCH_NAME		""
extern TLS_API char *var_tls_rand_exch_name;

#define VAR_SERVER_TLS_SCACHE_DB	"server_tls_session_cache_database"
#define DEF_SERVER_TLS_SCACHE_DB	""
extern TLS_API char *var_server_tls_scache_db;

#define VAR_CLIENT_TLS_SCACHE_DB	"client_tls_session_cache_database"
#define DEF_CLIENT_TLS_SCACHE_DB	""
extern TLS_API char *var_client_tls_scache_db;

#define VAR_TLS_RESEED_PERIOD		"tls_random_reseed_period"
#define DEF_TLS_RESEED_PERIOD		3600
extern TLS_API int var_tls_reseed_period;

#define VAR_TLS_PRNG_UPD_PERIOD		"tls_random_prng_update_period"
#define DEF_TLS_PRNG_UPD_PERIOD		3600
extern TLS_API int var_tls_prng_upd_period;

#define VAR_SERVER_TLS_SCACHTIME	"server_tls_session_cache_timeout"
#define DEF_SERVER_TLS_SCACHTIME	3600
extern TLS_API int var_server_tls_scache_timeout;

#define VAR_CLIENT_TLS_SCACHTIME	"client_tls_session_cache_timeout"
#define DEF_CLIENT_TLS_SCACHTIME	3600
extern TLS_API int var_client_tls_scache_timeout;

#define VAR_TLS_RAND_BYTES		"tls_random_bytes"
#define DEF_TLS_RAND_BYTES		32
extern TLS_API int var_tls_rand_bytes;

#define VAR_SERVER_TLS_LOGLEVEL		"server_tls_loglevel"
#define DEF_SERVER_TLS_LOGLEVEL		0
extern TLS_API int var_server_tls_loglevel;

#define VAR_CLIENT_TLS_LOGLEVEL		"client_tls_loglevel"
#define DEF_CLIENT_TLS_LOGLEVEL		0
extern TLS_API int var_client_tls_loglevel;	/* In client(8) and tlsmgr(8) */

/*
  * Attribute names.
  */
#define TLS_ATTR_STATUS			"status"

 /*
  * Generic triggers.
  */
#define TRIGGER_REQ_WAKEUP		'W'     /* wakeup */

#endif
