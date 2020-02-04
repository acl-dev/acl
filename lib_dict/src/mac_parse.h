#ifndef _MAC_PARSE_H_INCLUDED_
#define _MAC_PARSE_H_INCLUDED_

#include "lib_acl.h"

 /*
  * External interface.
  */
#define MAC_PARSE_LITERAL	1
#define MAC_PARSE_EXPR		2
#define MAC_PARSE_VARNAME	MAC_PARSE_EXPR	/* 2.1 compatibility */

#define MAC_PARSE_OK		0
#define MAC_PARSE_ERROR		(1<<0)
#define MAC_PARSE_UNDEF		(1<<1)
#define MAC_PARSE_USER		2	/* start user definitions */

typedef int (*MAC_PARSE_FN)(int, ACL_VSTRING *, void *);

extern int mac_parse(const char *, MAC_PARSE_FN, void *);

#endif
