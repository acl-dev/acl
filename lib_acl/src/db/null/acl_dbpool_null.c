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
#include "net/acl_vstream_net.h"
#include "db/acl_dbpool.h"

#endif

#ifndef ACL_CLIENT_ONLY

#include "acl_dbnull.h"
#include "acl_dbpool_null.h"

typedef struct ACL_DB_HANDLE_NULL {
	ACL_DB_HANDLE handle;
	ACL_VSTREAM  *connection;
} ACL_DB_HANDLE_NULL;

typedef struct ACL_DB_POOL_NULL {
	ACL_DB_POOL db_pool;

	acl_pthread_mutex_t mutex;
	ACL_ARRAY *handles;
	time_t when_check;
} ACL_DB_POOL_NULL;

#define	DB_LOCK(mlock) { acl_pthread_mutex_lock(&mlock); }
#define	DB_UNLOCK(mlock) { acl_pthread_mutex_unlock(&mlock); }

/*----------------------------------------------------------------------------*/
static ACL_DB_HANDLE_NULL *__new_null_handle(void)
{
	ACL_DB_HANDLE_NULL *handle;

	handle = (ACL_DB_HANDLE_NULL *)
		acl_mycalloc(1, sizeof(ACL_DB_HANDLE_NULL));

	handle->handle.sql_results = acl_dbnull_results;
	handle->handle.sql_result  = acl_dbnull_result;
	handle->handle.sql_update  = acl_dbnull_update;

	return (handle);
}

static ACL_DB_HANDLE_NULL *__open_handle(ACL_DB_HANDLE_NULL *handle,
					ACL_DB_INFO *db_info)
{
	const char *myname = "__open_handle";
	int   reuse_flag = 0;

	if (handle == NULL)
		handle = __new_null_handle();
	else
		reuse_flag = 1;

	handle->connection = acl_vstream_connect(db_info->db_addr, ACL_BLOCKING,
		db_info->conn_timeout, db_info->rw_timeout, db_info->buf_size);

	if (handle->connection == NULL) {
		char  ebuf[256];
		acl_msg_error("%s, %s(%d): connect addr(%s) error(%s)",
			__FILE__, myname, __LINE__, db_info->db_addr,
			acl_last_strerror(ebuf, sizeof(ebuf)));
		if (!reuse_flag)
			acl_myfree(handle);
		return (NULL);
	}

	handle->handle.status  = ACL_DBH_STATUS_READY;
	handle->handle.timeout = time(NULL) + db_info->timeout_inter;
	handle->handle.ping    = time(NULL) + db_info->ping_inter;

	return (handle);
}

/*----------------------------------------------------------------------------*/
static void __close_handle(ACL_DB_HANDLE_NULL *handle)
{
	if (handle->connection == NULL)
		return;
	acl_vstream_close(handle->connection);
	handle->connection = NULL;
	handle->handle.status = ACL_DBH_STATUS_NULL;
	handle->handle.timeout = 0;
	handle->handle.ping = 0;
}

/*----------------------------------------------------------------------------*/
static int __dbpool_ping_default(ACL_DB_HANDLE *db_handle)
{
	ACL_DB_HANDLE_NULL *handle = (ACL_DB_HANDLE_NULL*) db_handle;

	if (acl_vstream_probe_status(handle->connection) == 0)
		return (0);
	return (-1);
}

/*----------------------------------------------------------------------------*/
static void __dbpool_check(ACL_DB_POOL *db_pool)
{
	ACL_DB_POOL_NULL *pool = (ACL_DB_POOL_NULL*) db_pool;
	ACL_DB_HANDLE_NULL *handle;
	int  i, n, ping_inter;
	time_t now = time(NULL);

	ping_inter = pool->db_pool.db_info.ping_inter;

	n = acl_array_size(pool->handles);

	for (i = 0; i < n; i++) {
		handle = (ACL_DB_HANDLE_NULL *) acl_array_index(pool->handles, i);
		if (handle == NULL)
			continue;
		if (handle->handle.status != ACL_DBH_STATUS_READY)
			continue;

		/* if the connecion is idle timeout ? */
		if (now > handle->handle.timeout) {
			__close_handle(handle);
			pool->db_pool.db_ready--;
			continue;
		}

		if (pool->db_pool.dbh_ping == NULL)
			continue;

		/* has the ping time reached ? */
		if (now <= handle->handle.ping) 
			continue;

		if (pool->db_pool.dbh_ping((ACL_DB_HANDLE*) handle) == 0) {
			handle->handle.ping = time(NULL) + ping_inter;
		} else {
			__close_handle(handle);
			pool->db_pool.db_ready--;
		}
	}
}

