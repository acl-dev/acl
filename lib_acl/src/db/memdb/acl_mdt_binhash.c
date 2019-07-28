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
#include "stdlib/acl_mystring.h"

#endif

#include "struct.h"
#include "mdb_private.h"

/**
 * 创建索引
 */
static ACL_MDT_IDX *mdt_idx_create(ACL_MDT *mdt, size_t init_capacity,
	const char *name, unsigned int flag)
{
	ACL_MDT_IDX_BHASH *idx;
	unsigned int flag2 = 0;

	if (init_capacity < 128)
		init_capacity = 128;
	idx = (ACL_MDT_IDX_BHASH*) acl_mycalloc(1, sizeof(ACL_MDT_IDX_BHASH));

	if ((mdt->tbl_flag & ACL_MDT_FLAG_SLICE1))
		flag2 |= ACL_BINHASH_FLAG_SLICE1;
	else if ((mdt->tbl_flag & ACL_MDT_FLAG_SLICE2))
		flag2 |= ACL_BINHASH_FLAG_SLICE2;
	else if ((mdt->tbl_flag & ACL_MDT_FLAG_SLICE3))
		flag2 |= ACL_BINHASH_FLAG_SLICE3;

	if ((mdt->tbl_flag & ACL_MDT_FLAG_SLICE_RTGC_OFF))
		flag2 |= ACL_BINHASH_FLAG_SLICE_RTGC_OFF;
	if ((flag & ACL_MDT_FLAG_KMR))
		flag2 |= ACL_BINHASH_FLAG_KEY_REUSE;

	idx->table = acl_binhash_create((int) init_capacity, flag2);
	idx->idx.name = acl_mystrdup(name);
	idx->idx.flag = flag;
	return ((ACL_MDT_IDX*) idx);
}

static void mdt_idx_free(ACL_MDT_IDX *idx)
{
	ACL_MDT_IDX_BHASH *idx_bhash = (ACL_MDT_IDX_BHASH*) idx;

	acl_binhash_free(idx_bhash->table, NULL);
	acl_myfree(idx->name);
	acl_myfree(idx_bhash);
}


/**
 * 向一个表索引中添加新的字段
 * @param idx {ACL_MDT_IDX*} 表索引
 * @param key {const char*} 数据表索引字段值
 * @param rec {ACL_MDT_REC*}
 * @return {ACL_HTABLE_INFO*}
 */
static void mdt_idx_add(ACL_MDT_IDX *idx, const char *key, ACL_MDT_REC *rec)
{
	const char *myname = "mdt_idx_add";
	ACL_MDT_IDX_BHASH *idx_bhash = (ACL_MDT_IDX_BHASH*) idx;
	ACL_BINHASH_INFO *info;

	info = acl_binhash_enter(idx_bhash->table, key,
		(int) strlen(key) + 1, (char*) rec);
	if (info == NULL)
		acl_msg_error("%s(%d): binhash_enter error, value(%s)",
			myname, __LINE__, key);
	else
		rec->key = info->key.c_key;
}

/**
 * 从数据表的索引中查询对应某个索引键值的结果集
 * @param idx {ACL_MDT_IDX*} 表索引
 * @param key {const char*} 数据表索引字段值
 * @return {ACL_MDT_REC*} 对应某个索引字段值的结果集合
 */
static ACL_MDT_REC *mdt_idx_get(ACL_MDT_IDX *idx, const char *key)
{
	ACL_MDT_IDX_BHASH *idx_bhash = (ACL_MDT_IDX_BHASH*) idx;
	ACL_MDT_REC *rec;

	rec = (ACL_MDT_REC*) acl_binhash_find(idx_bhash->table, key,
		(int) strlen(key) + 1);
	return (rec);
}

/**
 * 从一个表索引中删除该索引
 * @param idx {ACL_MDT_IDX*} 表索引
 * @param key_value {const char*} 数据结点的引用结点的引用键值
 */
static void mdt_idx_del(ACL_MDT_IDX *idx, const char *key)
{
	ACL_MDT_IDX_BHASH *idx_bhash = (ACL_MDT_IDX_BHASH*) idx;

	/* idx->table 哈希表里存储的是: rec->key: rec 对，所以不需要在
	 * 哈希表内部释放 rec 内存，因为可以显示地释放该资源
	 */
	acl_binhash_delete(idx_bhash->table, key, (int) strlen(key) + 1, NULL);
}

/**
 * 释放二进制哈希模式的数据表
 */
static void mdt_binhash_free(ACL_MDT *mdt)
{
	ACL_MDT_BHASH *mdt_bhash = (ACL_MDT_BHASH*) mdt;

	acl_myfree(mdt_bhash);
}

ACL_MDT *acl_mdt_binhash_create()
{
	ACL_MDT_BHASH *mdt;

	mdt = (ACL_MDT_BHASH *) acl_mycalloc(1, sizeof(ACL_MDT_BHASH));
	mdt->mdt.tbl_free = mdt_binhash_free;
	mdt->mdt.idx_create = mdt_idx_create;
	mdt->mdt.idx_free = mdt_idx_free;
	mdt->mdt.idx_add = mdt_idx_add;
	mdt->mdt.idx_get = mdt_idx_get;
	mdt->mdt.idx_del = mdt_idx_del;
	return ((ACL_MDT*) mdt);
}
