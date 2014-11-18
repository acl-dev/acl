#ifndef _DICT_TC_H_INCLUDED_
#define _DICT_TC_H_INCLUDED_

#ifdef HAS_DB

#include <dict.h>

 /*
  * External interface.
  */
#define DICT_TYPE_TC "tc"

extern DICT *dict_tc_open(const char *, int, int);

#endif

#endif /* _DICT_TC_H_INCLUDED_ */