/*----------------------------------------------------------------------------*/
static ACL_DB_HANDLE *__dbpool_peek(ACL_DB_POOL *db_pool)
{
	const char *myname = "__dbpool_peek";
	ACL_DB_POOL_NULL *null_pool = (ACL_DB_POOL_NULL *) db_pool;
	ACL_DB_HANDLE_NULL *handle, *handle_slot = NULL;
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
	DB_UNLOCK(null_pool->mutex); \
	return (_x_); \
} while (0)

	DB_LOCK(null_pool->mutex);

	if (time(NULL) >= null_pool->when_check) {
		int   inter = db_pool->db_info.ping_inter > db_pool->db_info.timeout_inter
			? db_pool->db_info.timeout_inter : db_pool->db_info.ping_inter;

		db_pool->dbh_check(db_pool);
		null_pool->when_check = time(NULL) + inter;
	}

	if (db_pool->db_inuse >= db_pool->db_max) {
		acl_msg_warn("%s, %s(%d): all connections be used, reached db_max(%d)",
			__FILE__, myname, __LINE__, db_pool->db_max);
		RETURN (NULL);
	}

	n = acl_array_size(null_pool->handles);

	/* lookup null connection from pool */
	for (i = 0; i < n; i++) {
		handle = (ACL_DB_HANDLE_NULL *)
			acl_array_index(null_pool->handles, i);

		if (handle == NULL)
			continue;

		if (handle->handle.status == ACL_DBH_STATUS_READY) {
			handle->handle.status = ACL_DBH_STATUS_INUSE;
			db_pool->db_inuse++;
			db_pool->db_ready--;

			RETURN ((ACL_DB_HANDLE *) handle);
		} else if (handle->handle.status == ACL_DBH_STATUS_NULL
			   && handle_slot == NULL)
			handle_slot = handle;
	}

	/* create new connection */

	handle = __open_handle(handle_slot, &db_pool->db_info);
	if (handle == NULL)
		RETURN (NULL);

	if (handle_slot == NULL) {
		if (acl_array_append(null_pool->handles, (void *) handle) < 0) {
			char  tbuf[256];
			acl_msg_fatal("%s, %s(%d): append to handles error(%s)",
				__FILE__, myname, __LINE__,
				acl_last_strerror(tbuf, sizeof(tbuf)));
		}
		handle->handle.parent = &null_pool->db_pool;
	}
	handle->handle.status = ACL_DBH_STATUS_INUSE;
	db_pool->db_inuse++;
	
	RETURN ((ACL_DB_HANDLE *) handle);
#ifdef	ACL_BCB_COMPILER
	return (NULL);
#endif
}

