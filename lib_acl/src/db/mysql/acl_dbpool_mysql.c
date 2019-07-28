#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "thread/acl_thread.h"
#include "stdlib/acl_stdlib.h"
#include "db/acl_dbpool.h"

#endif

#ifndef ACL_CLIENT_ONLY
#ifdef	HAS_MYSQL

#include "mysql.h"
#include "acl_dbmysql.h"
#include "acl_dbpool_mysql.h"

typedef struct ACL_DB_HANDLE_MYSQL {
	ACL_DB_HANDLE handle;
	MYSQL   *connection;
} ACL_DB_HANDLE_MYSQL;

typedef struct ACL_DB_POOL_MYSQL {
	ACL_DB_POOL db_pool;

	acl_pthread_mutex_t mutex;
	ACL_ARRAY *handles;
	time_t when_check;
} ACL_DB_POOL_MYSQL;

#define	DB_LOCK(mlock) { acl_pthread_mutex_lock(&mlock); }
#define	DB_UNLOCK(mlock) { acl_pthread_mutex_unlock(&mlock); }

static void __close_mysql_handle(ACL_DB_HANDLE_MYSQL *mysql_handle);

/*----------------------------------------------------------------------------*/
static ACL_DB_HANDLE_MYSQL *__new_mysql_handle(void)
{
	ACL_DB_HANDLE_MYSQL *mysql_handle;

	mysql_handle = (ACL_DB_HANDLE_MYSQL *)
		acl_mycalloc(1, sizeof(ACL_DB_HANDLE_MYSQL));

	mysql_handle->handle.sql_results = acl_dbmysql_results;
	mysql_handle->handle.sql_result = acl_dbmysql_result;
	mysql_handle->handle.sql_update = acl_dbmysql_update;
	mysql_handle->handle.sql_select = acl_dbmysql_select;
	mysql_handle->handle.free_result = acl_dbmysql_free_result;

	return (mysql_handle);
}

