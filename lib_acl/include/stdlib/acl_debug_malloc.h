#ifndef ACL_DEBUG_MALLOC_INCLUDE_H
#define ACL_DEBUG_MALLOC_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "acl_define.h"

typedef struct ACL_DEBUG_MEM ACL_DEBUG_MEM;

ACL_API void acl_debug_dump(void);
ACL_API ACL_DEBUG_MEM *acl_debug_malloc_init(ACL_DEBUG_MEM *debug_mem_ptr,
	const char* dump_file);

#ifdef __cplusplus
}
#endif

#endif
