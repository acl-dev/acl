#ifndef _DICT_DB_H_INCLUDED_
#define _DICT_DB_H_INCLUDED_

#ifdef HAS_BDB
# include "db.h"
# include "dict.h"

 /*
  * External interface.
  */
#define DICT_TYPE_HASH	"hash"
#define DICT_TYPE_BTREE	"btree"

#ifdef ACL_UNIX
# define FILE_HANDLE(x) (x)
#elif defined(WIN32)
# include <io.h>
# define FILE_HANDLE(x) _get_osfhandle(x)
#endif
DICT *dict_hash_open(const char *path, int open_flags, int dict_flags);
DICT *dict_btree_open(const char *path, int open_flags, int dict_flags);
DB_ENV *dict_db_env(DICT *dict);

#endif

#endif