static ACL_DB_HANDLE_MYSQL *__open_mysql_handle(ACL_DB_POOL_MYSQL *mysql_pool,
	ACL_DB_HANDLE_MYSQL *mysql_handle, ACL_DB_INFO *db_info)
{
	const char *myname = "__open_mysql_handle";
	int   reuse_flag = 0;
	const char *ptr;
	char *db_host, *db_unix;
	int   db_port;
	char  tmpbuf[256];
	int   n, len, i;
	my_bool reconnect = 1;

	ptr = strchr(db_info->db_addr, '/');
	if (ptr == NULL) {
		ptr = strchr(db_info->db_addr, ':');
		if (ptr == NULL)
			acl_msg_fatal("%s, %s(%d): invalid db_addr=%s",
				__FILE__, myname, __LINE__, db_info->db_addr);
		len = ptr - db_info->db_addr;
		if (len == 0)
			acl_msg_fatal("%s, %s(%d): invalid db_addr=%s",
				__FILE__, myname, __LINE__, db_info->db_addr);

		len++;	/* 1 for '\0' */
		i = sizeof(tmpbuf) - 1;
		n = i > len ? len : i - 1;
		ACL_SAFE_STRNCPY(tmpbuf, db_info->db_addr, n);
		db_host = tmpbuf;

		ptr++;  /* skip ':' */
		db_port = atoi(ptr);
		if (db_port <= 0)
			acl_msg_fatal("%s, %s(%d): invalid port=%d",
				__FILE__, myname, __LINE__, db_port);

		db_unix = NULL;
	} else {
		db_unix = db_info->db_addr;
		db_host = NULL;
		db_port = 0;
	}

	if (mysql_handle == NULL) {
		mysql_handle = __new_mysql_handle();
		mysql_handle->handle.parent = &mysql_pool->db_pool;
	} else
		reuse_flag = 1;

	mysql_handle->connection = mysql_init(NULL);
	if (mysql_handle->connection == NULL) {
		acl_msg_error("%s, %s(%d): mysql init error",
			__FILE__, myname, __LINE__);
		if (!reuse_flag)
			acl_myfree(mysql_handle);
		return (NULL);
	}

	if (db_info->conn_timeout > 0)
		mysql_options(mysql_handle->connection,
			MYSQL_OPT_CONNECT_TIMEOUT,
			(const void*) &db_info->conn_timeout);
	if (db_info->rw_timeout > 0) {
		mysql_options(mysql_handle->connection,
			MYSQL_OPT_READ_TIMEOUT,
			(const void*) &db_info->rw_timeout);
		mysql_options(mysql_handle->connection,
			MYSQL_OPT_WRITE_TIMEOUT,
			(const void*) &db_info->rw_timeout);
	}
	mysql_options(mysql_handle->connection,
		MYSQL_OPT_RECONNECT, (const void*) &reconnect);

	if (db_info->db_before_connect
		&& db_info->db_before_connect((ACL_DB_HANDLE*) mysql_handle,
			db_info->ctx) < 0)
	{
		acl_msg_error("%s, %s(%d): db_before_connect return < 0",
			__FILE__, myname, __LINE__);
		mysql_close(mysql_handle->connection);
		mysql_handle->connection = NULL;
		if (!reuse_flag)
			acl_myfree(mysql_handle);
		return (NULL);
	}

	if (mysql_real_connect(mysql_handle->connection,
				db_host,
				db_info->db_user,
				db_info->db_pass,
				db_info->db_name,
				db_port,
				db_unix,
				db_info->db_flags) == NULL) {
		acl_msg_error("%s, %s(%d): connect mysql error(%s), db_host=%s",
				__FILE__, myname, __LINE__,
				mysql_error(mysql_handle->connection), db_info->db_addr);

		mysql_close(mysql_handle->connection);
		mysql_handle->connection = NULL;
		if (!reuse_flag)
			acl_myfree(mysql_handle);
		return (NULL);
	}

#if MYSQL_VERSION_ID >= 50000
	if (mysql_autocommit(mysql_handle->connection, db_info->auto_commit) != 0) {
		acl_msg_error("%s, %s(%d): mysql_autocommit error",
				__FILE__, myname, __LINE__);
		mysql_close(mysql_handle->connection);
		mysql_handle->connection = NULL;
		if (!reuse_flag)
			acl_myfree(mysql_handle);
		return (NULL);
	}
#else
	db_info->auto_commit = 0;
#endif

	if (db_info->db_after_connect
		&& db_info->db_after_connect((ACL_DB_HANDLE*) mysql_handle,
			db_info->ctx) < 0)
	{
		acl_msg_error("%s, %s(%d): db_after_connect return < 0",
			__FILE__, myname, __LINE__);
		mysql_handle->connection = NULL;
		if (!reuse_flag)
			acl_myfree(mysql_handle);
		return (NULL);
	}

	if (acl_msg_verbose)
		acl_msg_info("OK, database connected, db_host: %s"
			", db_name: %s, db_user: %s, autocommit %s",
			db_info->db_addr, db_info->db_name, db_info->db_user,
			db_info->auto_commit ? "true" : "false");
#if 0
	acl_msg_info("db_pass: %s\n", db_info->db_pass);
#endif

	mysql_handle->handle.status  = ACL_DBH_STATUS_READY;
	mysql_handle->handle.timeout = time(NULL) + db_info->timeout_inter;
	mysql_handle->handle.ping    = time(NULL) + db_info->ping_inter;

	if (!reuse_flag) {
		if (acl_array_append(mysql_pool->handles, mysql_handle) < 0) {
			acl_msg_fatal("%s, %s(%d): append to handles error(%s)",
				__FILE__, myname, __LINE__, acl_last_serror());
		}
	}

	return (mysql_handle);
}

