#ifndef	ACL_MEMDB_INCLUDE_H
#define	ACL_MEMDB_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"

typedef struct ACL_MDT_NOD ACL_MDT_NOD;
typedef struct ACL_MDT_RES ACL_MDT_RES;
typedef struct ACL_MDT_REC ACL_MDT_REC;
typedef struct ACL_MDT_REF ACL_MDT_REF;
typedef struct ACL_MDT_IDX ACL_MDT_IDX;
typedef struct ACL_MDT ACL_MDT;
typedef struct ACL_MDB ACL_MDB;

/* 数据表各索引关键字段的约束标志位 */
#define ACL_MDT_FLAG_NUL		(0)
#define ACL_MDT_FLAG_UNI		(1 << 0)	/**< 表示唯一 */
#define	ACL_MDT_FLAG_KMR		(1 << 1)	/**< 表示重用键内存 */
#define ACL_MDT_FLAG_DMR		(1 << 2)	/**< 表示重用值内存 */
#define	ACL_MDT_FLAG_SLICE1		(1 << 10)	/**< 启用 ACL_SLICE_FLAG_GC1 */
#define	ACL_MDT_FLAG_SLICE2		(1 << 11)	/**< 启用 ACL_SLICE_FLAG_GC2 */
#define	ACL_MDT_FLAG_SLICE3		(1 << 12)	/**< 启用 ACL_SLICE_FLAG_GC3 */
#define	ACL_MDT_FLAG_SLICE_RTGC_OFF	(1 << 13)	/**< 关闭内存切片的实时垃圾回收功能 */

/************************************************************************/
/*                          in acl_mdb.c                                */
/************************************************************************/

/**
 * 创建一个数据库句柄
 * @param dbname {const char*} 数据库名
 * @param dbtype {const char*} 数据库类型: hash/avl
 * @return {ACL_MDB*} 数据库句柄
 */
ACL_API ACL_MDB *acl_mdb_create(const char *dbname, const char *dbtype);

/**
 * 关闭并释放一个内存数据库
 * @param mdb {ACL_MDB*} 数据库句柄
 */
ACL_API void acl_mdb_free(ACL_MDB *mdb);

/**
 * 在给定数据库上创建一个数据表
 * @param mdb {ACL_MDB*} 数据库句柄
 * @param tbl_name {const char*} 表名
 * @param tbl_flag {unsigned int} 表的属性标志位
 * @param init_capacity {size_t} 数据表内针对每个索引键的内部哈希表的初始化容量
 * @param key_labels {const char *[]} 数据表字段名数组
 * @param flags {unsigned int[]} 与 key_labels 相对应的约束标志位
 * @return {ACL_MDT*} 新建的数据表句柄
 */
ACL_API ACL_MDT *acl_mdb_tbl_create(ACL_MDB *mdb, const char *tbl_name,
	unsigned int tbl_flag, size_t init_capacity,
	const char *key_labels[], unsigned int flags[]);

/**
 * 添加一条新的数据记录
 * @param mdb {ACL_MDB*} 数据库句柄
 * @param tbl_name {const char*} 数据表名
 * @param data {void*} 应用数据项
 * @param dlen {unsigned int} data 的数据大小
 * @param key_labels {const char *[]} 数据表字段名数组
 * @param keys {const char *[]} 数据表字段名对应值数组
 * @return {ACL_MDB_NOD*} 新添加的数据在数据表中存储的句柄
 */
ACL_API ACL_MDT_NOD *acl_mdb_add(ACL_MDB *mdb, const char *tbl_name,
	void *data, unsigned int dlen,
	const char *key_labels[], const char *keys[]);

/**
 * 探测数据表中对应的字段值是否存在
 * @param mdb {ACL_MDB*} 数据库句柄
 * @param tbl_name {const char*} 数据表名
 * @param key_label {const char*} 数据表索引字段名
 * @param key {const char*} 数据表索引字段键值
 * @return {int} 0: 不存在, != 0: 存在
 */
ACL_API int acl_mdb_probe(ACL_MDB *mdb, const char *tbl_name,
	const char *key_label, const char *key);

