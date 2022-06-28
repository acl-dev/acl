#ifndef ACL_HTABLE_INCLUDE_H
#define ACL_HTABLE_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include "../thread/acl_thread.h"
#include "acl_hash.h"			/* just for ACL_HASH_FN */
#include "acl_slice.h"
#include "acl_iterator.h"

/*--------------------------------------------------------------------------*/
typedef struct ACL_HTABLE	ACL_HTABLE;
typedef struct ACL_HTABLE_INFO 	ACL_HTABLE_INFO;

/**
 * 哈希表对象结构句柄
 */
struct ACL_HTABLE {
	int     size;                   /* length of entries array */
	int     init_size;              /* length of initial entryies array */
	int     used;                   /* number of entries in table */
	ACL_HTABLE_INFO **data;         /* entries array, auto-resized */
	unsigned int flag;              /* properties flag */
	int     status;                 /* the operator's status on the htable */

	ACL_HASH_FN  hash_fn;           /* hash function */
	ACL_SLICE_POOL *slice;
	acl_pthread_mutex_t *rwlock;

	/* for acl_iterator */

	/* 取迭代器头函数 */
	void *(*iter_head)(ACL_ITER*, struct ACL_HTABLE*);
	/* 取迭代器下一个函数 */
	void *(*iter_next)(ACL_ITER*, struct ACL_HTABLE*);
	/* 取迭代器尾函数 */
	void *(*iter_tail)(ACL_ITER*, struct ACL_HTABLE*);
	/* 取迭代器上一个函数 */
	void *(*iter_prev)(ACL_ITER*, struct ACL_HTABLE*);
	/* 取迭代器关联的当前容器成员结构对象 */
	ACL_HTABLE_INFO *(*iter_info)(ACL_ITER*, struct ACL_HTABLE*);
};

/**
 * 哈希表中每一个哈希项的存储信息类型
 */
struct ACL_HTABLE_INFO {
	/**
	 * 哈希键, 只所以如此声明，是因为当创建哈希表的标志位为
	 * ACL_BINHASH_FLAG_KEY_REUSE 时需要复用输入的键空间
	 */
	union {
		char *key;
		const char *c_key;
	} key;
	void   *value;			/**< associated value */
	unsigned hash;			/**< store the key's hash value */
	struct ACL_HTABLE_INFO *next;	/**< colliding entry */
	struct ACL_HTABLE_INFO *prev;	/**< colliding entry */
};

/**
 * ACL_HTABLE 遍历用类型
 */
typedef struct ACL_HTABLE_ITER {
	/* public */
	ACL_HTABLE_INFO *ptr;

	/* private */
	int  i;
	int  size;
	ACL_HTABLE_INFO **h;
} ACL_HTABLE_ITER;

/**
 * 建立哈希表
 * @param size 哈希表长度
 * @param flag {unsigned int} 哈希表属性标志位, ACL_BINHASH_FLAG_xxx
 * @return 所建哈希表的头指针或为空(这时表示出了严重的错误, 主要是内存分配问题)
 */
ACL_API ACL_HTABLE *acl_htable_create(int size, unsigned int flag);
/* 添加新的对象时是否直接复用键地址 */
#define	ACL_HTABLE_FLAG_KEY_REUSE	(1 << 0)

/* 是否针对启用多线程互斥方式 */
#define	ACL_HTABLE_FLAG_USE_LOCK	(1 << 1)

/* 每次查询时是否将查询结果对象放在链头 */
#define	ACL_HTABLE_FLAG_MSLOOK		(1 << 2)

/* 统一将键转换为小写，从而实现键查询不区分大小写的功能 */
#define	ACL_HTABLE_FLAG_KEY_LOWER	(1 << 3)

ACL_API ACL_HTABLE *acl_htable_create3(int size, unsigned int flag,
		ACL_SLICE_POOL *slice);

