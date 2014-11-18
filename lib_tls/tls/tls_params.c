#include "StdAfx.h"

#ifdef USE_TLS

#include "tls.h"
#include "tls_params.h"
#include "tls_private.h"

 /*
  * ipc params
  */
int   var_ipc_timeout;
int   var_ipc_idle_limit;
int   var_ipc_ttl_limit;

 /*
  * Tunable parameters.
  */
char *var_tls_high_clist;
char *var_tls_medium_clist;
char *var_tls_low_clist;
char *var_tls_export_clist;
char *var_tls_null_clist;
int   var_tls_daemon_rand_bytes;
char *var_tls_eecdh_strong;
char *var_tls_eecdh_ultra;
char *var_server_tls_mand_proto;
char *var_server_tls_proto;

char var_tlsmgr_service[256];

/* tls_params_init - Load TLS related config parameters */

void    tls_params_init(void)
{
    const char *myname = "tls_param_init";
    static ACL_CONFIG_STR_TABLE str_table[] = {
	{ VAR_TLS_HIGH_CLIST, DEF_TLS_HIGH_CLIST, &var_tls_high_clist },
	{ VAR_TLS_MEDIUM_CLIST, DEF_TLS_MEDIUM_CLIST, &var_tls_medium_clist },
	{ VAR_TLS_LOW_CLIST, DEF_TLS_LOW_CLIST, &var_tls_low_clist },
	{ VAR_TLS_EXPORT_CLIST, DEF_TLS_EXPORT_CLIST, &var_tls_export_clist },
	{ VAR_TLS_NULL_CLIST, DEF_TLS_NULL_CLIST, &var_tls_null_clist },
	{ VAR_TLS_EECDH_STRONG, DEF_TLS_EECDH_STRONG, &var_tls_eecdh_strong },
	{ VAR_TLS_EECDH_ULTRA, DEF_TLS_EECDH_ULTRA, &var_tls_eecdh_ultra },

	{ VAR_SERVER_TLS_MAND_PROTO, DEF_SERVER_TLS_MAND_PROTO, &var_server_tls_mand_proto },
	{ VAR_SERVER_TLS_PROTO, DEF_SERVER_TLS_PROTO, &var_server_tls_proto },

	{ 0, 0, 0 },
    };
    static ACL_CONFIG_INT_TABLE int_table[] = {
	{ VAR_TLS_DAEMON_RAND_BYTES, DEF_TLS_DAEMON_RAND_BYTES, &var_tls_daemon_rand_bytes, 1, 0 },

	{ VAR_IPC_TIMEOUT, DEF_IPC_TIMEOUT, &var_ipc_timeout, 0, 0 },
	{ VAR_IPC_IDLE, DEF_IPC_IDLE, &var_ipc_idle_limit, 0, 0 },
	{ VAR_IPC_TTL, DEF_IPC_TTL, &var_ipc_ttl_limit, 0, 0 },

	{ 0, 0, 0, 0, 0 },
    };
    static int init_done;

    if (init_done) {
        acl_msg_warn("%s: has been called before", myname);
	return;
    }
    init_done = 1;

    acl_get_app_conf_str_table(str_table);
    acl_get_app_conf_int_table(int_table);
}

#endif