/**
 * 从数据库中查询符合条件的结果集
 * @param mdb {ACL_MDB*} 数据库句柄
 * @param tbl_name {const char*} 数据表名
 * @param key_label {const char*} 数据表中的字段名
 * @param key {const char*} 数据表中的字段值
 * @param from {int} 查询的结果希望是从该位置开始进行存储
 * @param limit {int} 查询的结果的最大希望个数
 * @return {ACL_MDT_RES*} 查询结果集，如果为空则表明查询结果为空或出错
 */
ACL_API ACL_MDT_RES *acl_mdb_find(ACL_MDB *mdb, const char *tbl_name,
	const char *key_label, const char *key, int from, int limit);

/**
 * 从数据库中列出某数据表中某个范围的结果集
 * @param mdb {ACL_MDB*} 数据库句柄
 * @param tbl_name {const char*} 数据表名
 * @param from {int} 查询的结果希望是从该位置开始进行存储
 * @param limit {int} 查询的结果的最大希望个数
 * @return {ACL_MDT_RES*} 查询结果集，如果为空则表明查询结果为空或出错
 */
ACL_API ACL_MDT_RES *acl_mdb_list(ACL_MDB *mdb, const char *tbl_name,
	int from, int limit);

/**
 * 从数据库中删除一条数据记录
 * @param mdb {ACL_MDB*} 数据库句柄
 * @param tbl_name {const char*} 数据表名
 * @param key_label {const char*} 数据表字段名
 * @param key {const char*} 数据表字段值
 * @param onfree_fn {void (*)(void*, unsigned int)}
  *	释放用户的对象时调用的释放回调函数
 * @return {int} 删除的行数量
 */
ACL_API int acl_mdb_del(ACL_MDB *mdb, const char *tbl_name,
	const char *key_label, const char *key,
	void (*onfree_fn)(void*, unsigned int));

/**
 * 遍历数据库中某个数据表的所有数据结点
 * @param mdb {ACL_MDB*} 数据库句柄
 * @param tbl_name {const char*} 数据表名
 * @param walk_fn 遍历回调函数，如果该函数返回非0值，则停止遍历
 * @param from {int} 查询的结果希望是从该位置开始进行存储
 * @param limit {int} 查询的结果的最大希望个数
 * @return {int} 遍历的数据结点数值
 */
ACL_API int acl_mdb_walk(ACL_MDB *mdb, const char *tbl_name,
	int (*walk_fn)(const void*, unsigned int),
	int from, int limit);

/**
 * 数据库中某个数据表中元素总个数
 * @param mdb {ACL_MDB*} 数据库句柄
 * @param tbl_name {const char*} 数据表名
 * @return {int} >=0
 */
ACL_API int acl_mdb_cnt(ACL_MDB *mdb, const char *tbl_name);

/************************************************************************/
/*                          in acl_mdt.c                                */
/************************************************************************/

/**
 * 创建一个数据表
 * @param dbtype {const char *} 表类型: hash/avl
 * @param tbl_name {const char*} 表名
 * @param tbl_flag {unsigned int} 表的属性标志位
 * @param init_capacity {size_t} 每个内部哈希表的初始化容量
 * @param key_labels {const char *[]} 表中的各个字段名数组，最后以NULL结束
 * @param flags {unsigned int[]} 与 key_labels 相对应的约束标志位
 * @return {ACL_MDT*} 新建的数据表的句柄
 */
ACL_API ACL_MDT *acl_mdt_create(const char *dbtype, const char *tbl_name,
	unsigned int tbl_flag, size_t init_capacity,
	const char *key_labels[], unsigned int flags[]);

/**
 * 释放一个内存表
 * @param mdt {ACL_MDT*} 内存数据表句柄
 */
ACL_API void acl_mdt_free(ACL_MDT *mdt);

/**
 * 向数据表中添加一条新的数据记录
 * @param mdt {ACL_MDT*} 数据表句柄
 * @param data {void*} 用户的动态数据, 如果表的 ACL_MDT_FLAG_DMR 标志位
 *  未被设置，则将在内部拷贝一份该动态数据
 * @param dlen {unsigned int} data 的数据长度
 * @param key_labels {const char*[]} 数据表的索引字段名数组，以NULL结束
 * @param keys {const char*[]} 数据表的索引字段值数组，以NULL结束
 * @return {ACL_MDT_NOD*} 新添加的数据结点对象
 */
