#ifndef ACL_MBOX_INCLUDE_H
#define ACL_MBOX_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "acl_define.h"

typedef struct ACL_MBOX ACL_MBOX;

ACL_API ACL_MBOX *acl_mbox_create(void);
ACL_API void acl_mbox_free(ACL_MBOX *mbox, void (*free_fn)(void*));
ACL_API int acl_mbox_send(ACL_MBOX *mbox, void *msg);
ACL_API void *acl_mbox_read(ACL_MBOX *mbox, int timeout, int *success);
ACL_API size_t acl_mbox_nsend(ACL_MBOX *mbox);
ACL_API size_t acl_mbox_nread(ACL_MBOX *mbox);

#ifdef __cplusplus
}
#endif

#endif
