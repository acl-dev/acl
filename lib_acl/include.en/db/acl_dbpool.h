#ifndef	ACL_DBPOOL_INCLUDE_H
#define	ACL_DBPOOL_INCLUDE_H

#ifndef ACL_CLIENT_ONLY

#ifdef	__cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"
#include <time.h>
#include "../stdlib/acl_stdlib.h"

#define	ACL_DB_DEBUG_MEM	(1<<0)

typedef struct ACL_DB_HANDLE ACL_DB_HANDLE;
typedef struct ACL_SQL_RES ACL_SQL_RES;
typedef struct ACL_DB_POOL ACL_DB_POOL;

typedef struct ACL_DB_INFO {
	int   db_max;		/* maximum number of connections in the pool */
	char  db_addr[256];	/* database server address */
	char  db_name[256];	/* database name */
	char  db_user[256];	/* database account */
	char  db_pass[256];	/* account password */
	unsigned long db_flags;	/* (mysql) connection flag bits */
	int   ping_inter;	/* interval for probing database connection */
	int   timeout_inter;	/* timeout interval for database connection */
	int   auto_commit;	/* (mysql) whether to enable auto-commit mode */
	int   conn_timeout;	/* (mysql/null) connection timeout */
	int   rw_timeout;	/* (mysql/null) IO read/write timeout */
	int   buf_size;		/* (null) IO buffer size */
	int   debug_flag;	/* debug flag bits */

	/* User-defined callback functions before/after connecting to the
	 * database, these can be NULL, if db_before_connect/db_after_connect
	 * returns < 0 it will cause acl_dbpool_peek to return NULL
	 */
	int  (*db_before_connect)(ACL_DB_HANDLE* db_handle, void *ctx);
	int  (*db_after_connect)(ACL_DB_HANDLE* db_handle, void *ctx);

	void *ctx;		/* parameter for db_before_connect/db_after_connect */
} ACL_DB_INFO;

struct ACL_DB_HANDLE {
#define	ACL_DBH_STATUS_NULL	0
#define	ACL_DBH_STATUS_READY	1
#define	ACL_DBH_STATUS_INUSE	2
	int    status;
	time_t timeout;
	time_t ping;

	ACL_DB_POOL *parent;

	int (*sql_results)(ACL_DB_HANDLE *handle, const char *sql, int  *error,
		int (*walk_fn)(const void** result_row, void *arg), void *arg);
	int (*sql_result)(ACL_DB_HANDLE *handle, const char *sql, int  *error,
		int (*callback)(const void** result_row, void *arg), void *arg);
	int (*sql_update)(ACL_DB_HANDLE *handle, const char *sql, int  *error);

	ACL_SQL_RES *(*sql_select)(ACL_DB_HANDLE *handle, const char *sql, int *error);
	void (*free_result)(ACL_SQL_RES *res);
};

struct ACL_SQL_RES {
	void *res;
	int   num;

        /* for acl_iterator */

	/* Get the head element of the container */
	const void *(*iter_head)(ACL_ITER*, struct ACL_SQL_RES*);
	/* Get the next element of the container */
	const void *(*iter_next)(ACL_ITER*, struct ACL_SQL_RES*);
};

struct ACL_DB_POOL {
	ACL_DB_INFO db_info;

	ACL_DB_HANDLE *(*dbh_peek)(ACL_DB_POOL *);
	void  (*dbh_check)(ACL_DB_POOL *);
	void  (*dbh_release)(ACL_DB_HANDLE *);
	void *(*dbh_export)(ACL_DB_HANDLE *);
	void  (*dbh_close)(ACL_DB_HANDLE *);
	int   (*dbh_ping)(ACL_DB_HANDLE *);

	void  (*destroy)(ACL_DB_POOL *);

	int  db_max;
	int  db_ready;
	int  db_inuse;
};

/*------------------------------------------------------------------*/
#define	ACL_DB_AUTO_COMMIT(_db_pool_) (_db_pool_ ? _db_pool_->db_info.auto_commit : 0)

/* in acl_dbpool.c */
/**
 * Create a database connection pool.
 * @param db_type {const char*} database type, currently only supports mysql
 * @param db_info {const ACL_DB_INFO*} structure containing all necessary
 *  database connection information
 * @return {ACL_DB_POOL*} a database connection pool
 */
ACL_API ACL_DB_POOL *acl_dbpool_create(const char *db_type, const ACL_DB_INFO *db_info);

/**
 * Destroy a database connection pool.
 * @param db_pool database connection pool object
 */
ACL_API void acl_dbpool_destroy(ACL_DB_POOL *db_pool);

/**
 * Get a connection handle from the connection pool.
 * @param db_pool {ACL_DB_POOL*} database connection pool object
 * @return {ACL_DB_HANDLE*} database connection handle, returns NULL
 *  if the connection pool is full
 */
ACL_API ACL_DB_HANDLE *acl_dbpool_peek(ACL_DB_POOL *db_pool);

/**
 * Manually check each connection in the connection pool. A connection pool
 * internally periodically checks each connection; you can also manually force
 * a check through this function.
 * @param db_pool {ACL_DB_POOL*} database connection pool object
 */
ACL_API void acl_dbpool_check(ACL_DB_POOL *db_pool);

/**
 * Release the database connection handle back to the database connection pool.
 * @param db_handle {ACL_DB_HANDLE*} database connection handle
 */
ACL_API void acl_dbpool_release(ACL_DB_HANDLE *db_handle);
/**
 * Convert the database handle to the actual database connection handle.
 * @param db_handle {ACL_DB_HANDLE*} database connection handle
 * @return void * Users need to cast it to their own database connection type
 */
ACL_API void *acl_dbpool_export(ACL_DB_HANDLE *db_handle);
/**
 * When users detect that the database connection pool is abnormal, they can
 * force close the connection through this interface.
 * @param db_handle {ACL_DB_HANDLE*} database connection handle
 */
ACL_API void acl_dbpool_close(ACL_DB_HANDLE *db_handle);

/**
 * Set the timeout PING function for the connection pool. If
 * this value is not set, the internal default method will
 * be used.
 * @param db_pool {ACL_DB_POOL*} database connection pool
 *  object
 * @param ping_fn {int (*)(ACL_DB_HANDLE*)} function pointer
 *  for probing connection status
 */
ACL_API void acl_dbpool_set_ping(ACL_DB_POOL *db_pool, int (*ping_fn)(ACL_DB_HANDLE*));
/*------------------------------------------------------------------*/

#ifdef	__cplusplus
}
#endif

#endif /* ACL_CLIENT_ONLY */
#endif