ACL_API ACL_MDT_NOD *acl_mdt_add(ACL_MDT *mdt, void *data,
	unsigned int dlen, const char *key_labels[], const char *keys[]);

/**
 * 探测数据表中对应的字段值是否存在
 * @param mdt {ACL_MDT*} 数据表句柄
 * @param key_label {const char*} 数据表索引字段名
 * @param key {const char*} 数据表索引字段键值
 * @return {int} 0: 不存在, != 0: 存在
 */
ACL_API int acl_mdt_probe(ACL_MDT *mdt, const char *key_label, const char *key);

/**
 * 从数据表中查询某个数据表索引键值的结果集合
 * @param mdt {ACL_MDT*} 数据表句柄
 * @param key_label {const char*} 数据表索引字段名
 * @param key {const char*} 数据表索引字段键值
 * @param from {int} 查询的结果希望是从该位置开始进行存储
 * @param limit {int} 查询的结果的最大希望个数
 * @return {ACL_MDT_REC*} 对应某个索引字段值的结果集合
 */
ACL_API ACL_MDT_RES *acl_mdt_find(ACL_MDT *mdt, const char *key_label,
	const char *key, int from, int limit);

/**
 * 从数据表中顺序列出某个范围内的所有数据结点集合
 * @param mdt {ACL_MDT*} 数据表句柄
 * @param from {int} 查询的结果希望是从该位置开始进行存储
 * @param limit {int} 查询的结果的最大希望个数
 * @return {ACL_MDT_REC*} 对应某个索引字段值的结果集合
 */
ACL_API ACL_MDT_RES *acl_mdt_list(ACL_MDT *mdt, int from, int limit);

/**
 * 从数据表中删除对应某个索引字段键值的结果集合
 * @param mdt {ACL_MDT*} 数据表句柄
 * @param key_label {const char*} 数据表索引字段名
 * @param key {const char*} 数据表索引字段键值
 * @param onfree_fn {void (*)(void*, unsigned int}
 *	用户用来释放动态数据的回调函数
 * @return {int} 所释放的数据结点的数目
 */
ACL_API int acl_mdt_delete(ACL_MDT *mdt, const char *key_label,
	const char *key, void (*onfree_fn)(void*, unsigned int));

/**
 * 遍历数据表的所有数据结点，并回调用户的处理函数
 * @param mdt {ACL_MDT*} 数据表句柄
 * @param walk_fn 回调函数，如果返回0则继续，否则停止遍历
 * @param from {int} 查询的结果希望是从该位置开始进行存储
 * @param len {int} 查询的结果的最大希望个数
 * @return {int} 所遍历的数据长度
 */
ACL_API int acl_mdt_walk(ACL_MDT *mdt, int (*walk_fn)(const void*, unsigned int),
	int from, int len);

/**
 * 从所查询的结果集合中获取下一个数据结果
 * @param res {ACL_MDT_RES*} 数据结果集合
 * @return {void*} 用户自己能够识别的动态数据，若返回NULL表示出错或已经没有数据
 */
ACL_API const void *acl_mdt_fetch_row(ACL_MDT_RES *res);

/**
 * 获得查询结果集中的记录数
 * @param res {ACL_MDT_RES*} 数据结果集合
 * @return {int} 0: 结果集为空; > 0: 结果集不为空
 */
ACL_API int acl_mdt_row_count(ACL_MDT_RES *res);

/**
 * 释放查询结果动态内存，但并不释放实际的数据结点
 * @param res {ACL_MDT_RES*} 数据结果集合
 */
ACL_API void acl_mdt_res_free(ACL_MDT_RES *res);

/**
 * 数据表中所有元素的总数
 * @param mdt {ACL_MDT*} 数据表句柄
 * @return {int} >=0
 */
ACL_API int acl_mdt_cnt(ACL_MDT *mdt);

#ifdef	__cplusplus
}
#endif

#endif

