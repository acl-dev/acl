
#ifndef __HTABLE_H_INCLUDED__
#define __HTABLE_H_INCLUDED__

#ifdef  __cplusplus
extern "C" {
#endif

#include "private.h"

#define	HAS_VALUE

/*--------------------------------------------------------------------------*/
/**
 * 哈希函数类型定义
 * @param buffer 需要被哈希的字符串
 * @param len s 的长度
 */
typedef unsigned (*HASH_FN)(const void *buffer, size_t len);

/**
 * 哈希表对象结构句柄, 该类型定义在 htable.c 中是为了保护内部成员变量
 */
typedef struct HTABLE	HTABLE;

/**
 * 哈希表中每一个哈希项的存储信息类型
 */
#ifdef  PACK_STRUCT
#pragma pack(4)
#endif
typedef struct HTABLE_INFO {
	union {
		char   *key;
		const char *c_key;
	} key;				/**< lookup key */
#ifdef	HAS_VALUE
	char   *value;			/**< associated value */
#endif
	struct HTABLE_INFO *next;	/**< colliding entry */
	struct HTABLE_INFO *prev;	/**< colliding entry */
} HTABLE_INFO;
#ifdef  PACK_STRUCT
#pragma pack(0)
#endif

/**
 * 建立哈希表
 * @param size 哈希表长度
 * @param flag 与 ACL_MDT_IDX 中的 flag 相同
 * @return 所建哈希表的头指针或为空(这时表示出了严重的错误, 主要是内存分配问题)
 */
HTABLE *htable_create(int size, unsigned int flag, int use_slice);

/**
 * 设置哈希表的控制参数
 * @param table 哈希表对象句柄
 * @param name 控制参数的变参初始值, name 及以后的控制参数如下定义
 *  HTABLE_CTL_END: 变参表结束标志
 *  HTABLE_CTL_RWLOCK: 是否启用读写锁机制
 *  HTABLE_CTL_HASH_FN: 用户自定义的哈希值计算函数
 */
void htable_ctl(HTABLE *table, int name, ...);
#define	HTABLE_CTL_END      0  /**< 控制结束标志 */
#define	HTABLE_CTL_HASH_FN  2  /**< 设置私有哈希函数 */

/**
 * 检查上一次哈希表操作后哈希表的状态
 * @param table 哈希表指针
 * @return {int} 操作哈希表后的状态, 参见如下的 HTABLE_STAT_XXX
 */
int htable_last_errno(HTABLE *table);
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
 * @param value 用户自己的特定数据项(可以由类型硬转化而来, 但是此数据项必须不能堆栈变量)
 * @return 所分配的哈希表项的指针, == NULL: 表示内部分分配内存出错, 为严重的错误
 *  注：如果在添加时该哈希争键存在，则返回已经存在的哈希项，使用者应该通过调用
 *  htable_last_errno() 来查看是否重复添加同一个键值(HTABLE_STAT_DUPLEX_KEY)
 */
HTABLE_INFO *htable_enter(HTABLE *table, const char *key, char *value);

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
char *htable_find(HTABLE *table, const char *key);

/**
 * 根据所给的 key 键删除某一哈希项
 * @param table 哈希表指针
 * @param key 键
 * @param free_fn 如果该函数指针不为空并且找到了对应于 key 键的数据项, 则先调用用户
 *        所提供的析构函数做一些清尾工作, 然后再释放该哈希项
 * @return 0: 成功;  -1: 未找到该 key 键
 */
int htable_delete(HTABLE *table, const char *key, void (*free_fn) (char *));

/**
 * 释放整个哈希表
 * @param table 哈希表指针
 * @param free_fn 如果该指针不为空则对哈希表中的每一项哈希项先用该函数做清尾工作, 然后再释放
 */
void htable_free(HTABLE *table, void (*free_fn) (char *));

/**
 * 对哈希表中的每一项哈希项进行处理
 * @param table 哈希表指针
 * @param walk_fn 处理每一项哈希项的函数指针, 不能为空
 * @param arg 用户自己类型的数据
 */
void htable_walk(HTABLE *table, void (*walk_fn) (HTABLE_INFO *, char *), char *arg);

/**
 * 返回哈希表当前的容器空间大小(该值应大于哈希表中元素个数)
 * @param table 哈希表指针
 * @return 哈希表的容器空间大小
 */
int htable_capacity(const HTABLE *table);

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

void htable_list_free(HTABLE_INFO ** list);

/**
 * 显示哈希表中 key 键的分布状态
 * @param table 哈希表指针
 */
void htable_stat(const HTABLE *table);

#ifdef  __cplusplus
}
#endif

#endif

