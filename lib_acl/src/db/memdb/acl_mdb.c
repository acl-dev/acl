#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_htable.h"
#include "stdlib/acl_mystring.h"

#include "db/acl_mdb.h"

#endif

#include "struct.h"
#include "mdb_private.h"

ACL_MDB *acl_mdb_create(const char *dbname, const char *dbtype)
{
	const char *myname = "acl_mdb_create";
	ACL_MDB *mdb;

	if (dbname == NULL || *dbname == 0) {
		acl_msg_error("%s(%d): dbname invalid", myname, __LINE__);
		return (NULL);
	}
	if (dbtype == NULL || *dbtype == 0) {
		acl_msg_error("%s(%d): dbtype invalid", myname, __LINE__);
		return (NULL);
	}

	mdb = acl_mycalloc(1, sizeof(ACL_MDB));
	ACL_SAFE_STRNCPY(mdb->name, dbname, sizeof(mdb->name));
	ACL_SAFE_STRNCPY(mdb->type, dbtype, sizeof(mdb->type));
	mdb->tbls = acl_htable_create(10, 0);
	return (mdb);
}

static void free_tbl_fn(void *arg)
{
	ACL_MDT *mdt = (ACL_MDT*) arg;
	acl_mdt_free(mdt);
}

void acl_mdb_free(ACL_MDB *mdb)
{
	acl_htable_free(mdb->tbls, free_tbl_fn);
	acl_myfree(mdb);
}

ACL_MDT *acl_mdb_tbl_create(ACL_MDB *mdb, const char *tbl_name,
	unsigned int tbl_flag, size_t init_capacity,
	const char *key_labels[], unsigned int flags[])
{
	const char *myname = "acl_mdb_tbl_create";
	ACL_MDT *mdt;

	if (mdb == NULL || tbl_name == NULL || *tbl_name == 0) {
		acl_msg_error("%s(%d): input invalid", myname, __LINE__);
		return (NULL);
	}

	if (init_capacity < 128)
		init_capacity = 128;

	mdt = acl_mdt_create(mdb->type, tbl_name, tbl_flag,
			init_capacity, key_labels, flags);

	if (acl_htable_enter(mdb->tbls, tbl_name, (char *) mdt) == NULL)
		acl_msg_fatal("%s(%d): acl_htable_enter error, tbl_name = %s",
			myname, __LINE__, tbl_name);

	return (mdt);
}

ACL_MDT_NOD *acl_mdb_add(ACL_MDB *mdb, const char *tbl_name,
	void *data, unsigned int dlen,
	const char *key_labels[], const char *keys[])
{
	const char *myname = "acl_mdb_add";
	ACL_MDT *mdt;
	ACL_MDT_NOD *node;

	if (tbl_name == NULL || *tbl_name == 0) {
		acl_msg_error("%s(%d): tbl_name invalid", myname, __LINE__);
		return (NULL);
	}
	if (data == NULL) {
		acl_msg_error("%s(%d): data invalid", myname, __LINE__);
		return (NULL);
	}

	/* 从数据库中获得所需要的数据索引表 */
	mdt = (ACL_MDT *) acl_htable_find(mdb->tbls, tbl_name);
	if (mdt == NULL) {
		acl_msg_error("%s(%d): table no exist, tbl_name(%s)",
			myname, __LINE__, tbl_name);
		return (NULL);
	}

	node = mdt->add(mdt, data, dlen, key_labels, keys);

	return (node);
}

int acl_mdb_probe(ACL_MDB *mdb, const char *tbl_name,
	const char *key_label, const char *key)
{
	const char *myname = "acl_mdb_probe";
	ACL_MDT *mdt;

	if (tbl_name == NULL || *tbl_name == 0) {
		acl_msg_error("%s(%d): tbl_name invalid", myname, __LINE__);
		return (0);
	}
	if (key_label == NULL || *key_label == 0) {
		acl_msg_error("%s(%d): key_label invalid", myname, __LINE__);
		return (0);
	}
	if (key == NULL || *key == 0) {
		acl_msg_error("%s(%d): key invalid", myname, __LINE__);
		return (0);
	}

	mdt = (ACL_MDT *) acl_htable_find(mdb->tbls, tbl_name);
	if (mdt == NULL) {
		acl_msg_error("%s(%d): table no exist, tbl_name(%s)",
			myname, __LINE__, tbl_name);
		return (0);
	}

	return (mdt->probe(mdt, key_label, key));
}

