#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include <stdio.h>
#include <stdlib.h>
#include "stdlib/acl_mystring.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_scan_dir.h"
#include "db/zdb.h"

#endif

#ifndef ACL_CLIENT_ONLY

#include "zdb_private.h"

static int zdb_dat_scan_path(ZDB *db, const char *path,
	int (*walk_fn)(ZDB_DAT_STORE *store))
{
	const char *myname = "zdb_dat_scan_path";
	ZDB_DAT_STORE *store;
	ACL_SCAN_DIR *scan;
	const char *fname;
	char  pathbuf[256];
	int   ret = 0;

	scan = acl_scan_dir_open(path, 1);
	if (scan == NULL) {
		acl_msg_error("%s(%d): open dir %s error(%s)",
			myname, __LINE__, path, acl_last_serror());
		return (-1);
	}

	while (1) {
		fname = acl_scan_dir_next_file(scan);
		if (fname == NULL) {
			acl_msg_info("%s(%d): scan over for %s",
				myname, __LINE__, path);
			break;
		}
		if (strrncasecmp(fname, ".dat", 4) != 0) {
			acl_msg_info("%s(%d): skip %s/%s", myname,
				__LINE__, acl_scan_dir_path(scan), fname);
			continue;
		}
		snprintf(pathbuf, sizeof(pathbuf), "%s/%s",
			acl_scan_dir_path(scan), fname);
		store = zdb_dat_store_open(db, pathbuf);
		if (store == NULL) {
			acl_msg_error("%s(%d): open file(%s) error(%s)",
				myname, __LINE__, pathbuf, acl_last_serror());
			break;
		}

		ret = walk_fn(store);
		zdb_dat_store_close(store);

		if (ret < 0) {
			acl_msg_error("%s(%d): walk_fn ret: %d, break",
				myname, __LINE__, ret);
			break;
		}
	}

	acl_scan_dir_close(scan);
	return (ret);
}

int zdb_dat_walk(ZDB *db, int (*walk_fn)(ZDB_DAT_STORE *store))
{
	const char *myname = "zdb_dat_walk";
	int   ret = 0, i;

	for (i = 0; db->dat_disks[i].path != NULL; i++) {
		acl_msg_info("%s(%d): begin scan %s",
			myname, __LINE__, db->dat_disks[i].path);
		ret = zdb_dat_scan_path(db, db->dat_disks[i].path, walk_fn);
		acl_msg_info("%s(%d): scan %s end\n",
			myname, __LINE__, db->dat_disks[i].path);
		if (ret < 0)
			break;
	}

	return (ret);
}

#endif /* ACL_CLIENT_ONLY */
