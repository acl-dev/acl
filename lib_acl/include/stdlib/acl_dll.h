#ifndef	ACL_DLL_INCLUDE_H
#define	ACL_DLL_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include "acl_vstream.h"
#include "acl_debug_malloc.h"
#include "acl_mem_slice.h"

typedef struct ACL_DLL_ENV {
	ACL_VSTREAM *logfp;
	ACL_DEBUG_MEM *mmd;
	ACL_MEM_SLICE *mem_slice;
} ACL_DLL_ENV;

ACL_API ACL_DLL_HANDLE acl_dlopen(const char *dlname);
ACL_API void acl_dlclose(ACL_DLL_HANDLE handle);
ACL_API ACL_DLL_FARPROC acl_dlsym(void *handle, const char *name);
ACL_API const char *acl_dlerror(void);

#ifdef	__cplusplus
}
#endif

#endif
