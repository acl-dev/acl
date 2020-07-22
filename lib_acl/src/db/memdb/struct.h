#ifndef __MESTRUCT_INCLUDE_H__
#define __MESTRUCT_INCLUDE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdlib/acl_define.h"
#include "stdlib/acl_array.h"
#include "stdlib/acl_htable.h"
#include "db/acl_mdb.h"
#include "stdlib/acl_avl.h"
#include "stdlib/acl_slice.h"
#include "stdlib/acl_htable.h"
#include "stdlib/acl_binhash.h"
#include "ring.h"

/* 用户查询结果集合 */
struct ACL_MDT_RES {
	ACL_ARRAY *a;			/* 将 ACL_MDT_NOD 的查询结果集合存储在此数组中 */
	int   ipos;			/* 下一个数组元素在动态数组中的位置 */
};

/* 数据结点的数据结构定义(24 byte) */
struct ACL_MDT_NOD {
	RING mdt_entry;			/* 链接进 ACL_MDT 中的 node_head */
	RING ref_head;			/* 引用该数据结点的所有引用结点的集合 */
	void  *data;			/* 用户级数据项的内部表示, 可以为各种用户
					 * 所自定义的数据类型, 通过强制类型转换将其
					 * 统一转化为 void * 类型, 在建立索引时用户
					 * 需要以自定义数据类型中的某些字段为键值
					 * 来建立索引记录及索引表, 从而建立索引数据库,
					 * 最终该 data 字段中的键将与 ACL_MDT_REC 中的
					 * key 相同并映射到 ACL_MDT_REC 中的 table
					 * 集合中.
					 */

	unsigned int dlen:24;		/* data 数据的长度 */
	unsigned int nrefer:8;		/* 引用该数据结点的 ACL_MDT_REF 数量 */
};

/* 索引表的索引记录数据结构定义, 索引表(ACL_MDT)的索引集合字段(table)
 * 的组成单位为 ACL_REC, 即 ACL_MDT.table 是由一系列 ACL_REC
 * 数据索引记录组成的.
 * (12 byte)
 */
struct ACL_MDT_REC {
	RING ref_head;			/* 具有相同键值的 ACL_MDT_NOD_REF 的集合 */
	ACL_MDT_IDX *idx;		/* 引用其所属的索引表 */
	const char *key;		/* 索引键引用 */
	unsigned int   nrefer:8;	/* 该结果集合的 ACL_MDT_NODE_REF 元素总和 */
};

/**
 * 引用结点定义(20 byte)
 */
struct ACL_MDT_REF {
	RING nod_entry;			/* 与 ACL_MDT_NOD 关联 */
	RING rec_entry;			/* 与 ACL_MDT_REC 关联 */
	ACL_MDT_NOD *node;		/* 引用 ACL_MDT_NOD */
	ACL_MDT_REC *rec;		/* 引用 ACL_MDT_REC */
};

/**
 * 表的索引字段对象类型, 索引表中每个索引项都有一个索引字段对象
 */
struct ACL_MDT_IDX {
	RING mdt_entry;			/* 链接进 ACL_MDT 中的 idx_head 链表 */
	char *name;			/* 字段名称 */
	ACL_MDT *mdt;			/* 反向引用其所从属的索引表 */
	unsigned int flag;		/* 该数据表的约束标志 */
};

/**
 * 表索引以哈希表方式存储
 */
typedef struct ACL_MDT_IDX_HASH {
	ACL_MDT_IDX idx;
	ACL_HTABLE *table;		/* 容纳索引键及其所标识数据结点对象的集合 */
} ACL_MDT_IDX_HASH;

/**
 * 表索引以哈希表方式存储
 */
typedef struct ACL_MDT_IDX_BHASH {
	ACL_MDT_IDX idx;
	ACL_BINHASH *table;		/* 容纳索引键及其所标识数据结点对象的集合 */
} ACL_MDT_IDX_BHASH;

/**
 * 表索引以平稳二叉树方式存储
 */
typedef struct ACL_MDT_IDX_AVL {
	ACL_MDT_IDX idx;
	avl_tree_t avl;
	ACL_SLICE *slice;		/* 内存分配池 */
} ACL_MDT_IDX_AVL;

/* 索引表数据结构定义, 每个索引表在建立时就是以数据结点(ACL_NODE)中的
 * 用户数据(data)中的某个数据字段为键值建立的.
 */
struct ACL_MDT {
	RING idx_head;			/* 索引字段对象链表 */
	RING nod_head;			/* 所有数据结点的集合 */

	char *name;			/* 索引表名称 */
	int   node_cnt;			/* 所有数据结点的总和 */
	int   error;			/* 当出错时记录出错状态号 */
	unsigned int tbl_flag;		/* 标志位 */

	ACL_SLICE *nod_slice;
	ACL_SLICE *rec_slice;
	ACL_SLICE *ref_slice;

	/* public */
	ACL_MDT_NOD *(*add)(ACL_MDT *mdt, void *data, unsigned int dlen,
	        const char *key_labels[], const char *keys[]);
	ACL_MDT_RES *(*get)(ACL_MDT *mdt, const char *key_label,
	        const char *key, int from, int limit);
	int (*del)(ACL_MDT *mdt, const char *key_label,
	        const char *key, void (*onfree_fn)(void *data, unsigned int dlen));
	int (*probe)(ACL_MDT *mdt, const char *key_label, const char *key);
	ACL_MDT_RES *(*list)(ACL_MDT *mdt, int from, int limit);
	int (*walk)(ACL_MDT *mdt, int (*walk_fn)(const void *data, unsigned int dlen),
		int from, int limit);

	void (*tbl_free)(ACL_MDT*);

	/* private */
	ACL_MDT_IDX *(*idx_create)(ACL_MDT *mdt, size_t init_capacity,
		const char *name, unsigned int flag);
	void (*idx_free)(ACL_MDT_IDX *idx);
	void (*idx_add)(ACL_MDT_IDX *idx, const char *key, ACL_MDT_REC *rec);
	void (*idx_del)(ACL_MDT_IDX *idx, const char *key);
	ACL_MDT_REC *(*idx_get)(ACL_MDT_IDX *idx, const char *key);
};

typedef struct ACL_MDT_HASH {
	ACL_MDT mdt;
} ACL_MDT_HASH;

typedef struct ACL_MDT_BHASH {
	ACL_MDT mdt;
} ACL_MDT_BHASH;

typedef struct ACL_MDT_AVL {
	ACL_MDT mdt;
} ACL_MDT_AVL;

/* 索引数据库数据结构定义 */
struct ACL_MDB {
	char   name[128];		/* 数据库名称 */
	char   type[32];		/* 数据库类型: hash/avl */
	ACL_HTABLE *tbls;		/* 容纳所有索引表的集合 */
};

#ifdef __cplusplus
}
#endif

#endif

