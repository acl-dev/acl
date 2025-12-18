#ifndef	__ACL_XMLCODE_INCLUDE_H__
#define	__ACL_XMLCODE_INCLUDE_H__

#include "../stdlib/acl_define.h"
#include "../stdlib/acl_vstring.h"

#ifdef	__cplusplus
extern "C" {
#endif

ACL_API int acl_xml_encode(const char *in, ACL_VSTRING *out);
ACL_API int acl_xml_decode(const char *in, ACL_VSTRING *out);

#ifndef ACL_CLIENT_ONLY

/**
 * XML string encoding function.
 * @param in {const char**} Address of source string address.
 *  After return, this address records the address of unprocessed
 *  data. If the output buffer is large enough, this address
 *  points to the position after the source end.
 * @param ilen {size_t} Source data total length
 * @param out {char*} Output buffer, stores conversion result
 * @param olen {size_t} Output buffer size
 * @return {size_t} Converted data length stored in output buffer
 *  (this length may be greater than source data length). If
 *  output buffer is not NULL:
 *  1) == 0 when return value is 0
 *  2) == 1 when return value is 0, and the last byte must be '\0'
 *  3) > 1 when the last byte must be '\0', and return value > 0
 *     (does not include '\0')
 *  Note:
 *  1) After function returns, in's address will change, pointing
 *     to the next unprocessed address
 *  2) Before calling the function, you should save in's address
 *     first, and calculate remaining length of in's unprocessed
 *     data:
 *     ilen -= in_saved - in;
 *  3) Although when olen > 0, internally automatically appends
 *     '\0' to out's end, the returned data length does not
 *     include '\0'
 */
ACL_API size_t acl_xml_encode2(const char** in, size_t ilen,
		char* out, size_t olen);

ACL_API const char *acl_xml_decode2(const char *in, char **out, size_t *size);

#endif /* ACL_CLIENT_ONLY */

#ifdef	__cplusplus
}
#endif

#endif
