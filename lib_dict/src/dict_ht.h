#ifndef _DICT_HT_H_INCLUDED_
#define _DICT_HT_H_INCLUDED_

#include "lib_acl.h"
#include <dict.h>

 /*
  * External interface.
  */
#define DICT_TYPE_HT	"internal"

extern DICT *dict_ht_open(const char *, ACL_HTABLE *, void (*) (void *));

#endif
