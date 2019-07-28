
#ifndef _DEBUG_HTABLE_H_INCLUDED_
#define _DEBUG_HTABLE_H_INCLUDED_

#ifdef  __cplusplus
extern "C" {
#endif

#include "stdlib/acl_define.h"

/*--------------------------------------------------------------------------*/

/* Structure of one hash table entry. */

/**
 * 哈希函数类型定义
 * @param buffer 需要被哈希的字符串
 * @param len s 的长度
 */
typedef unsigned (*DEBUG_HASH_FN)(const void *buffer, size_t len);

/**
 * 哈希表对象结构句柄, 该类型定义在 htable.c 中是为了保护内部成员变量
 */
typedef struct DEBUG_HTABLE	DEBUG_HTABLE;

/* 哈希表中每一个哈希项的存储信息类型 */
typedef struct DEBUG_HTABLE_INFO {
	char   *key;			/* lookup key */
	char   *value;			/* associated value */
	struct DEBUG_HTABLE_INFO *next;	/* colliding entry */
	struct DEBUG_HTABLE_INFO *prev;	/* colliding entry */
} DEBUG_HTABLE_INFO;

/**
 * 建立哈希表
 * @param size 哈希表长度
 * @return 所建哈希表的头指针或为空(这时表示出了严重的错误, 主要是内存分配问题)
 */
DEBUG_HTABLE *debug_htable_create(int size);

/**
 * 往哈希表里添加新的项
 * @param table 哈希表指针
 * @param key 键, 在函数内部会复制此 key 键
 * @param value 用户自己的特定数据项(可以由类型硬转化而来, 但是此数据项必须不能堆栈变量)
 * @return 所分配的哈希表项的指针, == NULL: 表示内部分分配内存出错, 为严重的错误
 */
DEBUG_HTABLE_INFO *debug_htable_enter(DEBUG_HTABLE *table, const char *key, char *value);

/**
 * 由所给的 key 键查寻某一特定哈希项
 * @param table 哈希表指针
 * @param key 键
 * @return 不为空指针: 表示查到了对应于 key 键的哈希项
 *         为空: 表示未查到对应于 key 键的哈希项
 */
DEBUG_HTABLE_INFO *debug_htable_locate(DEBUG_HTABLE *table, const char *key);

/**
 * 由所给的 key 键查寻用户的数据项
 * @param table 哈希表指针
 * @param key 键
 * @return 不为空: 表示查到了对应于 key 键的数据项, 用户可以根据用户自己的数据类型进行转换
 *         为空: 表示未查到对应于 key 键的数据项
 */
char *debug_htable_find(DEBUG_HTABLE *table, const char *key);

/**
 * 功能:		根据所给的 key 键删除某一哈希项
 * @param table 哈希表指针
 * @param key 键
 * @param free_fn 如果该函数指针不为空并且找到了对应于 key 键的数据项, 则先调用用户
 *        所提供的析构函数做一些清尾工作, 然后再释放该哈希项
 * @return 0: 成功;  -1: 未找到该 key 键
 */
int debug_htable_delete(DEBUG_HTABLE *table, const char *key, void (*free_fn) (char *));

/**
 * 释放整个哈希表
 * @param table 哈希表指针
 * @param free_fn 如果该指针不为空则对哈希表中的每一项哈希项先用该函数做清尾工作, 然后再释放
 */
void debug_htable_free(DEBUG_HTABLE *table, void (*free_fn) (char *));

/**
 * 重置哈希表, 该函数会释放哈希表中的所有内容项, 并重新初始化
 * @param table 哈希表指针
 * @param free_fn 如果该指针不为空则对哈希表中的每一项哈希项先用该函数做清尾工作, 然后再释放
 * @return 是否重置成功. 0: OK; < 0: error.
 */
int debug_htable_reset(DEBUG_HTABLE *table, void (*free_fn) (char *));

/**
 * 对哈希表中的每一项哈希项进行处理
 * @param table 哈希表指针
 * @param walk_fn 处理每一项哈希项的函数指针, 不能为空
 * @param arg 用户自己类型的数据
 */
void debug_htable_walk(DEBUG_HTABLE *table, void (*walk_fn) (DEBUG_HTABLE_INFO *, char *), char *arg);

/**
 * 将哈希表里的所有项组合成一个链表
 * @param table 哈希表
 * @return 不为空: 链表指针; 为空: 表示该哈希表里没有哈希项
 */
DEBUG_HTABLE_INFO **debug_htable_list(const DEBUG_HTABLE *table);

#ifdef  __cplusplus
}
#endif

#endif

