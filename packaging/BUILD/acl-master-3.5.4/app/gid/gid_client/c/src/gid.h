#ifndef	__GID_INCLUDE_H__
#define	__GID_INCLUDE_H__

#include "lib_acl.h"

acl_int64 gid_cmdline_next(ACL_VSTREAM *client, const char *tag, int *errnum);
acl_int64 gid_json_next(ACL_VSTREAM *client, const char *tag, int *errnum);
acl_int64 gid_xml_next(ACL_VSTREAM *client, const char *tag, int *errnum);

#endif
