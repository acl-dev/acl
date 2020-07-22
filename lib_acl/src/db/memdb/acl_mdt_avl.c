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
#include "thread/acl_thread.h"
#include "stdlib/acl_vstring.h"
#include "stdlib/acl_mystring.h"
#include "stdlib/acl_avl.h"

#endif

#include "define.h"
#include "struct.h"
#include "mdb_private.h"

#ifdef  PACK_STRUCT
#pragma pack(4)
#endif
typedef struct {
	union {
		char *key;
		const char *c_key;
	} key;
	ACL_MDT_REC *rec;
	avl_node_t node;
} TREE_NODE;
#ifdef  PACK_STRUCT
#pragma pack(0)
#endif

/**
 * AVL 用的比较回调函数
 */
static int cmp_fn(const void *v1, const void *v2)
{
	const TREE_NODE *n1 = (const TREE_NODE*) v1;
	const TREE_NODE *n2 = (const TREE_NODE*) v2;
	int   ret = strcmp(n1->key.c_key, n2->key.c_key);

	if (ret < 0)
		return (-1);
	else if (ret > 0)
		return (1);
	else
		return (0);
}

/**
 * 创建索引
 */
static ACL_MDT_IDX *mdt_idx_create(ACL_MDT *mdt, size_t init_capacity acl_unused,
	const char *name, unsigned int flag)
{
	ACL_MDT_IDX_AVL *idx;
#ifdef	_LP64
	unsigned int slice_align = ACL_SLICE_FLAG_LP64_ALIGN;
#else
	unsigned int slice_align = 0;
#endif
	unsigned int rtgc_off = 0;

	idx = (ACL_MDT_IDX_AVL*) acl_mycalloc(1, sizeof(ACL_MDT_IDX_AVL));
	avl_create(&idx->avl, cmp_fn, sizeof(TREE_NODE), offsetof(TREE_NODE, node));

	if ((mdt->tbl_flag & ACL_MDT_FLAG_SLICE_RTGC_OFF))
		rtgc_off = 1;

	if ((mdt->tbl_flag & ACL_MDT_FLAG_SLICE1))
		idx->slice = acl_slice_create("ACL_MDT_IDX_AVL->slice", 0,
			sizeof(TREE_NODE), ACL_SLICE_FLAG_GC1 | slice_align | rtgc_off);
	else if ((mdt->tbl_flag & ACL_MDT_FLAG_SLICE2))
		idx->slice = acl_slice_create("ACL_MDT_IDX_AVL->slice", 0,
			sizeof(TREE_NODE), ACL_SLICE_FLAG_GC2 | slice_align | rtgc_off);
	else if ((mdt->tbl_flag & ACL_MDT_FLAG_SLICE3))
		idx->slice = acl_slice_create("ACL_MDT_IDX_AVL->slice", 0,
			sizeof(TREE_NODE), ACL_SLICE_FLAG_GC3 | slice_align | rtgc_off);

	idx->idx.name = acl_mystrdup(name);
	idx->idx.flag = flag;
	return ((ACL_MDT_IDX*) idx);
}

static void mdt_idx_free(ACL_MDT_IDX *idx)
{
	ACL_MDT_IDX_AVL *idx_avl = (ACL_MDT_IDX_AVL*) idx;
	TREE_NODE *pnode;

	while (1) {
		pnode = (TREE_NODE*) avl_first(&idx_avl->avl);
		if (pnode == NULL)
			break;

		avl_remove(&idx_avl->avl, pnode);
		if (!(idx->flag & ACL_MDT_FLAG_KMR))
			acl_myfree(pnode->key.key);

		if (idx_avl->slice)
			acl_slice_free2(idx_avl->slice, pnode);
		else
			acl_myfree(pnode);
	}

	avl_destroy(&idx_avl->avl);
	acl_myfree(idx->name);
	if (idx_avl->slice)
		acl_slice_destroy(idx_avl->slice);
	acl_myfree(idx_avl);
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
	ACL_MDT_IDX_AVL *idx_avl = (ACL_MDT_IDX_AVL*) idx;
	TREE_NODE *pnode;

	if (idx_avl->slice)
		pnode = (TREE_NODE*) acl_slice_alloc(idx_avl->slice);
	else
		pnode = (TREE_NODE*) acl_mymalloc(sizeof(TREE_NODE));
	if (idx->flag & ACL_MDT_FLAG_KMR)
		pnode->key.c_key = key;
	else
		pnode->key.key = acl_mystrdup(key);

	pnode->rec = rec;
	avl_add(&idx_avl->avl, pnode);
	rec->key = pnode->key.c_key;
}

/**
 * 从数据表的索引中查询对应某个索引键值的结果集
 * @param idx {ACL_MDT_IDX*} 表索引
 * @param key {const char*} 数据表索引字段值
 * @return {ACL_MDT_REC*} 对应某个索引字段值的结果集合
 */
static ACL_MDT_REC *mdt_idx_get(ACL_MDT_IDX *idx, const char *key)
{
	ACL_MDT_IDX_AVL *idx_avl = (ACL_MDT_IDX_AVL*) idx;
	TREE_NODE  node, *pnode;

	node.key.c_key = key;
	pnode = (TREE_NODE*) avl_find(&idx_avl->avl, &node, NULL);
	return (pnode ? pnode->rec : NULL);
}

/**
 * 从一个表索引中删除该索引
 * @param idx {ACL_MDT_IDX*} 表索引
 * @param rec {ACL_MDT_REC*} 索引表中对应某个键的结果集对象
 * @param key_value {const char*} 数据结点的引用结点的引用键值
 */
static void mdt_idx_del(ACL_MDT_IDX *idx, const char *key)
{
	const char *myname = "mdt_idx_del";
	ACL_MDT_IDX_AVL *idx_avl = (ACL_MDT_IDX_AVL*) idx;
	TREE_NODE node, *pnode;

	node.key.c_key = key;
	pnode = (TREE_NODE*) avl_find(&idx_avl->avl, &node, NULL);
	if (pnode == NULL)
		acl_msg_fatal("%s: key(%s) not exist", myname, key);
	avl_remove(&idx_avl->avl, pnode);
	if (!(idx->flag & ACL_MDT_FLAG_KMR))
		acl_myfree(pnode->key.key);
	if (idx_avl->slice)
		acl_slice_free2(idx_avl->slice, pnode);
	else
		acl_myfree(pnode);
}

/**
 * 释放平衡二叉树模式的数据表
 */
static void mdt_avl_free(ACL_MDT *mdt)
{
	ACL_MDT_AVL *mdt_avl = (ACL_MDT_AVL*) mdt;

	acl_myfree(mdt_avl);
}

ACL_MDT *acl_mdt_avl_create()
{
	ACL_MDT_AVL *mdt;

	mdt = (ACL_MDT_AVL *) acl_mycalloc(1, sizeof(ACL_MDT_AVL));
	mdt->mdt.tbl_free = mdt_avl_free;
	mdt->mdt.idx_create = mdt_idx_create;
	mdt->mdt.idx_free = mdt_idx_free;
	mdt->mdt.idx_add = mdt_idx_add;
	mdt->mdt.idx_get = mdt_idx_get;
	mdt->mdt.idx_del = mdt_idx_del;
	return ((ACL_MDT*) mdt);
}
