#ifndef ACL_VBUF_PRINT_INCLUDE_H
#define ACL_VBUF_PRINT_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "acl_define.h"

 /*
  * System library.
  */
#include <stdarg.h>

 /*
  * Utility library.
  */
#include "acl_vbuf.h"

 /*
  * External interface.
  */
ACL_API ACL_VBUF *acl_vbuf_print(ACL_VBUF *, const char *, va_list);

#ifdef  __cplusplus
}
#endif

#endif

