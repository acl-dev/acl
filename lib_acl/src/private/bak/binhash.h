#ifndef __BINHASH_H_INCLUDED_H__
#define __BINHASH_H_INCLUDED_H__

#ifdef  __cplusplus
extern "C" {
#endif

#include "private.h"

typedef struct BINHASH BINHASH;

/**
 * Structure of one hash table entry.
 */
#ifdef  PACK_STRUCT
#pragma pack(4)
#endif
typedef struct BINHASH_INFO {
	union {
		char *key;
		const char *c_key;
	} key;                      /**< lookup key */
	int     key_len;            /**< key length */
	char   *value;              /**< associated value */
	struct BINHASH_INFO *next;  /**< colliding entry */
	struct BINHASH_INFO *prev;  /**< colliding entry */
} BINHASH_INFO;
#ifdef  PACK_STRUCT
#pragma pack(0)
#endif

/**
 * 创建一个哈希表
 * @param size {int} 哈希表的初始化大小
 * @param flag {unsigned int} 与 ACL_MDT_IDX 中的 flag 相同
 * @return {BINHASH*} 新创建的哈希表指针
 */
BINHASH *binhash_create(int size, unsigned int flag, int use_slice);

/**
 * 向哈希表中添加对象
 * @param table {BINHASH*} 哈希表指针
 * @param key {const char*} 哈希键
 * @param key_len {int} key 的长度
 * @param value {char*} 键值
 * @return {BINHASH_INFO*} 新创建的哈希条目指针
 */
BINHASH_INFO *binhash_enter(BINHASH *table, const char *key, int key_len, char *value);

/**
 * 从哈希表中根据键名取得对应的哈希条目
 * @param table {BINHASH*} 哈希表指针
 * @param key {const char*} 哈希键
 * @param key_len {int} key 的长度
 * @return {BINHASH_INFO*} 哈希条目指针
 */
BINHASH_INFO *binhash_locate(BINHASH *table, const char *key, int key_len);

/**
 * 查询某个哈希键的键值
 * @param table {BINHASH*} 哈希表指针
 * @param key {const char*} 哈希键
 * @param key_len {int} key 的长度
 * @return {char*} 哈希键值
 */
char *binhash_find(BINHASH *table, const char *key, int key_len);

/**
 * 删除某个哈希项
 * @param table {BINHASH*} 哈希表指针
 * @param key {const char*} 哈希键
 * @param key_len {int} key 的长度
 * @param free_fn {void (*)(char*)} 用来释放哈希键值的函数指针，如果为空则不在内部释放键值
 */
void binhash_delete(BINHASH *table, const char *key, int key_len, void (*free_fn) (char *));

/**
 * 释放哈希表
 * @param table {BINHASH*} 哈希表指针
 * @param free_fn {void (*)(char*)} 如果不为空，则用此函数来释放哈希表内的所有键值
 */
void binhash_free(BINHASH *table, void (*free_fn) (char *));

/**
 * 遍历整个哈希表，并用用户给出的回调函数操作哈希表中的键值
 * @param table {BINHASH*} 哈希表指针
 * @param walk_fn {void (*)(BINHASH_INFO*, char*)} 在遍历哈希表中的每个元素时的回调函数
 * @param arg {char*} 用户传递的参数，作为参数在 walk_fn 中传递
 */
void binhash_walk(BINHASH *table, void (*walk_fn) (BINHASH_INFO *, char *), char *arg);

/**
 * 列出当前哈希表中的所有元素数组列表
 * @param table {BINHASH*} 哈希表指针
 * @return {BINHASH_INFO*} 哈希表中所有元素组成的BINHASH_INFO数组, 
 *  该数组中的最后一个指针为 NULL
 */
BINHASH_INFO **binhash_list(BINHASH *table);

#ifdef  __cplusplus
}
#endif

#endif