int sane_mysql_reopen(ACL_DB_HANDLE *handle)
{
	const char *myname = "sane_mysql_repopen";
	ACL_DB_HANDLE_MYSQL *mysql_handle = (ACL_DB_HANDLE_MYSQL*) handle;
	MYSQL *myconn_saved = mysql_handle->connection;
	ACL_DB_INFO *db_info = &mysql_handle->handle.parent->db_info;

	if (__open_mysql_handle((ACL_DB_POOL_MYSQL*) handle->parent,
			mysql_handle, db_info) == NULL)
	{
		acl_msg_error("%s(%d): reopen mysql connection error",
			myname, __LINE__);
		mysql_handle->connection = myconn_saved;
		return (-1);
	}
	__close_mysql_handle(mysql_handle);
	return (0);
}

/*----------------------------------------------------------------------------*/
static void __close_mysql_handle(ACL_DB_HANDLE_MYSQL *mysql_handle)
{
	if (mysql_handle->connection == NULL)
		return;
	mysql_close(mysql_handle->connection);
	mysql_handle->connection = NULL;
	mysql_handle->handle.status = ACL_DBH_STATUS_NULL;
	mysql_handle->handle.timeout = 0;
	mysql_handle->handle.ping = 0;
}

/*----------------------------------------------------------------------------*/
static void __dbpool_mysql_check(ACL_DB_POOL *db_pool)
{
	ACL_DB_POOL_MYSQL *mysql_pool = (ACL_DB_POOL_MYSQL*) db_pool;
	ACL_DB_HANDLE_MYSQL *mysql_handle;
	int  i, n, ping_inter;
	time_t now = time(NULL);

	ping_inter = mysql_pool->db_pool.db_info.ping_inter;

	n = acl_array_size(mysql_pool->handles);

	for (i = 0; i < n; i++) {
		mysql_handle = (ACL_DB_HANDLE_MYSQL *)
				acl_array_index(mysql_pool->handles, i);
		if (mysql_handle == NULL)
			continue;
		if (mysql_handle->handle.status != ACL_DBH_STATUS_READY)
			continue;

		/* if the connecion is idle timeout ? */
		if (now > mysql_handle->handle.timeout) {
			__close_mysql_handle(mysql_handle);
			mysql_pool->db_pool.db_ready--;
			continue;
		}

		/* has the ping time reached ? */
		if (now <= mysql_handle->handle.ping) 
			continue;

		if (mysql_ping(mysql_handle->connection) == 0) {
			mysql_handle->handle.ping = time(NULL) + ping_inter;
		} else {
			__close_mysql_handle(mysql_handle);
			mysql_pool->db_pool.db_ready--;
		}
	}
}

