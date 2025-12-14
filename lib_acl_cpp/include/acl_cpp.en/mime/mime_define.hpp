#pragma once
#include "../acl_cpp_define.hpp"

#if !defined(ACL_MIME_DISABLE)

/*
 * Content types and subtypes that we care about, either because we have to,
 * or because we want to filter out broken MIME messages.
 */

#define MIME_CTYPE_OTHER        0
#define MIME_CTYPE_TEXT         1
#define MIME_CTYPE_MESSAGE      2
#define MIME_CTYPE_MULTIPART    3
#define MIME_CTYPE_IMAGE        4
#define MIME_CTYPE_APPLICATION  5
#define MIME_CTYPE_MIN          MIME_CTYPE_OTHER
#define MIME_CTYPE_MAX          MIME_CTYPE_APPLICATION

#define MIME_STYPE_OTHER        6
#define MIME_STYPE_PLAIN        7
#define MIME_STYPE_HTML         8
#define MIME_STYPE_RFC822       9
#define MIME_STYPE_PARTIAL      10
#define MIME_STYPE_EXTERN_BODY  11
#define MIME_STYPE_JPEG         12
#define MIME_STYPE_GIF          13
#define MIME_STYPE_BMP          14
#define MIME_STYPE_PNG          15
#define MIME_STYPE_OCTET_STREAM 16
#define MIME_STYPE_MIXED        17
#define MIME_STYPE_ALTERNATIVE  18
#define MIME_STYPE_RELATED      19
#define MIME_STYPE_MIN          MIME_STYPE_OTHER
#define MIME_STYPE_MAX          MIME_STYPE_RELATED

#define MIME_ENC_OTHER          20
#define MIME_ENC_QP             21   /* encoding + domain */
#define MIME_ENC_BASE64         22   /* encoding + domain */
#define MIME_ENC_7BIT           23   /* domain only */
#define MIME_ENC_8BIT           24   /* domain only */
#define MIME_ENC_BINARY         25   /* domain only */
#define	MIME_ENC_UUCODE         26   /* encoding + domain */
#define	MIME_ENC_XXCODE         27   /* encoding + domain */
#define MIME_ENC_MIN            MIME_ENC_OTHER
#define MIME_ENC_MAX            MIME_ENC_XXCODE

#define MIME_MIN                MIME_CTYPE_OTHER
#define MIME_MAX                MIME_ENC_MAX

#endif // !defined(ACL_MIME_DISABLE)
