#ifndef	ACL_RING_INCLUDE_H
#define	ACL_RING_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include <stddef.h>

typedef struct ACL_RING ACL_RING;

/**
 * 数据环结构类型定义
 */
struct ACL_RING {
	ACL_RING *succ;           /**< successor */
	ACL_RING *pred;           /**< predecessor */

	ACL_RING *parent;         /**< the header of all the rings */
	int len;                  /**< the count in the ring */
};

typedef struct ACL_RING_ITER {
	ACL_RING *ptr;
} ACL_RING_ITER;

/**
 * 初始化数据环
 * @param ring {ACL_RING*} 数据环
 */
ACL_API void acl_ring_init(ACL_RING *ring);

/**
 * 获得当前数据环内元素个数
 * @param ring {ACL_RING*} 数据环
 * @return {int} 数据环内元素个数
 */
ACL_API int  acl_ring_size(const ACL_RING *ring);

/**
 * 将一个新元素添加进环的头部
 * @param ring {ACL_RING*} 数据环
 * @param entry {ACL_RING*} 新的元素
 */
ACL_API void acl_ring_prepend(ACL_RING *ring, ACL_RING *entry);

/**
 * 将一个新元素添加进环的尾部
 * @param ring {ACL_RING*} 数据环
 * @param entry {ACL_RING*} 新的元素
 */
ACL_API void acl_ring_append(ACL_RING *ring, ACL_RING *entry);

/**
 * 将一个环元素从数据环中删除
 * @param entry {ACL_RING*} 环元素
 */
ACL_API void acl_ring_detach(ACL_RING *entry);

/**
 * 从环中弹出头部环元素
 * @param ring {ACL_RING*} 数据环
 * @return {ACL_RING*} 头部环元素，如果返回空则表示该数据环为空
 */
ACL_API ACL_RING *acl_ring_pop_head(ACL_RING *ring);

/**
 * 从环中弹出尾部环元素
 * @param ring {ACL_RING*} 数据环
 * @return {ACL_RING*} 尾部环元素，如果返回空则表示该数据环为空
 */
ACL_API ACL_RING *acl_ring_pop_tail(ACL_RING *ring);

/*--------------------  一些方便快捷的宏操作 --------------------------------*/

/**
 * 返回当前环元素的下一个环元素
 */
#define ACL_RING_SUCC(c) ((c)->succ)
#define	acl_ring_succ	ACL_RING_SUCC

/**
 * 返回当前环元素的前一个环元素
 */
#define ACL_RING_PRED(c) ((c)->pred)
#define	acl_ring_pred	ACL_RING_PRED

/**
 * 将环元素指针转换成应用的自定义类型的指针地址
 * @param ring_ptr {ACL_RING*} 环元素指针
 * @param app_type 应用自定义类型
 * @param ring_member {ACL_RING*} 环元素在应用自定义结构中的成员名称
 * @return {app_type*} 应用自定义结构类型的对象地址
 */
#define ACL_RING_TO_APPL(ring_ptr, app_type, ring_member) \
    ((app_type *) (((char *) (ring_ptr)) - offsetof(app_type,ring_member)))

#define	acl_ring_to_appl	ACL_RING_TO_APPL

/**
 * 从头部至尾部遍历数据环中的所有环元素
 * @param iter {ACL_RING_ITER}
 * @param head_ptr {ACL_RING*} 数据环的头指针
 * @example:
 	typedef struct {
		char  name[32];
		ACL_RING entry;
	} DUMMY;

	void test()
	{
		ACL_RING head;
		DUMMY *dummy;
		ACL_RING_ITER iter;
		int   i;

		acl_ring_init(&head);

		for (i = 0; i < 10; i++) {
			dummy = (DUMMY*) acl_mycalloc(1, sizeof(DUMMY));
			snprintf(dummy->name, sizeof(dummy->name), "dummy:%d", i);
			acl_ring_append(&head, &dummy->entry);
		}

		acl_ring_foreach(iter, &head) {
			dummy = acl_ring_to_appl(iter.ptr, DUMMY, entry);
			printf("name: %s\n", dummy->name);
		}

		while (1) {
			iter.ptr = acl_ring_pop_head(&head);
			if (iter.ptr == NULL)
				break;
			dummy = acl_ring_to_appl(iter.ptr, DUMMY, entry);
			acl_myfree(dummy);
		}
	}
 */
#define	ACL_RING_FOREACH(iter, head_ptr) \
        for ((iter).ptr = acl_ring_succ((head_ptr)); (iter).ptr != (head_ptr);  \
             (iter).ptr = acl_ring_succ((iter).ptr))

#define	acl_ring_foreach		ACL_RING_FOREACH