ACL_MDT_RES *acl_mdb_find(ACL_MDB *mdb, const char *tbl_name,
	const char *key_label, const char *key, int from, int limit)
{
	const char *myname = "acl_mdb_find";
	ACL_MDT *mdt;

	if (tbl_name == NULL || *tbl_name == 0) {
		acl_msg_error("%s(%d): tbl_name invalid", myname, __LINE__);
		return (NULL);
	}
	if (key_label == NULL || *key_label == 0) {
		acl_msg_error("%s(%d): key_label invalid", myname, __LINE__);
		return (NULL);
	}
	if (key == NULL || *key == 0) {
		acl_msg_error("%s(%d): key invalid", myname, __LINE__);
		return (NULL);
	}

	mdt = (ACL_MDT *) acl_htable_find(mdb->tbls, tbl_name);
	if (mdt == NULL) {
		acl_msg_error("%s(%d): table no exist, tbl_name(%s)",
			myname, __LINE__, tbl_name);
		return (NULL);
	}

	return (mdt->get(mdt, key_label, key, from, limit));
}

ACL_MDT_RES *acl_mdb_list(ACL_MDB *mdb, const char *tbl_name, int from, int limit)
{
	const char *myname = "acl_mdb_list";
	ACL_MDT *mdt;

	if (tbl_name == NULL || *tbl_name == 0) {
		acl_msg_error("%s(%d): tbl_name invalid", myname, __LINE__);
		return (NULL);
	}

	mdt = (ACL_MDT *) acl_htable_find(mdb->tbls, tbl_name);
	if (mdt == NULL) {
		acl_msg_error("%s(%d): table no exist, tbl_name(%s)",
			myname, __LINE__, tbl_name);
		return (NULL);
	}

	return (mdt->list(mdt, from, limit));
}

int acl_mdb_del(ACL_MDB *mdb, const char *tbl_name,
	const char *key_label, const char *key,
	void (*onfree_fn)(void*, unsigned int))
{
	const char *myname = "acl_mdb_del";
	ACL_MDT *mdt;

	if (tbl_name == NULL || *tbl_name == 0) {
		acl_msg_error("%s(%d): tbl_name invalid", myname, __LINE__);
		return (-1);
	}
	if (key_label == NULL || *key_label == 0) {
		acl_msg_error("%s(%d): key_label invalid", myname, __LINE__);
		return (-1);
	}
	if (key == NULL || *key == 0) {
		acl_msg_error("%s(%d): key invalid", myname, __LINE__);
		return (-1);
	}

	mdt = (ACL_MDT *) acl_htable_find(mdb->tbls, tbl_name);
	if (mdt == NULL) {
		acl_msg_error("%s(%d): table no exist, tbl_name(%s)",
			myname, __LINE__, tbl_name);
		return (-1);
	}

	return (mdt->del(mdt, key_label, key, onfree_fn));
}

int acl_mdb_walk(ACL_MDB *mdb, const char *tbl_name,
	int (*walk_fn)(const void*, unsigned int), int from, int limit)
{
	const char *myname = "acl_mdb_walk";
	ACL_MDT *mdt;

	if (tbl_name == NULL || *tbl_name == 0) {
		acl_msg_error("%s(%d): tbl_name invalid", myname, __LINE__);
		return (-1);
	}

	mdt = (ACL_MDT *) acl_htable_find(mdb->tbls, tbl_name);
	if (mdt == NULL) {
		acl_msg_error("%s(%d): table no exist, tbl_name(%s)",
			myname, __LINE__, tbl_name);
		return (-1);
	}

	return (mdt->walk(mdt, walk_fn, from, limit));
}

int acl_mdb_cnt(ACL_MDB *mdb, const char *tbl_name)
{
	const char *myname = "acl_mdb_cnt";
	ACL_MDT *mdt;

	if (tbl_name == NULL || *tbl_name == 0) {
		acl_msg_error("%s(%d): tbl_name invalid", myname, __LINE__);
		return (-1);
	}

	mdt = (ACL_MDT *) acl_htable_find(mdb->tbls, tbl_name);
	if (mdt == NULL) {
		acl_msg_error("%s(%d): table no exist, tbl_name(%s)",
			myname, __LINE__, tbl_name);
		return (-1);
	}

	return (acl_mdt_cnt(mdt));
}
