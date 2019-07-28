#ifndef ACL_BINHASH_INCLUDE_H
#define ACL_BINHASH_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include "acl_hash.h"			/* just for ACL_HASH_FN */
#include "acl_slice.h"
#include "acl_iterator.h"

typedef struct ACL_BINHASH ACL_BINHASH;
typedef struct ACL_BINHASH_INFO ACL_BINHASH_INFO;

/**     
 * Structure of one hash table.
 */
struct ACL_BINHASH {
	int     size;                   /**< length of entries array */
	int     used;                   /**< number of entries in table */
	unsigned int flag;              /**< the hash table's properties flag */
	int     status;                 /**< the hash tables' operation status */
	ACL_BINHASH_INFO **data;        /**< entries array, auto-resized */
	ACL_SLICE *slice;               /**< memory slice */
	ACL_HASH_FN hash_fn;            /**< hash function */

	/* for acl_iterator */

	/* 取迭代器头函数 */
	void *(*iter_head)(ACL_ITER*, struct ACL_BINHASH*);
	/* 取迭代器下一个函数 */
	void *(*iter_next)(ACL_ITER*, struct ACL_BINHASH*);
	/* 取迭代器尾函数 */
	void *(*iter_tail)(ACL_ITER*, struct ACL_BINHASH*);
	/* 取迭代器上一个函数 */
	void *(*iter_prev)(ACL_ITER*, struct ACL_BINHASH*);
	/* 取迭代器关联的当前容器成员结构对象 */
	ACL_BINHASH_INFO *(*iter_info)(ACL_ITER*, struct ACL_BINHASH*);
};

/**
 * Structure of one hash table entry.
 */
struct ACL_BINHASH_INFO {
	union {
		void *key;
		const void *c_key;
	} key;				/**
					 * 哈希键, 只所以如此声明，是因为当创建哈希表的标志位为
					 * ACL_BINHASH_FLAG_KEY_REUSE 时需要复用输入的键空间
					 */
	int     key_len;                /**< 哈希键长度 */
	void   *value;                  /**< 哈希键所对应的用户数据 */
	struct ACL_BINHASH_INFO *next;  /**< colliding entry */
	struct ACL_BINHASH_INFO *prev;  /**< colliding entry */
};

/**
 * ACL_BINHASH 遍历用类型
 */
typedef struct ACL_BINHASH_ITER {
	/* public */
	ACL_BINHASH_INFO *ptr;

	/* private */
	int  i;
	int  size;
	ACL_BINHASH_INFO **h;
} ACL_BINHASH_ITER;

/**
 * 创建一个哈希表
 * @param size {int} 哈希表的初始化大小
 * @param flag {unsigned int} 哈希表属性标志位, ACL_BINHASH_FLAG_xxx
 * @return {ACL_BINHASH*} 新创建的哈希表指针
 */
ACL_API ACL_BINHASH *acl_binhash_create(int size, unsigned int flag);
#define	ACL_BINHASH_FLAG_KEY_REUSE	(1 << 0)
#define	ACL_BINHASH_FLAG_SLICE_RTGC_OFF	(1 << 1)
#define	ACL_BINHASH_FLAG_SLICE1		(1 << 2)
#define	ACL_BINHASH_FLAG_SLICE2		(1 << 3)
#define	ACL_BINHASH_FLAG_SLICE3		(1 << 4)

/**
 * 向哈希表中添加对象
 * @param table {ACL_BINHASH*} 哈希表指针
 * @param key {const void*} 哈希键
 * @param key_len {int} key 的长度
 * @param value {void*} 键值
 * @return {ACL_BINHASH_INFO*} 新创建的哈希条目指针
 */
ACL_API ACL_BINHASH_INFO *acl_binhash_enter(ACL_BINHASH *table, const void *key, int key_len, void *value);

/**
 * 从哈希表中根据键名取得对应的哈希条目
 * @param table {ACL_BINHASH*} 哈希表指针
 * @param key {const void*} 哈希键
 * @param key_len {int} key 的长度
 * @return {ACL_BINHASH_INFO*} 哈希条目指针
 */
ACL_API ACL_BINHASH_INFO *acl_binhash_locate(ACL_BINHASH *table, const void *key, int key_len);

/**
 * 查询某个哈希键的键值
 * @param table {ACL_BINHASH*} 哈希表指针
 * @param key {const void*} 哈希键
 * @param key_len {int} key 的长度
 * @return {void*} 哈希键值
 */
ACL_API void *acl_binhash_find(ACL_BINHASH *table, const void *key, int key_len);

/**
 * 删除某个哈希项
 * @param table {ACL_BINHASH*} 哈希表指针
 * @param key {const void*} 哈希键
 * @param key_len {int} key 的长度
 * @param free_fn {void (*)(void*)} 用来释放哈希键值的函数指针，如果为空则不在内部释放键值
 * @return {int} 0: ok, -1: error
 */
ACL_API int acl_binhash_delete(ACL_BINHASH *table, const void *key, int key_len, void (*free_fn) (void *));

/**
 * 释放哈希表
 * @param table {ACL_BINHASH*} 哈希表指针
 * @param free_fn {void (*)(void*)} 如果不为空，则用此函数来释放哈希表内的所有键值
 */
ACL_API void acl_binhash_free(ACL_BINHASH *table, void (*free_fn) (void *));

