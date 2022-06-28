
#ifndef	__ACL_DBPOOL_MYSQL_INCLUDE_H__
#define	__ACL_DBPOOL_MYSQL_INCLUDE_H__

#include "stdlib/acl_define.h"
#include "db/acl_dbpool.h"

#ifndef ACL_CLIENT_ONLY

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef	HAS_MYSQL

/* in acl_dbpool_mysql.c */

/**
 * 创建一个 mysql 类型的数据库连接池
 * @param db_info 记录着有关连接数据所需要的信息
 * @return DB_POOL * 返回一个能用的连接池句柄
 */
ACL_DB_POOL *acl_dbpool_mysql_create(const ACL_DB_INFO *db_info);

/**
 * 重新再打开 mysql 连接, 如果重新打开成功, 则同时关闭旧连接, 如果打开
 * 失败, 则保留原连接于 handle 中
 * @param handle {ACL_DB_HANDLE*}
 * @return {int} 0: 表示重新打开成功
 */
int sane_mysql_reopen(ACL_DB_HANDLE *handle);

#endif /* HAS_MYSQL */

#ifdef	__cplusplus
}
#endif

#endif /* ACL_CLIENT_ONLY */
#endif
