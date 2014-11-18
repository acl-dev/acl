#ifndef	__ACL_MASTER_PATHNAME_INCLUDE_H__
#define	__ACL_MASTER_PATHNAME_INCLUDE_H__

#ifdef	__cplusplus
extern "C" {
#endif

#include "stdlib/acl_define.h"
#ifdef ACL_UNIX

extern char   *acl_master_pathname(const char *queue_path,
	const char *service_class, const char *service_name);

#endif /* ACL_UNIX */

#ifdef	__cplusplus
}
#endif

#endif

