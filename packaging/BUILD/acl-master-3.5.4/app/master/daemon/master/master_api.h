#ifndef __MASTER_API_INCLUDE_H__
#define __MASTER_API_INCLUDE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "master.h"

extern ACL_MASTER_SERV *acl_master_lookup(const char *path);
extern ACL_MASTER_SERV *acl_master_start(const char *path, int *nchilden,
	int *nsignaled, STATUS_CALLBACK callback, void *ctx, const char *ext);
extern ACL_MASTER_SERV *acl_master_restart(const char *path, int *nchilden,
	int *nsignaled, STATUS_CALLBACK callback, void *ctx, const char *ext);
extern int acl_master_kill(const char *path);
extern int acl_master_stop(const char *path);
extern int acl_master_reload(const char *path, int *nchildren, int *nsignaled,
	STATUS_CALLBACK callback, void *ctx);
extern void acl_master_callback_clean(const char *path);


#ifdef __cplusplus
}
#endif

#endif
