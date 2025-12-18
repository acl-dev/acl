#ifndef ACL_DBERR_INCLUDE_H
#define ACL_DBERR_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#define ACL_DB_OK		0       /* Success */
#define ACL_DB_ERR_CALLBACK	1       /* User callback function execution failed */
#define ACL_DB_ERR_SELECT	100     /* User query operation failed */
#define ACL_DB_ERR_UPDATE	101     /* User update operation failed */
#define ACL_DB_ERR_EMPTY	102     /* Query result is empty */
#define ACL_DB_ERR_STORE	103     /* Failed when storing query result in buffer */
#define ACL_DB_ERR_AFFECTED	104     /* Update operation did not actually update database */
#define	ACL_DB_ERR_ALLOC	105     /* Internal memory allocation failed */

#ifdef __cplusplus
}
#endif

#endif