/**
 * 设置哈希表的控制参数
 * @param table 哈希表对象句柄
 * @param name 控制参数的变参初始值, name 及以后的控制参数如下定义
 *  ACL_HTABLE_CTL_END: 变参表结束标志
 *  ACL_HTABLE_CTL_RWLOCK: 是否启用读写锁机制
 *  ACL_HTABLE_CTL_HASH_FN: 用户自定义的哈希值计算函数
 */
ACL_API void acl_htable_ctl(ACL_HTABLE *table, int name, ...);
#define	ACL_HTABLE_CTL_END      0  /**< 控制结束标志 */
#define	ACL_HTABLE_CTL_RWLOCK   1  /**< 是否加锁 */
#define	ACL_HTABLE_CTL_HASH_FN  2  /**< 设置私有哈希函数 */

/**
 * 检查上一次哈希表操作后哈希表的状态
 * @param table 哈希表指针
 * @return {int} 操作哈希表后的状态, 参见如下的 ACL_HTABLE_STAT_XXX
 */
ACL_API int acl_htable_errno(ACL_HTABLE *table);
#define	ACL_HTABLE_STAT_OK          0  /**< 状态正常 */
#define	ACL_HTABLE_STAT_INVAL       1  /**< 无效参数 */
#define	ACL_HTABLE_STAT_DUPLEX_KEY  2  /**< 重复键 */

/**
 * 设置哈希表的当前状态, error 取值 ACL_HTABLE_STAT_XXX
 * @param table 哈希表指针
 * @param error 设置哈希表的错误状态
 */
ACL_API void acl_htable_set_errno(ACL_HTABLE *table, int error);

/**
 * 往哈希表里添加新的项
 * @param table 哈希表指针
 * @param key 键, 在函数内部会复制此 key 键
 * @param value 用户自己的特定数据项(可以由类型硬转化而来, 但是此数据项必须
 *  不能堆栈变量)
 * @return 所分配的哈希表项的指针, == NULL: 表示内部分配内存出错, 为严重的错误
 *  注：如果在添加时该哈希争键存在，则返回已经存在的哈希项，使用者应该通过调用
 *  acl_htable_last_errno() 来查看是否重复添加同一个键值(ACL_HTABLE_STAT_DUPLEX_KEY)
 */
ACL_API ACL_HTABLE_INFO *acl_htable_enter(ACL_HTABLE *table,
		const char *key, void *value);

/**
 * 往哈希表里添加新的项，当多个线程同时进行此操作时，函数内部会自动保证互斥操作
 * @param table 哈希表指针
 * @param key 键, 在函数内部会复制此 key 键
 * @param value 用户自己的特定数据项(可以由类型硬转化而来, 但是此数据项必须
 *  不能堆栈变量)
 * @param callback 如果该函数指针不为空，则当添加成功后便调用该函数
 * @param arg callback 的参数之一
 * @return {int} 0 表示 添加成功，-1 表示添加失败
 *  注：如果在添加时该哈希争键存在，则返回已经存在的哈希项，使用者应该通过调用
 *  acl_htable_last_errno() 来查看是否重复添加同一个键值(ACL_HTABLE_STAT_DUPLEX_KEY)
 */
ACL_API int acl_htable_enter_r(ACL_HTABLE *table, const char *key, void *value,
		void (*callback)(ACL_HTABLE_INFO *ht, void *arg), void *arg);

/**
 * 由所给的 key 键查寻某一特定哈希项
 * @param table 哈希表指针
 * @param key 键
 * @return 不为空指针: 表示查到了对应于 key 键的哈希项
 *         为空: 表示未查到对应于 key 键的哈希项
 */
ACL_API ACL_HTABLE_INFO *acl_htable_locate(ACL_HTABLE *table, const char *key);

/**
 * 由所给的 key 键查寻某一特定哈希项，当多个线程同时进行此操作时，
 * 函数内部会自动保证互斥操作
 * @param table 哈希表指针
 * @param key 键
 * @param callback 查到所要求的键值后如果该指针非空则调用之
 * @param arg callback 参数之一
 * @return 不为空指针: 表示查到了对应于 key 键的哈希项
 *         为空: 表示未查到对应于 key 键的哈希项
 */
