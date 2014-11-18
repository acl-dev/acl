#ifndef	__ACL_TRACE_INCLUDE_H__
#define	__ACL_TRACE_INCLUDE_H__

#ifdef	__cplusplus
extern "C" {
#endif

#include "../acl_define.h"
#ifdef ACL_UNIX

void acl_dump_trace(const char *filepath);

#endif /* ACL_UNIX */

#ifdef	__cplusplus
}
#endif

#endif
