#ifndef	ACL_DBUF_POOL_INCLUDE_H
#define	ACL_DBUF_POOL_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct ACL_DBUF_POOL ACL_DBUF_POOL;

/* public */
ACL_API ACL_DBUF_POOL *acl_dbuf_pool_create(int block_size);
ACL_API void acl_dbuf_pool_destroy(ACL_DBUF_POOL *pool);
ACL_API void acl_dbuf_pool_free(ACL_DBUF_POOL *pool, void *ptr, int length);
ACL_API void *acl_dbuf_pool_alloc(ACL_DBUF_POOL *pool, int length);
ACL_API void *acl_dbuf_pool_calloc(ACL_DBUF_POOL *pool, int length);
ACL_API void *acl_dbuf_pool_memdup(ACL_DBUF_POOL *pool, const void *s, size_t len);
ACL_API char *acl_dbuf_pool_strdup(ACL_DBUF_POOL *pool, const char *s);

/* private */
ACL_API void acl_dbuf_pool_test(size_t max);

#ifdef	__cplusplus
}
#endif

#endif
