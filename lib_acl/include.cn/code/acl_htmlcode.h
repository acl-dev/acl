#ifndef	ACL_HTMLCODE_INCLUDE_H
#define	ACL_HTMLCODE_INCLUDE_H

#ifndef ACL_CLIENT_ONLY

#ifdef	__cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"
#include "../stdlib/acl_vstring.h"

ACL_API int acl_html_encode(const char *in, ACL_VSTRING *out);
ACL_API int acl_html_decode(const char *in, ACL_VSTRING *out);

#ifdef	__cplusplus
}
#endif

#endif /* ACL_CLIENT_ONLY */
#endif
