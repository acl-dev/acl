#ifndef	ACL_YQUEUE_INCLUDE_H
#define	ACL_YQUEUE_INCLUDE_H

#ifdef __cplusplus
extern "C" 
{
#endif

#include "acl_define.h"

typedef struct ACL_YQUEUE ACL_YQUEUE;

ACL_API ACL_YQUEUE* acl_yqueue_new(void);
ACL_API void acl_yqueue_free(ACL_YQUEUE *yqueue, void(*free_fn)(void*));
ACL_API void **acl_yqueue_front(ACL_YQUEUE *yqueue);
ACL_API void **acl_yqueue_back(ACL_YQUEUE *yqueue);
ACL_API void acl_yqueue_push(ACL_YQUEUE *yqueue);
ACL_API void acl_yqueue_pop(ACL_YQUEUE *yqueue);

#ifdef __cplusplus
}
#endif

#endif
