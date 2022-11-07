#ifndef	__ACL_FIBER_MUTEX_INCLUDE_H__
#define	__ACL_FIBER_MUTEX_INCLUDE_H__

#include "fiber_define.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ACL_FIBER_MUTEX ACL_FIBER_MUTEX;

ACL_FIBER_MUTEX *acl_fiber_mutex_create(unsigned flag);
void acl_fiber_mutex_free(ACL_FIBER_MUTEX *mutex);
int acl_fiber_mutex_lock(ACL_FIBER_MUTEX *mutex);
int acl_fiber_mutex_unlock(ACL_FIBER_MUTEX *mutex);

#ifdef __cplusplus
}
#endif

#endif
