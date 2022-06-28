#ifndef _MAC_EXPAND_H_INCLUDED_
#define _MAC_EXPAND_H_INCLUDED_

#include "lib_acl.h"
#include "mac_parse.h"

 /*
  * Features.
  */
#define MAC_EXP_FLAG_NONE	(0)
#define MAC_EXP_FLAG_RECURSE	(1<<0)

 /*
  * Real lookup or just a test?
  */
#define MAC_EXP_MODE_TEST	(0)
#define MAC_EXP_MODE_USE	(1)

typedef const char *(*MAC_EXP_LOOKUP_FN)(char *, int, const char *, char **, size_t *);

extern int mac_expand(ACL_VSTRING *, const char *, int, const char *, MAC_EXP_LOOKUP_FN, char *);

#endif