/**
 * 遍历整个哈希表，并用用户给出的回调函数操作哈希表中的键值
 * @param table {ACL_BINHASH*} 哈希表指针
 * @param walk_fn {void (*)(ACL_BINHASH_INFO*, void*)} 在遍历哈希表中的每个元素时的回调函数
 * @param arg {void*} 用户传递的参数，作为参数在 walk_fn 中传递
 */
ACL_API void acl_binhash_walk(ACL_BINHASH *table, void (*walk_fn) (ACL_BINHASH_INFO *, void *), void *arg);

/**
 * 列出当前哈希表中的所有元素数组列表
 * @param table {ACL_BINHASH*} 哈希表指针
 * @return {ACL_BINHASH_INFO*} 哈希表中所有元素组成的ACL_BINHASH_INFO数组, 
 *  该数组中的最后一个指针为 NULL
 */
ACL_API ACL_BINHASH_INFO **acl_binhash_list(ACL_BINHASH *table);

/**
 * 获得哈希表操作时的出错号
 * @param table {ACL_BINHASH*} 哈希表指针
 * @return {int} 错误号
 */
ACL_API int acl_binhash_errno(ACL_BINHASH *table);
#define ACL_BINHASH_STAT_OK		0
#define ACL_BINHASH_STAT_INVAL		1
#define ACL_BINHASH_STAT_DUPLEX_KEY	2
#define	ACL_BINHASH_STAT_NO_KEY		3

/**
 * 返回哈希表当前的容器空间大小
 * @param table 哈希表指针
 * @return 哈希表的容器空间大小
 */
ACL_API int acl_binhash_size(const ACL_BINHASH *table);

/**
 * 当前哈希表中对象的个数
 * @param table {ACL_BINHASH*} 哈希表指针
 * @return {int}
 */
ACL_API int acl_binhash_used(ACL_BINHASH *table);

ACL_API ACL_BINHASH_INFO **acl_binhash_data(ACL_BINHASH *table);
ACL_API const ACL_BINHASH_INFO *acl_binhash_iter_head(ACL_BINHASH *table, ACL_BINHASH_ITER *iter);
ACL_API const ACL_BINHASH_INFO *acl_binhash_iter_next(ACL_BINHASH_ITER *iter);
ACL_API const ACL_BINHASH_INFO *acl_binhash_iter_tail(ACL_BINHASH *table, ACL_BINHASH_ITER *iter);
ACL_API const ACL_BINHASH_INFO *acl_binhash_iter_prev(ACL_BINHASH_ITER *iter);

/*--------------------  一些方便快捷的宏操作 --------------------------------*/

#define	ACL_BINHASH_ITER_KEY(iter)	((iter).ptr->key.c_key)
#define	acl_binhash_iter_key		ACL_BINHASH_ITER_KEY

#define	ACL_BINHASH_ITER_VALUE(iter)	((iter).ptr->value)
#define	acl_binhash_iter_value		ACL_BINHASH_ITER_VALUE

/**
 * 遍历 ACL_BINHASH
 * @param iter {ACL_BINHASH_ITER}
 * @param table_ptr {ACL_BINHASH *}
 * @example:
	void test()
	{
		ACL_BINHASH *table = acl_binhash_create(10, 0);
		ACL_BINHASH_ITER iter;
		char *value, key[32];
		int   i;

		for (i = 0; i < 100; i++) {
			value = (char*) acl_mystrdup("value");
			snprintf(key, sizeof(key), "key:%d", i);
			(void) acl_binhash_enter(table, key, strlen(key), value);
		}

		acl_binhash_foreach(iter, table) {
			printf("%s=%s\n", iter.ptr->key.c_key, iter.ptr->value);
			if (i == 50)
				break;
		}

		acl_binhash_free(table, acl_myfree_fn);
	}
 */
#if 0
#define	ACL_BINHASH_FOREACH(iter, table_ptr)  \
    if (table_ptr)  \
        for((iter).size = acl_binhash_size((table_ptr)), (iter).i = 0,  \
          (iter).h = acl_binhash_data((table_ptr)); (iter).i < (iter).size; (iter).i++)  \
            for ((iter).ptr = *(iter).h++; (iter).ptr; (iter).ptr = (iter).ptr->next)
#define	ACL_BINHASH_FOREACH_REVERSE(iter, table_ptr)  \
    if (table_ptr)  \
        for((iter).size = acl_binhash_size((table_ptr)), (iter).i = (iter).size - 1,  \
          (iter).h = acl_binhash_data((table_ptr)) + (iter).i; (iter).i >= 0; (iter).i--)  \
            for ((iter).ptr = *(iter).h--; (iter).ptr; (iter).ptr = (iter).ptr->next)
#else
#define	ACL_BINHASH_FOREACH(iter, table_ptr)  \
    if (table_ptr)  \
        for((void) acl_binhash_iter_head((table_ptr), &iter);  \
            (iter).ptr;  \
            (void) acl_binhash_iter_next(&iter))
#define	ACL_BINHASH_FOREACH_REVERSE(iter, table_ptr)  \
    if (table_ptr)  \
        for((void) acl_binhash_iter_tail((table_ptr), &iter);  \
            (iter).ptr;  \
            (void) acl_binhash_iter_prev(&iter))
#endif

#define	acl_binhash_foreach		ACL_BINHASH_FOREACH
#define	acl_binhash_foreach_reverse	ACL_BINHASH_FOREACH_REVERSE

#ifdef  __cplusplus
}
#endif

#endif

