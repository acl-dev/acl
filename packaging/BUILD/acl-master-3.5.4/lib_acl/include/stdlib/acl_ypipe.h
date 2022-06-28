#ifndef	ACL_YPIPI_CINLUDE_H
#define	ACL_YPIPI_CINLUDE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "acl_define.h"

typedef struct ACL_YPIPE ACL_YPIPE;

ACL_API ACL_YPIPE *acl_ypipe_new(void);
ACL_API int   acl_ypipe_check_read(ACL_YPIPE *self);
ACL_API void *acl_ypipe_read(ACL_YPIPE *self);
ACL_API void  acl_ypipe_write(ACL_YPIPE *self, void *data);
ACL_API int   acl_ypipe_flush(ACL_YPIPE *self);
ACL_API void  acl_ypipe_free(ACL_YPIPE *self, void(*free_fun)(void*));

#ifdef __cplusplus
}
#endif
#endif
