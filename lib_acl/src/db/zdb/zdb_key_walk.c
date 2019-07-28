#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include <stdio.h>
#include <stdlib.h>
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_mystring.h"
#include "stdlib/acl_scan_dir.h"
#include "db/zdb.h"

#endif

#ifndef ACL_CLIENT_ONLY

#include "zdb_private.h"

int zdb_key_walk(ZDB *db, int (*walk_fn)(ZDB_KEY_STORE*))
{
	const char *myname = "zdb_key_walk";
	ZDB_KEY_STORE *store;
	ACL_SCAN_DIR *scan = NULL;
	const char *fname;
	char  pathbuf[256];
	int   ret = 0;

	scan = acl_scan_dir_open(db->key_path, 1);
	if (scan == NULL) {
		acl_msg_error("%s: open dir %s error(%s)",
			myname, db->key_path, acl_last_serror());
		return (-1);
	}

	while (1) {
		fname = acl_scan_dir_next_file(scan);
		if (fname == NULL) {
			acl_msg_info("%s: scan over for %s\n", myname, db->key_path);
			break;
		}
		if (strrncasecmp(fname, ".key", 4) != 0) {
			acl_msg_info("%s: skip %s/%s\n", myname,
				acl_scan_dir_path(scan), fname);
			continue;
		}
		snprintf(pathbuf, sizeof(pathbuf), "%s/%s",
			acl_scan_dir_path(scan), fname);
		store = zdb_key_store_open2(db, pathbuf);
		if (store == NULL) {
			acl_msg_error("%s: open file(%s) error(%s)",
				myname, pathbuf, acl_last_serror());
			ret = -1;
			break;
		}

		ret = walk_fn(store);
		zdb_key_store_close(store);

		if (ret < 0)
			break;
	}

	acl_scan_dir_close(scan);
	return (ret);
}

#endif /* ACL_CLIENT_ONLY */
