#ifndef _DICT_CDB_H_INCLUDED_
#define _DICT_CDB_H_INCLUDED_

#ifdef HAS_CDB

#include <dict.h>

 /*
  * External interface.
  */
#define DICT_TYPE_CDB "cdb"

extern DICT *dict_cdb_open(const char *, int, int);

#endif

#endif /* _DICT_CDB_H_INCLUDED_ */

