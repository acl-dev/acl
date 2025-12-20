#ifndef ACL_DBERR_INCLUDE_H
#define ACL_DBERR_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#define ACL_DB_OK		0       /* 成功 */
#define ACL_DB_ERR_CALLBACK	1       /* 用户的回调函数返回失败 */
#define ACL_DB_ERR_SELECT	100     /* 用户查询语句失败 */
#define ACL_DB_ERR_UPDATE	101     /* 用户更新语句失败 */
#define ACL_DB_ERR_EMPTY	102     /* 查询结果为空 */
#define ACL_DB_ERR_STORE	103     /* 查询语句获得的结果后存储于本地时失败 */
#define ACL_DB_ERR_AFFECTED	104     /* 更新语句对数据库无实际更新操作 */
#define	ACL_DB_ERR_ALLOC	105     /* 内部分配内存失败 */

#ifdef __cplusplus
}
#endif

#endif

