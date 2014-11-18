#pragma once
#include "acl_cpp/acl_cpp_define.hpp"

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
#define MIME_CTYPE_MAX          MIME_CTYPE_APPLICATION

#define MIME_STYPE_OTHER        0
#define MIME_STYPE_PLAIN        1
#define MIME_STYPE_HTML         2
#define MIME_STYPE_RFC822       3
#define MIME_STYPE_PARTIAL      4
#define MIME_STYPE_EXTERN_BODY  5
#define MIME_STYPE_JPEG         6
#define MIME_STYPE_GIF          7
#define MIME_STYPE_BMP          8
#define MIME_STYPE_PNG          9
#define MIME_STYPE_OCTET_STREAM 10
#define MIME_STYPE_MIXED        11
#define MIME_STYPE_ALTERNATIVE  12
#define MIME_STYPE_RELATED      13
#define MIME_STYPE_MAX          MIME_STYPE_RELATED

#define MIME_ENC_OTHER  0
#define MIME_ENC_QP     1   /* encoding + domain */
#define MIME_ENC_BASE64 2   /* encoding + domain */
#define MIME_ENC_7BIT   7   /* domain only */
#define MIME_ENC_8BIT   8   /* domain only */
#define MIME_ENC_BINARY 9   /* domain only */
#define	MIME_ENC_UUCODE 10  /* encoding + domain */
#define	MIME_ENC_XXCODE 11  /* encoding + domain */
