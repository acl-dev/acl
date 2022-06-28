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
	int   db_max;		/* 连接池最大连接数 */
	char  db_addr[256];	/* 数据库服务地址 */
	char  db_name[256];	/* 数据库名称 */
	char  db_user[256];	/* 数据库帐号 */
	char  db_pass[256];	/* 帐号密码 */
	unsigned long db_flags;	/* (mysql) 连接标志位 */
	int   ping_inter;	/* 探测数据库连接的时间间隔 */
	int   timeout_inter;	/* 数据库连接的空闲超时时间 */
	int   auto_commit;	/* (mysql) 是否启用自动提交过程 */
	int   conn_timeout;	/* (mysql/null) 连接超时时间 */
	int   rw_timeout;	/* (mysql/null) IO读写超时时间 */
	int   buf_size;		/* (null) IO缓冲区大小 */
	int   debug_flag;	/* 调试标志位 */

	/* 在真实连接数据库之前/后调用用户设置的回调函数, 此项可以设为 NULL,
	 * 如果 db_before_connect/db_after_connect 返回 < 0 则会导致
	 * acl_dbpool_peek 返回 NULL
	 */
	int  (*db_before_connect)(ACL_DB_HANDLE* db_handle, void *ctx);
	int  (*db_after_connect)(ACL_DB_HANDLE* db_handle, void *ctx);

	void *ctx;		/* db_before_connect/db_after_connect 参数之一 */
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

	/* 取迭代器头函数 */
	const void *(*iter_head)(ACL_ITER*, struct ACL_SQL_RES*);
	/* 取迭代器下一个函数 */
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

/*----------------------------------------------------------------------------*/
#define	ACL_DB_AUTO_COMMIT(_db_pool_) (_db_pool_ ? _db_pool_->db_info.auto_commit : 0)

/* in acl_dbpool.c */
/**
 * 创建一个数据库连接池
 * @param db_type {const char*} 数据库类型名, 目前仅支持 mysql
 * @param db_info {const ACL_DB_INFO*} 记录着有关连接数据所需要的信息
 * @return {ACL_DB_POOL*} 一个数据库连接池
 */
ACL_API ACL_DB_POOL *acl_dbpool_create(const char *db_type, const ACL_DB_INFO *db_info);

/**
 * 销毁一个数据库连接池
 * @param db_pool 数据库连接池句柄
 */
ACL_API void acl_dbpool_destroy(ACL_DB_POOL *db_pool);

/**
 * 从连接池中获取一个连接句柄
 * @param db_pool {ACL_DB_POOL*} 数据库连接池句柄
 * @return {ACL_DB_HANDLE*} 数据库连接句柄，如果为空则表示出错或连接池已满
 */
ACL_API ACL_DB_HANDLE *acl_dbpool_peek(ACL_DB_POOL *db_pool);

/**
 * 手工检查连接池的每个连接?一般连接池内部会定期检查每个连接，
 * 也可以通过此函数手工进行强制检查
 * @param db_pool {ACL_DB_POOL*} 数据库连接池句柄
 */
ACL_API void acl_dbpool_check(ACL_DB_POOL *db_pool);

/**
 * 将数据库连接句柄释放给数据库连接池
 * @param db_handle {ACL_DB_HANDLE*} 数据库连接句柄
 */
ACL_API void acl_dbpool_release(ACL_DB_HANDLE *db_handle);
/**
 * 将数据库连接转换为实际的数据库连接句柄
 * @param db_handle {ACL_DB_HANDLE*} 数据库连接句柄
 * @return void * 使用者需要将其强制转换为自己所用的数据库连接引擎
 */
ACL_API void *acl_dbpool_export(ACL_DB_HANDLE *db_handle);
/**
 * 当使用者自己检测到该数据库连接出错时，可以通过此接口强行关闭该连接
 * @param db_handle {ACL_DB_HANDLE*} 数据库连接句柄
 */
ACL_API void acl_dbpool_close(ACL_DB_HANDLE *db_handle);

/**
 * 设置连接池的定时PING处理函数，如果不设置此值则内部采用缺省方式
 * @param db_pool {ACL_DB_POOL*} 数据库连接池句柄
 * @param ping_fn {int (*)(ACL_DB_HANDLE*)} 探测连接状态的函数指针
 */
ACL_API void acl_dbpool_set_ping(ACL_DB_POOL *db_pool, int (*ping_fn)(ACL_DB_HANDLE*));
/*----------------------------------------------------------------------------*/

#ifdef	__cplusplus
}
#endif

#endif /* ACL_CLIENT_ONLY */
#endif
