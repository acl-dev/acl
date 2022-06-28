#ifndef	__ACL_FDMAP_INCLUDE_H__
#define	__ACL_FDMAP_INCLUDE_H__

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct ACL_FD_MAP ACL_FD_MAP;

extern ACL_FD_MAP *acl_fdmap_create(int maxfd);
extern void acl_fdmap_add(ACL_FD_MAP *map, int fd, void *ctx);
extern void acl_fdmap_del(ACL_FD_MAP *map, int fd);
extern void *acl_fdmap_ctx(ACL_FD_MAP *map, int fd);
extern void acl_fdmap_free(ACL_FD_MAP *map);

#ifdef	__cplusplus
}
#endif

#endif