/**
 * 从尾部至头部遍历数据环中的所有环元素
 * @param iter {ACL_RING_ITER}
 * @param head_ptr {ACL_RING*} 数据环的头指针
 */
#define	ACL_RING_FOREACH_REVERSE(iter, head_ptr) \
        for ((iter).ptr = acl_ring_pred((head_ptr)); (iter).ptr != (head_ptr);  \
             (iter).ptr = acl_ring_pred((iter).ptr))

#define	acl_ring_foreach_reverse	ACL_RING_FOREACH_REVERSE

/**
 * 返回数据环中第一个环元素指针
 * @param head {ACL_RING*} 环头指针
 * @return {ACL_RING*} NULL: 环为空
 */
#define ACL_RING_FIRST(head) \
	(acl_ring_succ(head) != (head) ? acl_ring_succ(head) : 0)

#define	acl_ring_first		ACL_RING_FIRST

/**
 * 返回数据环中头第一个环元素指针同时将其转换应用自定义结构类型的对象地址
 * @param head {ACL_RING*} 环头指针
 * @param app_type 应用自定义结构类型
 * @param ring_member {ACL_RING*} 环元素在应用自定义结构中的成员名称
 * @return {app_type*} 应用自定义结构类型的对象地址
 */
#define ACL_RING_FIRST_APPL(head, app_type, ring_member) \
	(acl_ring_succ(head) != (head) ? \
	 ACL_RING_TO_APPL(acl_ring_succ(head), app_type, ring_member) : 0)

#define	acl_ring_first_appl	ACL_RING_FIRST_APPL

/**
 * 返回数据环中最后一个环元素指针
 * @param head {ACL_RING*} 环头指针
 * @return {ACL_RING*} NULL: 环为空
 */
#define ACL_RING_LAST(head) \
       (acl_ring_pred(head) != (head) ? acl_ring_pred(head) : 0)

#define	acl_ring_last		ACL_RING_LAST

/**
 * 返回数据环中最后一个环元素指针同时将其转换应用自定义结构类型的对象地址
 * @param head {ACL_RING*} 环头指针
 * @param app_type 应用自定义结构类型
 * @param ring_member {ACL_RING*} 环元素在应用自定义结构中的成员名称
 * @return {app_type*} 应用自定义结构类型的对象地址
 */
#define ACL_RING_LAST_APPL(head, app_type, ring_member) \
       (acl_ring_pred(head) != (head) ? \
	ACL_RING_TO_APPL(acl_ring_pred(head), app_type, ring_member) : 0)

#define	acl_ring_last_appl	ACL_RING_LAST_APPL

/**
 * 将一个新元素添加进环的尾部
 * @param ring {ACL_RING*} 数据环
 * @param entry {ACL_RING*} 新的元素
 */
#define	ACL_RING_APPEND(ring_in, entry_in) do {  \
	ACL_RING *ring_ptr = (ring_in), *entry_ptr = (entry_in);  \
        entry_ptr->succ      = ring_ptr->succ;  \
        entry_ptr->pred      = ring_ptr;  \
        entry_ptr->parent    = ring_ptr->parent;  \
        ring_ptr->succ->pred = entry_ptr;  \
        ring_ptr->succ       = entry_ptr;  \
        ring_ptr->parent->len++;  \
} while (0)

/**
 * 将一个新元素添加进环的头部
 * @param ring {ACL_RING*} 数据环
 * @param entry {ACL_RING*} 新的元素
 */
#define	ACL_RING_PREPEND(ring_in, entry_in) do {  \
	ACL_RING *ring_ptr = (ring_in), *entry_ptr = (entry_in);  \
	entry_ptr->pred      = ring_ptr->pred;  \
	entry_ptr->succ      = ring_ptr;  \
	entry_ptr->parent    = ring_ptr->parent;  \
	ring_ptr->pred->succ = entry_ptr;  \
	ring_ptr->pred       = entry_ptr;  \
	ring_ptr->parent->len++;  \
} while (0)

/**
 * 将一个环元素从数据环中删除
 * @param entry {ACL_RING*} 环元素
 */
#define	ACL_RING_DETACH(entry_in) do {  \
	ACL_RING   *succ, *pred, *entry_ptr = (entry_in);  \
	succ = entry_ptr->succ;  \
	pred = entry_ptr->pred;  \
	if (succ != NULL && pred != NULL) {  \
		pred->succ = succ;  \
		succ->pred = pred;  \
		entry_ptr->parent->len--;  \
		entry_ptr->succ = entry_ptr->pred = NULL;  \
	}  \
} while (0)

#ifdef  __cplusplus
}
#endif

#endif

