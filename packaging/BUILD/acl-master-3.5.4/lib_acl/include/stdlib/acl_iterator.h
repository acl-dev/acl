#ifndef	ACL_ITERATOR_INCLUDE_H
#define	ACL_ITERATOR_INCLUDE_H

typedef struct ACL_ITER ACL_ITER;

/**
 * ACL 库中数据结构用的通用迭代器结构定义
 */
struct ACL_ITER {
	void *ptr;		/**< 迭代器指针, 与容器相关 */
	void *data;		/**< 用户数据指针 */
	int   dlen;		/**< 用户数据长度, 实现者可设置此值也可不设置 */
	const char *key;	/**< 若为哈希表的迭代器, 则为哈希键值地址 */
	int   klen;		/**< 若为ACL_BINHASH迭代器, 则为键长度 */
	int   i;		/**< 当前迭代器在容器中的位置索引 */
	int   size;		/**< 当前容器中元素总个数 */
};

/**
 * 正向遍历容器中元素
 * @param iter {ACL_ITER}
 * @param container {void*} 容器地址
 * @examples: samples/iterator/
 */
#define	ACL_FOREACH(iter, container)  \
        for ((container)->iter_head(&(iter), (container));  \
             (iter).ptr;  \
             (container)->iter_next(&(iter), (container)))

/**
 * 反向遍历容器中元素
 * @param iter {ACL_ITER}
 * @param container {void*} 容器地址
 * @examples: samples/iterator/
 */
#define	ACL_FOREACH_REVERSE(iter, container)  \
        for ((container)->iter_tail(&(iter), (container));  \
             (iter).ptr;  \
             (container)->iter_prev(&(iter), (container)))

/**
 * 获得当前迭代指针与某容器关联的成员结构类型对象
 * @param iter {ACL_ITER}
 * @param container {void*} 容器地址
 */
#define	ACL_ITER_INFO(iter, container)  \
	(container)->iter_info(&(iter), (container))

#define	acl_foreach_reverse	ACL_FOREACH_REVERSE
#define	acl_foreach		ACL_FOREACH
#define	acl_iter_info		ACL_ITER_INFO

#endif