/*----------------------------------------------------------------------------*/
static ACL_DB_HANDLE *__dbpool_mysql_peek(ACL_DB_POOL *db_pool)
{
	const char *myname = "__dbpool_mysql_peek";
	ACL_DB_POOL_MYSQL *mysql_pool = (ACL_DB_POOL_MYSQL *) db_pool;
	ACL_DB_HANDLE_MYSQL *mysql_handle, *mysql_handle_slot = NULL;
	int   i, n;
	time_t now;
	static time_t last_time;  /* 因为在调用此函数时已经上锁,
				   * 所以此处声明一静态变量是线程安全的.
				   */

#undef	RETURN
#define	RETURN(_x_) do { \
	now = time(NULL); \
	if (acl_msg_verbose && now - last_time > 5) { \
		acl_msg_info("Database status: max = %d, idle = %d, busy = %d", \
				db_pool->db_max, db_pool->db_ready, db_pool->db_inuse); \
		last_time = now; \
	} \
	DB_UNLOCK(mysql_pool->mutex); \
	return (_x_); \
} while (0)

	DB_LOCK(mysql_pool->mutex);

	if (time(NULL) >= mysql_pool->when_check) {
		int   inter = db_pool->db_info.ping_inter > db_pool->db_info.timeout_inter
			? db_pool->db_info.timeout_inter : db_pool->db_info.ping_inter;

		db_pool->dbh_check(db_pool);
		mysql_pool->when_check = time(NULL) + inter;
	}


	if (db_pool->db_inuse >= db_pool->db_max) {
		acl_msg_warn("%s, %s(%d): all connections be used, reached db_max(%d)",
				__FILE__, myname, __LINE__, db_pool->db_max);
		RETURN (NULL);
	}

	n = acl_array_size(mysql_pool->handles);

	/* lookup mysql connection from pool */
	for (i = 0; i < n; i++) {
		mysql_handle = (ACL_DB_HANDLE_MYSQL *)
			acl_array_index(mysql_pool->handles, i);

		if (mysql_handle == NULL)
			continue;

		if (mysql_handle->handle.status == ACL_DBH_STATUS_READY) {
			mysql_handle->handle.status = ACL_DBH_STATUS_INUSE;
			db_pool->db_inuse++;
			db_pool->db_ready--;

			RETURN ((ACL_DB_HANDLE *) mysql_handle);
		} else if (mysql_handle->handle.status == ACL_DBH_STATUS_NULL
			   && mysql_handle_slot == NULL)
			mysql_handle_slot = mysql_handle;
	}

	/* create new mysql connection */

	mysql_handle = __open_mysql_handle(mysql_pool, mysql_handle_slot, &db_pool->db_info);
	if (mysql_handle == NULL)
		RETURN (NULL);

	mysql_handle->handle.status = ACL_DBH_STATUS_INUSE;
	db_pool->db_inuse++;
	
	RETURN ((ACL_DB_HANDLE *) mysql_handle);
}

