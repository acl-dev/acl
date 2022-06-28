#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_stdlib.h"
#include "db/acl_dberr.h"

#endif

#ifndef ACL_CLIENT_ONLY

#include "acl_dbnull.h"

int acl_dbnull_results(ACL_DB_HANDLE *handle acl_unused,
	const char *sql acl_unused, int *error acl_unused,
	int (*walk_fn)(const void** my_row, void *arg) acl_unused,
	void *arg acl_unused)
{       
	const char *myname = "acl_dbnull_results";

	acl_msg_fatal("%s, %s(%d): not supported!", __FILE__, myname, __LINE__);
	return (-1);
}

int acl_dbnull_result(ACL_DB_HANDLE *handle acl_unused,
	const char *sql acl_unused, int *error acl_unused,
	int (*callback)(const void** my_row, void *arg) acl_unused,
	void *arg acl_unused)
{
	const char *myname = "acl_dbnull_result";

	acl_msg_fatal("%s, %s(%d): not supported!", __FILE__, myname, __LINE__);
	return (-1);
}

int acl_dbnull_update(ACL_DB_HANDLE *handle acl_unused,
	const char *sql acl_unused, int *error acl_unused)
{
	const char *myname = "acl_dbnull_update";

	acl_msg_fatal("%s, %s(%d): not supported!", __FILE__, myname, __LINE__);
	return (-1);
}

#endif /* ACL_CLIENT_ONLY */
