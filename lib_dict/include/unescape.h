#ifndef __UNESCAPE_INCLUDE_H__
#define __UNESCAPE_INCLUDE_H__

#include "lib_acl.h"
#include "dict.h"

#ifdef __cplusplus
extern "C" {
#endif

DICT_API ACL_VSTRING *unescape(ACL_VSTRING *result, const char *data);
DICT_API ACL_VSTRING *escape(ACL_VSTRING *result, const char *data, ssize_t len);

#ifdef __cplusplus
}
#endif

#endif
