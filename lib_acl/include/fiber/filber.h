#ifndef ACL_FIBER_INCLUDE_H
#define ACL_FIBER_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "stdlib/acl_define.h"

typedef struct FIBER {
	unsigned int id;
} FIBER;

ACL_API FIBER *acl_fiber_create(void);

#ifdef  __cplusplus
}
#endif

#endif
