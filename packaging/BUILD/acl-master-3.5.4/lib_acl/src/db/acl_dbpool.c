#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_stdlib.h"
#include "db/acl_dbpool.h"

#endif

#ifndef ACL_CLIENT_ONLY

#include "mysql/acl_dbpool_mysql.h"
#include "null/acl_dbpool_null.h"

/*----------------------------------------------------------------------------*/
#if 0
static void __dbpool_debug_cfg(const ACL_DB_INFO *db_info)
{
	acl_msg_info("db_addr = %s\n", db_info->db_addr);
	acl_msg_info("db_user = %s\n", db_info->db_user);
	acl_msg_info("db_pass = %s\n", db_info->db_pass);
	acl_msg_info("db_name = %s\n", db_info->db_name);
	acl_msg_info("db_max  = %d\n", db_info->db_max);
	acl_msg_info("ping_inter = %d\n", db_info->ping_inter);
	acl_msg_info("timeout_inter = %d\n", db_info->timeout_inter);
}
#endif
/*----------------------------------------------------------------------------*/
ACL_DB_POOL *acl_dbpool_create(const char *db_type, const ACL_DB_INFO *db_info)
{
	const char *myname = "acl_dbpool_create";
	ACL_DB_POOL *db_pool = NULL;
	ACL_DB_INFO *info;

	if (db_type == NULL)
		db_type = "mysql";

	if (strcasecmp(db_type, "null") == 0)
		db_pool = acl_dbpool_null_create(db_info);
#ifdef	HAS_MYSQL
	else if (strcasecmp(db_type, "mysql") == 0)
		db_pool = acl_dbpool_mysql_create(db_info);
#endif
	else {
		acl_msg_fatal("%s, %s(%d): %s not supported yet",
				__FILE__, myname, __LINE__, db_type);
	}

	if (db_pool == NULL) {
		acl_msg_error("%s, %s(%d): dbpool_%s_create error",
				__FILE__, myname, __LINE__, db_type);
		return (NULL);
	}

	info = &db_pool->db_info;

#if 0
	ACL_SAFE_STRNCPY(info->db_addr, db_info->db_addr, sizeof(info->db_addr));
	ACL_SAFE_STRNCPY(info->db_name, db_info->db_name, sizeof(info->db_name));
	ACL_SAFE_STRNCPY(info->db_user, db_info->db_user, sizeof(info->db_user));
	ACL_SAFE_STRNCPY(info->db_pass, db_info->db_pass, sizeof(info->db_pass));

	info->db_max        = db_info->db_max;
	info->ping_inter    = db_info->ping_inter;
	info->timeout_inter = db_info->timeout_inter;
#endif

	memcpy(info, db_info, sizeof(ACL_DB_INFO));

	db_pool->db_max   = info->db_max;
	db_pool->db_ready = 0;
	db_pool->db_inuse = 0;
	
	return (db_pool);
}

/*----------------------------------------------------------------------------*/
void acl_dbpool_destroy(ACL_DB_POOL *db_pool)
{
	db_pool->destroy(db_pool);
}

/*----------------------------------------------------------------------------*/
ACL_DB_HANDLE *acl_dbpool_peek(ACL_DB_POOL *db_pool)
{
	return (db_pool->dbh_peek(db_pool));
}

/*----------------------------------------------------------------------------*/
void acl_dbpool_check(ACL_DB_POOL *db_pool)
{
	db_pool->dbh_check(db_pool);
}

/*----------------------------------------------------------------------------*/
void acl_dbpool_release(ACL_DB_HANDLE *db_handle)
{
	ACL_DB_POOL *db_pool;

	db_pool = db_handle->parent;

/*	if ((db_pool->db_info.debug_flag & ACL_DB_DEBUG_MEM))
		acl_memory_stat();
*/

	db_pool->dbh_release(db_handle);
}

/*----------------------------------------------------------------------------*/
void *acl_dbpool_export(ACL_DB_HANDLE *db_handle)
{
	ACL_DB_POOL *db_pool;

	db_pool = db_handle->parent;
	return (db_pool->dbh_export(db_handle));
}

/*----------------------------------------------------------------------------*/
void acl_dbpool_close(ACL_DB_HANDLE *db_handle)
{
	ACL_DB_POOL *db_pool;

	db_pool = db_handle->parent;
	db_pool->dbh_close(db_handle);
}
/*----------------------------------------------------------------------------*/
void acl_dbpool_set_ping(ACL_DB_POOL *db_pool, int (*ping_fn)(ACL_DB_HANDLE*))
{
	db_pool->dbh_ping = ping_fn;
}
/*----------------------------------------------------------------------------*/
#endif /* ACL_CLIENT_ONLY */