ACL_API int acl_htable_locate_r(ACL_HTABLE *table, const char *key,
		void (*callback)(ACL_HTABLE_INFO *ht, void *arg), void *arg);

/**
 * 由所给的 key 键查寻用户的数据项
 * @param table 哈希表指针
 * @param key 键
 * @return 不为空: 表示查到了对应于 key 键的数据项, 用户可以根据用户自己的
 *  数据类型进行转换; 为空: 表示未查到对应于 key 键的数据项
 */
ACL_API void *acl_htable_find(ACL_HTABLE *table, const char *key);

/**
 * 由所给的 key 键查寻用户的数据项, 当多个线程同时进行此操作时，
 * 函数内部会自动保证互斥操作
 * @param table 哈希表指针
 * @param key 键
 * @param callback 当查到所要求的键值后，如果该函数指针不为空则调用之
 * @param arg callback 的参数之一
 * @return 不为空: 表示查到了对应于 key 键的数据项, 用户可以根据用户自己的
 *  数据类型进行转换; 为空: 表示未查到对应于 key 键的数据项
 */
ACL_API int  acl_htable_find_r(ACL_HTABLE *table, const char *key,
		void (*callback)(void *value, void *arg), void *arg);

/**
 * 根据所给的 key 键删除某一哈希项
 * @param table 哈希表指针
 * @param key 键
 * @param free_fn 如果该函数指针不为空并且找到了对应于 key 键的数据项, 则先
 *  调用用户所提供的析构函数做一些清尾工作, 然后再释放该哈希项
 * @return 0: 成功;  -1: 未找到该 key 键
 */
ACL_API int acl_htable_delete(ACL_HTABLE *table, const char *key,
		void (*free_fn) (void *));
#define	acl_htable_delete_r	acl_htable_delete

/**
 * 直接根据 acl_htable_locate 返回的非空对象从哈希表中删除该对象
 * @param table 哈希表指针
 * @param ht {ACL_HTABLE_INFO*} 存储于哈希表中的内部结构对象
 * @param free_fn 如果该函数指针不为空并且找到了对应于 key 键的数据项, 则先
 *  调用用户所提供的析构函数做一些清尾工作, 然后再释放该哈希项
 */
ACL_API void acl_htable_delete_entry(ACL_HTABLE *table, ACL_HTABLE_INFO *ht,
		void (*free_fn) (void *));

/**
 * 释放整个哈希表
 * @param table 哈希表指针
 * @param free_fn 如果该指针不为空则对哈希表中的每一项哈希项先用该函数做
 *  清尾工作, 然后再释放
 */
ACL_API void acl_htable_free(ACL_HTABLE *table, void (*free_fn) (void *));

/**
 * 重置哈希表, 该函数会释放哈希表中的所有内容项, 并重新初始化
 * @param table 哈希表指针
 * @param free_fn 如果该指针不为空则对哈希表中的每一项哈希项先用该函数做
 *  清尾工作, 然后再释放
 * @return 是否重置成功. 0: OK; < 0: error.
 */
ACL_API int acl_htable_reset(ACL_HTABLE *table, void (*free_fn) (void *));
#define	acl_htable_reset_r	acl_htable_reset

/**
 * 对哈希表中的每一项哈希项进行处理
 * @param table 哈希表指针
 * @param walk_fn 处理每一项哈希项的函数指针, 不能为空
 * @param arg 用户自己类型的数据
 */
ACL_API void acl_htable_walk(ACL_HTABLE *table,
		void (*walk_fn) (ACL_HTABLE_INFO *, void *), void *arg);
#define	acl_htable_walk_r	acl_htable_walk

/**
 * 返回哈希表当前的容器空间大小
 * @param table 哈希表指针
 * @return 哈希表的容器空间大小
 */
ACL_API int acl_htable_size(const ACL_HTABLE *table);

/**
 * 返回哈希表当前的窗口中所含元素个数
 * @param table 哈希表指针
 * @return 哈希表中元素个数
 */
ACL_API int acl_htable_used(const ACL_HTABLE *table);

