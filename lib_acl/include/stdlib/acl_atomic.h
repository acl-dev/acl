#ifndef ACL_ATOMIC_INCLUDE_H
#define ACL_ATOMIC_INCLUDE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "acl_define.h"

typedef struct ACL_ATOMIC ACL_ATOMIC;

ACL_API ACL_ATOMIC *acl_atomic_new(void);
ACL_API void  acl_atomic_free(ACL_ATOMIC *self);
ACL_API void  acl_atomic_set(ACL_ATOMIC *self, void *value);
ACL_API void *acl_atomic_cas(ACL_ATOMIC *self, void *cmp, void *value);
ACL_API void *acl_atomic_xchg(ACL_ATOMIC *self, void *value);

#ifdef __cplusplus
}
#endif

#endif
