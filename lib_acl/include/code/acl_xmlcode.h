#ifndef	__ACL_XMLCODE_INCLUDE_H__
#define	__ACL_XMLCODE_INCLUDE_H__

#ifdef	__cplusplus
extern "C" {
#endif

#include "stdlib/acl_define.h"
#include "stdlib/acl_vstring.h"

ACL_API int acl_xml_encode(const char *in, ACL_VSTRING *out);
ACL_API int acl_xml_decode(const char *in, ACL_VSTRING *out);
ACL_API int acl_xml_encode2(const char **in, int *ilen, char **out, int *olen);
ACL_API int acl_xml_decode2(const char *in, char **out, int *size);

#ifdef	__cplusplus
}
#endif

#endif