/*----------------------------------------------------------------------------*/
static void __dbpool_mysql_release(ACL_DB_HANDLE *db_handle)
{
	const char *myname = "__dbpool_mysql_release";
	ACL_DB_POOL *db_pool;
	ACL_DB_POOL_MYSQL *mysql_pool;
	ACL_DB_HANDLE_MYSQL *mysql_handle;
	int   timeout_inter, ping_inter;

	mysql_handle = (ACL_DB_HANDLE_MYSQL *) db_handle;
	if (db_handle->status != ACL_DBH_STATUS_INUSE || mysql_handle->connection == NULL) {
		acl_msg_error("%s, %s(%d): status %s ACL_DBH_STATUS_INUSE, connection %s",
			__FILE__, myname, __LINE__,
			db_handle->status == ACL_DBH_STATUS_INUSE ? "=" : "!=",
			mysql_handle->connection ? "not null" : "null");
		return;
	}

	db_pool = db_handle->parent;
	timeout_inter = db_pool->db_info.timeout_inter;
	ping_inter = db_pool->db_info.ping_inter;

	if (db_pool == NULL)
		acl_msg_fatal("%s, %s(%d): db_handle's parent is null",
				__FILE__, myname, __LINE__);

	mysql_pool = (ACL_DB_POOL_MYSQL *) db_pool;
	DB_LOCK(mysql_pool->mutex);

	db_handle->status  = ACL_DBH_STATUS_READY;
	db_handle->timeout = time(NULL) + timeout_inter;
	db_handle->ping    = time(NULL) + ping_inter;

	db_pool->db_inuse--;
	db_pool->db_ready++;

	DB_UNLOCK(mysql_pool->mutex);
}
/*----------------------------------------------------------------------------*/
static void *__dbpool_mysql_export(ACL_DB_HANDLE *db_handle)
{
	ACL_DB_HANDLE_MYSQL *mysql_handle = (ACL_DB_HANDLE_MYSQL *) db_handle;

	return ((void *) mysql_handle->connection);
}
/*----------------------------------------------------------------------------*/
static void __dbpool_mysql_close(ACL_DB_HANDLE *db_handle)
{
	const char *myname = "__dbpool_mysql_close";
	ACL_DB_POOL *db_pool;
	ACL_DB_POOL_MYSQL *mysql_pool;
	ACL_DB_HANDLE_MYSQL *mysql_handle = (ACL_DB_HANDLE_MYSQL *) db_handle;

	if (db_handle->status != ACL_DBH_STATUS_INUSE || mysql_handle->connection == NULL) {
		acl_msg_error("%s, %s(%d): status %s ACL_DBH_STATUS_INUSE, connection %s",
			__FILE__, myname, __LINE__,
			db_handle->status == ACL_DBH_STATUS_INUSE ? "=" : "!=",
			mysql_handle->connection ? "not null" : "null");
		return;
	}

	db_pool = db_handle->parent;
	if (db_pool == NULL)
		acl_msg_fatal("%s, %s(%d): db_handle's parent is null",
				__FILE__, myname, __LINE__);
	mysql_pool = (ACL_DB_POOL_MYSQL *) db_pool;

	DB_LOCK(mysql_pool->mutex);

	__close_mysql_handle(mysql_handle);
	db_pool->db_inuse--;

	DB_UNLOCK(mysql_pool->mutex);
}
/*----------------------------------------------------------------------------*/
static void __dbpool_mysql_destroy(ACL_DB_POOL *db_pool)
{
	ACL_DB_POOL_MYSQL *mysql_pool;
	ACL_DB_HANDLE_MYSQL *mysql_handle;
	int   i, n;

	mysql_pool = (ACL_DB_POOL_MYSQL *) db_pool;

	n = acl_array_size(mysql_pool->handles);
	for (i = 0; i < n; i++) {
		mysql_handle = (ACL_DB_HANDLE_MYSQL *)
			acl_array_index(mysql_pool->handles, i);
		if (mysql_handle == NULL)
			continue;
		__close_mysql_handle(mysql_handle);
		acl_myfree(mysql_handle);
	}
	acl_pthread_mutex_destroy(&mysql_pool->mutex);
	acl_array_destroy(mysql_pool->handles, NULL);
	acl_myfree(mysql_pool);
	mysql_library_end();
}
/*----------------------------------------------------------------------------*/
ACL_DB_POOL *acl_dbpool_mysql_create(const ACL_DB_INFO *db_info)
{
	const char *myname = "acl_dbpool_mysql_create";
	ACL_DB_POOL_MYSQL *mysql_pool;

	mysql_pool = (ACL_DB_POOL_MYSQL *) acl_mycalloc(1, sizeof(ACL_DB_POOL_MYSQL));
	if (mysql_pool == NULL) {
		char  tbuf[256];
		acl_msg_fatal("%s, %s(%d): calloc error=%s",
			__FILE__, myname, __LINE__, acl_last_strerror(tbuf, sizeof(tbuf)));
	}

	mysql_pool->db_pool.dbh_peek    = &__dbpool_mysql_peek;
	mysql_pool->db_pool.dbh_check   = &__dbpool_mysql_check;
	mysql_pool->db_pool.dbh_release = &__dbpool_mysql_release;
	mysql_pool->db_pool.dbh_export  = &__dbpool_mysql_export;
	mysql_pool->db_pool.dbh_close   = &__dbpool_mysql_close;

	mysql_pool->db_pool.destroy     = &__dbpool_mysql_destroy;

	mysql_pool->handles  = acl_array_create(db_info->db_max);
	acl_pthread_mutex_init(&mysql_pool->mutex, NULL);

	return ((ACL_DB_POOL *) mysql_pool);
}
/*----------------------------------------------------------------------------*/

#if 0
#ifdef ACL_WINDOWS
/* XXX: just for the poor mysql */
extern void _dosmaperr(unsigned long error);
void _dosmaperr(unsigned long error)
{

}
#endif /* ACL_WINDOWS */
#endif

#endif /* HAS_MYSQL */
#endif /* ACL_CLIENT_ONLY */
