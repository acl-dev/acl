#ifndef HTABLE_INCLUDE_H
#define HTABLE_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "iterator.h"

/*--------------------------------------------------------------------------*/
typedef struct HTABLE		HTABLE;
typedef struct HTABLE_INFO 	HTABLE_INFO;

/**
 * 哈希表对象结构句柄
 */
struct HTABLE {
	int     size;                   /* length of entries array */
	int     init_size;              /* length of initial entryies array */
	int     used;                   /* number of entries in table */
	HTABLE_INFO **data;             /* entries array, auto-resized */
	int     status;                 /* the operator's status on the htable */

	/* for iterator */

	/* 取迭代器头函数 */
	void *(*iter_head)(ITER*, struct HTABLE*);
	/* 取迭代器下一个函数 */
	void *(*iter_next)(ITER*, struct HTABLE*);
	/* 取迭代器尾函数 */
	void *(*iter_tail)(ITER*, struct HTABLE*);
	/* 取迭代器上一个函数 */
	void *(*iter_prev)(ITER*, struct HTABLE*);
	/* 取迭代器关联的当前容器成员结构对象 */
	HTABLE_INFO *(*iter_info)(ITER*, struct HTABLE*);
};

/**
 * 哈希表中每一个哈希项的存储信息类型
 */
struct HTABLE_INFO {
	char *key;
	void *value;			/**< associated value */
	unsigned hash;			/**< store the key's hash value */
	struct HTABLE_INFO *next;	/**< colliding entry */
	struct HTABLE_INFO *prev;	/**< colliding entry */
};

/**
 * 建立哈希表
 * @param size 哈希表长度
 * @return 所建哈希表的头指针或为空(这时表示出了严重的错误, 主要是内存分配问题)
 */
HTABLE *htable_create(int size);

/**
 * 检查上一次哈希表操作后哈希表的状态
 * @param table 哈希表指针
 * @return {int} 操作哈希表后的状态, 参见如下的 HTABLE_STAT_XXX
 */
int htable_errno(HTABLE *table);
#define	HTABLE_STAT_OK          0  /**< 状态正常 */
#define	HTABLE_STAT_INVAL       1  /**< 无效参数 */
#define	HTABLE_STAT_DUPLEX_KEY  2  /**< 重复键 */

/**
 * 设置哈希表的当前状态, error 取值 HTABLE_STAT_XXX
 * @param table 哈希表指针
 * @param error 设置哈希表的错误状态
 */
void htable_set_errno(HTABLE *table, int error);

/**
 * 往哈希表里添加新的项
 * @param table 哈希表指针
 * @param key 键, 在函数内部会复制此 key 键
 * @param value 用户自己的特定数据项(可以由类型硬转化而来, 但是此数据项必须
 *  不能堆栈变量)
 * @return 所分配的哈希表项的指针, == NULL: 表示内部分配内存出错, 为严重的错误
 *  注：如果在添加时该哈希争键存在，则返回已经存在的哈希项，使用者应该通过调用
 *  htable_last_errno() 来查看是否重复添加同一个键值(HTABLE_STAT_DUPLEX_KEY)
 */
HTABLE_INFO *htable_enter(HTABLE *table, const char *key, void *value);

/**
 * 由所给的 key 键查寻某一特定哈希项
 * @param table 哈希表指针
 * @param key 键
 * @return 不为空指针: 表示查到了对应于 key 键的哈希项
 *         为空: 表示未查到对应于 key 键的哈希项
 */
HTABLE_INFO *htable_locate(HTABLE *table, const char *key);

/**
 * 由所给的 key 键查寻用户的数据项
 * @param table 哈希表指针
 * @param key 键
 * @return 不为空: 表示查到了对应于 key 键的数据项, 用户可以根据用户自己的
 *  数据类型进行转换; 为空: 表示未查到对应于 key 键的数据项
 */
void *htable_find(HTABLE *table, const char *key);

/**
 * 根据所给的 key 键删除某一哈希项
 * @param table 哈希表指针
 * @param key 键
 * @param free_fn 如果该函数指针不为空并且找到了对应于 key 键的数据项, 则先
 *  调用用户所提供的析构函数做一些清尾工作, 然后再释放该哈希项
 * @return 0: 成功;  -1: 未找到该 key 键
 */
int htable_delete(HTABLE *table, const char *key, void (*free_fn) (void *));

/**
 * 直接根据 htable_locate 返回的非空对象从哈希表中删除该对象
 * @param table 哈希表指针
 * @param ht {HTABLE_INFO*} 存储于哈希表中的内部结构对象
 * @param free_fn 如果该函数指针不为空并且找到了对应于 key 键的数据项, 则先
 *  调用用户所提供的析构函数做一些清尾工作, 然后再释放该哈希项
 */
void htable_delete_entry(HTABLE *table, HTABLE_INFO *ht, void (*free_fn) (void *));

/**
 * 释放整个哈希表
 * @param table 哈希表指针
 * @param free_fn 如果该指针不为空则对哈希表中的每一项哈希项先用该函数做
 *  清尾工作, 然后再释放
 */
void htable_free(HTABLE *table, void (*free_fn) (void *));

/**
 * 重置哈希表, 该函数会释放哈希表中的所有内容项, 并重新初始化
 * @param table 哈希表指针
 * @param free_fn 如果该指针不为空则对哈希表中的每一项哈希项先用该函数做
 *  清尾工作, 然后再释放
 * @return 是否重置成功. 0: OK; < 0: error.
 */
int htable_reset(HTABLE *table, void (*free_fn) (void *));

/**
 * 对哈希表中的每一项哈希项进行处理
 * @param table 哈希表指针
 * @param walk_fn 处理每一项哈希项的函数指针, 不能为空
 * @param arg 用户自己类型的数据
 */
void htable_walk(HTABLE *table, void (*walk_fn) (HTABLE_INFO *, void *), void *arg);

/**
 * 返回哈希表当前的容器空间大小
 * @param table 哈希表指针
 * @return 哈希表的容器空间大小
 */
int htable_size(const HTABLE *table);

/**
 * 返回哈希表当前的窗口中所含元素个数
 * @param table 哈希表指针
 * @return 哈希表中元素个数
 */
int htable_used(const HTABLE *table);

/**
 * 将哈希表里的所有项组合成一个链表
 * @param table 哈希表
 * @return 不为空: 链表指针; 为空: 表示该哈希表里没有哈希项
 */
HTABLE_INFO **htable_list(const HTABLE *table);

/**
 * 显示哈希表中 key 键的分布状态
 * @param table 哈希表指针
 */
void htable_stat(const HTABLE *table);
#define	htable_stat_r	htable_stat

/*--------------------  一些方便快捷的宏操作 --------------------------------*/

#define	HTABLE_ITER_KEY(iter)	((iter).ptr->key.c_key)
#define	htable_iter_key		HTABLE_ITER_KEY

#define	HTABLE_ITER_VALUE(iter)	((iter).ptr->value)
#define	htable_iter_value	HTABLE_ITER_VALUE

#ifdef  __cplusplus
}
#endif

#endif
