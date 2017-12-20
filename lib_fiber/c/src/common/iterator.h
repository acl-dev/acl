#ifndef	__ITERATOR_INCLUDE_H__
#define	__ITERATOR_INCLUDE_H__

typedef struct ITER ITER;

/**
 * ACL 库中数据结构用的通用迭代器结构定义
 */
struct ITER {
	void *ptr;		/**< 迭代器指针, 与容器相关 */
	void *data;		/**< 用户数据指针 */
	int   dlen;		/**< 用户数据长度, 实现者可设置此值也可不设置 */
	const char *key;	/**< 若为哈希表的迭代器, 则为哈希键值地址 */
	int   klen;		/**< 若为BINHASH迭代器, 则为键长度 */
	int   i;		/**< 当前迭代器在容器中的位置索引 */
	int   size;		/**< 当前容器中元素总个数 */
};

/**
 * 正向遍历容器中元素
 * @param iter {ITER}
 * @param container {void*} 容器地址
 * @examples: samples/iterator/
 */
#define	FOREACH(iter, container)  \
        for ((container)->iter_head(&(iter), (container));  \
             (iter).ptr;  \
             (container)->iter_next(&(iter), (container)))

/**
 * 反向遍历容器中元素
 * @param iter {ITER}
 * @param container {void*} 容器地址
 * @examples: samples/iterator/
 */
#define	FOREACH_REVERSE(iter, container)  \
        for ((container)->iter_tail(&(iter), (container));  \
             (iter).ptr;  \
             (container)->iter_prev(&(iter), (container)))

/**
 * 获得当前迭代指针与某容器关联的成员结构类型对象
 * @param iter {ITER}
 * @param container {void*} 容器地址
 */
#define	ITER_INFO(iter, container)  \
	(container)->iter_info(&(iter), (container))

#define	foreach_reverse	FOREACH_REVERSE
#define	foreach		FOREACH

#endif
