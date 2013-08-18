#ifndef __ACL_VBUF_PRINT_H_INCLUDED__
#define __ACL_VBUF_PRINT_H_INCLUDED__

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