/*----------------------------------------------------------------------------*/
static void __dbpool_release(ACL_DB_HANDLE *db_handle)
{
	const char *myname = "__dbpool_release";
	ACL_DB_POOL *db_pool;
	ACL_DB_POOL_NULL *null_pool;
	ACL_DB_HANDLE_NULL *handle;
	int   timeout_inter, ping_inter;

	handle = (ACL_DB_HANDLE_NULL *) db_handle;
	if (db_handle->status != ACL_DBH_STATUS_INUSE || handle->connection == NULL) {
		acl_msg_error("%s, %s(%d): status %s ACL_DBH_STATUS_INUSE, connection %s",
			__FILE__, myname, __LINE__,
			db_handle->status == ACL_DBH_STATUS_INUSE ? "=" : "!=",
			handle->connection ? "not null" : "null");
		return;
	}

	db_pool = db_handle->parent;
	timeout_inter = db_pool->db_info.timeout_inter;
	ping_inter = db_pool->db_info.ping_inter;

	if (db_pool == NULL)
		acl_msg_fatal("%s, %s(%d): db_handle's parent is null",
			__FILE__, myname, __LINE__);

	null_pool = (ACL_DB_POOL_NULL *) db_pool;
	DB_LOCK(null_pool->mutex);

	db_handle->status  = ACL_DBH_STATUS_READY;
	db_handle->timeout = time(NULL) + timeout_inter;
	db_handle->ping    = time(NULL) + ping_inter;

	db_pool->db_inuse--;
	db_pool->db_ready++;

	DB_UNLOCK(null_pool->mutex);
}
/*----------------------------------------------------------------------------*/
static void *__dbpool_export(ACL_DB_HANDLE *db_handle)
{
	ACL_DB_HANDLE_NULL *handle = (ACL_DB_HANDLE_NULL *) db_handle;

	return ((void *) handle->connection);
}
/*----------------------------------------------------------------------------*/
static void __dbpool_close(ACL_DB_HANDLE *db_handle)
{
	const char *myname = "__dbpool_close";
	ACL_DB_POOL *db_pool;
	ACL_DB_POOL_NULL *null_pool;
	ACL_DB_HANDLE_NULL *handle = (ACL_DB_HANDLE_NULL *) db_handle;

	if (db_handle->status != ACL_DBH_STATUS_INUSE || handle->connection == NULL) {
		acl_msg_error("%s, %s(%d): status %s ACL_DBH_STATUS_INUSE, connection %s",
			__FILE__, myname, __LINE__,
			db_handle->status == ACL_DBH_STATUS_INUSE ? "=" : "!=",
			handle->connection ? "not null" : "null");
		return;
	}

	db_pool = db_handle->parent;
	if (db_pool == NULL)
		acl_msg_fatal("%s, %s(%d): db_handle's parent is null",
			__FILE__, myname, __LINE__);
	null_pool = (ACL_DB_POOL_NULL *) db_pool;

	DB_LOCK(null_pool->mutex);

	__close_handle(handle);
	db_pool->db_inuse--;

	DB_UNLOCK(null_pool->mutex);
}
/*----------------------------------------------------------------------------*/
static void __dbpool_destroy(ACL_DB_POOL *db_pool)
{
	ACL_DB_POOL_NULL *null_pool;
	ACL_DB_HANDLE_NULL *handle;
	int   i, n;

	null_pool = (ACL_DB_POOL_NULL *) db_pool;

	n = acl_array_size(null_pool->handles);
	for (i = 0; i < n; i++) {
		handle = (ACL_DB_HANDLE_NULL *)
			acl_array_index(null_pool->handles, i);
		if (handle == NULL)
			continue;
		__close_handle(handle);
		acl_myfree(handle);
	}
	acl_pthread_mutex_destroy(&null_pool->mutex);
	acl_myfree(null_pool);
}
/*----------------------------------------------------------------------------*/
ACL_DB_POOL *acl_dbpool_null_create(const ACL_DB_INFO *db_info)
{
	const char *myname = "acl_dbpool_null_create";
	ACL_DB_POOL_NULL *null_pool;

	null_pool = (ACL_DB_POOL_NULL *) acl_mycalloc(1, sizeof(ACL_DB_POOL_NULL));
	if (null_pool == NULL) {
		char  tbuf[256];
		acl_msg_fatal("%s, %s(%d): calloc error=%s",
			__FILE__, myname, __LINE__, acl_last_strerror(tbuf, sizeof(tbuf)));
	}

	null_pool->db_pool.dbh_peek    = &__dbpool_peek;
	null_pool->db_pool.dbh_check   = &__dbpool_check;
	null_pool->db_pool.dbh_release = &__dbpool_release;
	null_pool->db_pool.dbh_export  = &__dbpool_export;
	null_pool->db_pool.dbh_close   = &__dbpool_close;
	null_pool->db_pool.dbh_ping    = &__dbpool_ping_default;

	null_pool->db_pool.destroy     = &__dbpool_destroy;

	null_pool->handles  = acl_array_create(db_info->db_max);
	acl_pthread_mutex_init(&null_pool->mutex, NULL);

	return ((ACL_DB_POOL *) null_pool);
}
/*----------------------------------------------------------------------------*/

#endif /* ACL_CLIENT_ONLY */