/**
 * 将哈希表里的所有项组合成一个链表
 * @param table 哈希表
 * @return 不为空: 链表指针; 为空: 表示该哈希表里没有哈希项
 */
ACL_API ACL_HTABLE_INFO **acl_htable_list(const ACL_HTABLE *table);

/**
 * 显示哈希表中 key 键的分布状态
 * @param table 哈希表指针
 */
ACL_API void acl_htable_stat(const ACL_HTABLE *table);
#define	acl_htable_stat_r	acl_htable_stat

ACL_API ACL_HTABLE_INFO **acl_htable_data(ACL_HTABLE *table);
ACL_API const ACL_HTABLE_INFO *acl_htable_iter_head(
		ACL_HTABLE *table, ACL_HTABLE_ITER *iter);
ACL_API const ACL_HTABLE_INFO *acl_htable_iter_next(ACL_HTABLE_ITER *iter);
ACL_API const ACL_HTABLE_INFO *acl_htable_iter_tail(
		ACL_HTABLE *table, ACL_HTABLE_ITER *iter);
ACL_API const ACL_HTABLE_INFO *acl_htable_iter_prev(ACL_HTABLE_ITER *iter);

/*--------------------  一些方便快捷的宏操作 --------------------------------*/

#define	ACL_HTABLE_ITER_KEY(iter)	((iter).ptr->key.c_key)
#define	acl_htable_iter_key		ACL_HTABLE_ITER_KEY

#define	ACL_HTABLE_ITER_VALUE(iter)	((iter).ptr->value)
#define	acl_htable_iter_value		ACL_HTABLE_ITER_VALUE

/**
 * 遍历 ACL_HTABLE
 * @param iter {ACL_HTABLE_ITER}
 * @param table_ptr {ACL_HTABLE *}
 * @example:
	void test()
	{
		ACL_HTABLE *table = acl_htable_create(10, 0);
		ACL_HTABLE_ITER iter;
		char *value, key[32];
		int   i;

		for (i = 0; i < 100; i++) {
			value = (char*) acl_mystrdup("value");
			snprintf(key, sizeof(key), "key:%d", i);
			(void) acl_htable_enter(table, key, value);
		}

		acl_htable_foreach(iter, table) {
			printf("%s=%s\n", acl_htable_iter_key(iter),
				acl_htable_iter_value(iter));
			if (i == 50)
				break;
		}
		acl_htable_free(table, acl_myfree_fn);
	}
 */

#if 0
#define	ACL_HTABLE_FOREACH(iter, table_ptr)  \
    if (table_ptr)  \
        for((iter).size = acl_htable_size((table_ptr)), (iter).i = 0,  \
          (iter).h = acl_htable_data((table_ptr)); (iter).i < (iter).size; (iter).i++)  \
            for ((iter).ptr = *(iter).h++; (iter).ptr; (iter).ptr = (iter).ptr->next)
#define	ACL_HTABLE_FOREACH_REVERSE(iter, table_ptr)  \
    if (table_ptr)  \
        for((iter).size = acl_htable_size((table_ptr)), (iter).i = (iter).size - 1,  \
          (iter).h = acl_htable_data((table_ptr)) + (iter).i; (iter).i >= 0; (iter).i--)  \
            for ((iter).ptr = *(iter).h--; (iter).ptr; (iter).ptr = (iter).ptr->next)
#else
#define	ACL_HTABLE_FOREACH(iter, table_ptr)  \
    if (table_ptr)  \
        for((void) acl_htable_iter_head((table_ptr), &iter);  \
            (iter).ptr;  \
            (void) acl_htable_iter_next(&iter))
#define	ACL_HTABLE_FOREACH_REVERSE(iter, table_ptr)  \
    if (table_ptr)  \
        for((void) acl_htable_iter_tail((table_ptr), &iter);  \
            (iter).ptr;  \
            (void) acl_htable_iter_prev(&iter))
#endif

#define	acl_htable_foreach		ACL_HTABLE_FOREACH
#define	acl_htable_foreach_reverse	ACL_HTABLE_FOREACH_REVERSE

#ifdef  __cplusplus
}
#endif

#endif

