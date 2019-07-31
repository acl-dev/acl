#ifndef	RING_INCLUDE_H
#define	RING_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef struct RING RING;

/**
 * 数据环结构类型定义
 */
struct RING {
	RING   *succ;           /**< successor */
	RING   *pred;           /**< predecessor */

	RING   *parent;         /**< the header of all the rings */
	int     len;            /**< the count in the ring */
};

typedef struct RING_ITER {
	RING *ptr;
} RING_ITER;

/**
 * 初始化数据环
 * @param ring {RING*} 数据环
 */
#ifdef USE_FAST_RING
#define ring_init(__ring) do { \
	RING *_ring   = __ring; \
	_ring->pred   = _ring->succ = _ring; \
	_ring->parent = _ring; \
	_ring->len    = 0; \
} while (0)
#else
void ring_init(RING *ring);
#endif

/**
 * 获得当前数据环内元素个数
 * @param ring {RING*} 数据环
 * @return {int} 数据环内元素个数
 */
#ifdef USE_FAST_RING
#define ring_size(r) (((RING*)(r))->len)
#else
int  ring_size(const RING *ring);
#endif

/**
 * 将一个新元素添加进环的尾部
 * @param ring {RING*} 数据环
 * @param entry {RING*} 新的元素
 */
#ifdef USE_FAST_RING
#define ring_append(r, e) do { \
	((RING*)(e))->succ       = ((RING*)(r))->succ; \
	((RING*)(e))->pred       = (RING*)(r); \
	((RING*)(e))->parent     = ((RING*)(r))->parent; \
	((RING*)(r))->succ->pred = (RING*)(e); \
	((RING*)(r))->succ       = (RING*)(e); \
	((RING*)(r))->parent->len++; \
} while (0)

#else
void ring_append(RING *ring, RING *entry);
#endif

/**
 * 将一个新元素添加进环的头部
 * @param ring {RING*} 数据环
 * @param entry {RING*} 新的元素
 */
#ifdef USE_FAST_RING
#define ring_prepend(r, e) do { \
	((RING*)(e))->pred       = ((RING*)(r))->pred; \
	((RING*)(e))->succ       = (RING*)(r); \
	((RING*)(e))->parent     = ((RING*)(r))->parent; \
	((RING*)(r))->pred->succ = (RING*)(e); \
	((RING*)(r))->pred       = (RING*)(e); \
	((RING*)(r))->parent->len++; \
} while (0)
#else
void ring_prepend(RING *ring, RING *entry);
#endif

/**
 * 将一个环元素从数据环中删除
 * @param entry {RING*} 环元素
 */
#ifdef USE_FAST_RING
#define ring_detach(e) do { \
	RING *_succ, *_pred; \
	if (((RING*)(e))->parent != (RING*)(e)) { \
		_succ = ((RING*)(e))->succ; \
		_pred = ((RING*)(e))->pred; \
		if (_succ && _pred) { \
			_pred->succ = _succ; \
			_succ->pred = _pred; \
			((RING*)(e))->parent->len--; \
			((RING*)(e))->succ   = (RING*)(e); \
			((RING*)(e))->pred   = (RING*)(e); \
			((RING*)(e))->parent = (RING*)(e); \
			((RING*)(e))->len    = 0; \
		} \
	} \
} while (0)
#else
void ring_detach(RING *entry);
#endif

/**
 * 从环中弹出头部环元素
 * @param ring {RING*} 数据环
 * @return {RING*} 头部环元素，如果返回空则表示该数据环为空
 */
#ifdef USE_FAST_RING
static inline RING *ring_pop_head(RING *ring)
{
	RING *succ;

	succ = ring->succ;
	if (succ == ring) {
		return NULL;
	}

	ring_detach(succ);
	return succ;
}
#else
RING *ring_pop_head(RING *ring);
#endif

/**
 * 从环中弹出尾部环元素
 * @param ring {RING*} 数据环
 * @return {RING*} 尾部环元素，如果返回空则表示该数据环为空
 */
#ifdef USE_FAST_RING
static inline RING *ring_pop_tail(RING *ring)
{
	RING *pred;

	pred = ring->pred;
	if (pred == ring) {
		return NULL;
	}

	ring_detach(pred);
	return pred;
}
#else
RING *ring_pop_tail(RING *ring);
#endif

/*--------------------  一些方便快捷的宏操作 --------------------------------*/

/**
 * 返回当前环元素的下一个环元素
 */
#define RING_SUCC(c) ((c)->succ)
#define	ring_succ	RING_SUCC

/**
 * 返回当前环元素的前一个环元素
 */
#define RING_PRED(c) ((c)->pred)
#define	ring_pred	RING_PRED

/**
 * 将环元素指针转换成应用的自定义类型的指针地址
 * @param ring_ptr {RING*} 环元素指针
 * @param app_type 应用自定义类型
 * @param ring_member {RING*} 环元素在应用自定义结构中的成员名称
 * @return {app_type*} 应用自定义结构类型的对象地址
 */
#define RING_TO_APPL(ring_ptr, app_type, ring_member) \
    ((app_type *) (((char *) (ring_ptr)) - offsetof(app_type,ring_member)))

#define	ring_to_appl	RING_TO_APPL

/**
 * 从头部至尾部遍历数据环中的所有环元素
 * @param iter {RING_ITER}
 * @param head_ptr {RING*} 数据环的头指针
 * @example:
 	typedef struct {
		char  name[32];
		RING entry;
	} DUMMY;

	void test()
	{
		RING head;
		DUMMY *dummy;
		RING_ITER iter;
		int   i;

		ring_init(&head);

		for (i = 0; i < 10; i++) {
			dummy = (DUMMY*) mycalloc(1, sizeof(DUMMY));
			snprintf(dummy->name, sizeof(dummy->name), "dummy:%d", i);
			ring_append(&head, &dummy->entry);
		}

		ring_foreach(iter, &head) {
			dummy = ring_to_appl(iter.ptr, DUMMY, entry);
			printf("name: %s\n", dummy->name);
		}

		while (1) {
			iter.ptr = ring_pop_head(&head);
			if (iter.ptr == NULL)
				break;
			dummy = ring_to_appl(iter.ptr, DUMMY, entry);
			myfree(dummy);
		}
	}
 */
#define	RING_FOREACH(iter, head_ptr) \
        for ((iter).ptr = ring_succ((head_ptr)); (iter).ptr != (head_ptr);  \
             (iter).ptr = ring_succ((iter).ptr))

#define	ring_foreach		RING_FOREACH

/**
 * 从尾部至头部遍历数据环中的所有环元素
 * @param iter {RING_ITER}
 * @param head_ptr {RING*} 数据环的头指针
 */
#define	RING_FOREACH_REVERSE(iter, head_ptr) \
        for ((iter).ptr = ring_pred((head_ptr)); (iter).ptr != (head_ptr);  \
             (iter).ptr = ring_pred((iter).ptr))

#define	ring_foreach_reverse	RING_FOREACH_REVERSE

/**
 * 返回数据环中第一个环元素指针
 * @param head {RING*} 环头指针
 * @return {RING*} NULL: 环为空
 */
#define RING_FIRST(head) \
	(ring_succ(head) != (head) ? ring_succ(head) : 0)

#define	ring_first		RING_FIRST

/**
 * 返回数据环中头第一个环元素指针同时将其转换应用自定义结构类型的对象地址
 * @param head {RING*} 环头指针
 * @param app_type 应用自定义结构类型
 * @param ring_member {RING*} 环元素在应用自定义结构中的成员名称
 * @return {app_type*} 应用自定义结构类型的对象地址
 */
#define RING_FIRST_APPL(head, app_type, ring_member) \
	(ring_succ(head) != (head) ? \
	 RING_TO_APPL(ring_succ(head), app_type, ring_member) : 0)

#define	ring_first_appl	RING_FIRST_APPL

/**
 * 返回数据环中最后一个环元素指针
 * @param head {RING*} 环头指针
 * @return {RING*} NULL: 环为空
 */
#define RING_LAST(head) \
       (ring_pred(head) != (head) ? ring_pred(head) : 0)

#define	ring_last		RING_LAST

/**
 * 返回数据环中最后一个环元素指针同时将其转换应用自定义结构类型的对象地址
 * @param head {RING*} 环头指针
 * @param app_type 应用自定义结构类型
 * @param ring_member {RING*} 环元素在应用自定义结构中的成员名称
 * @return {app_type*} 应用自定义结构类型的对象地址
 */
#define RING_LAST_APPL(head, app_type, ring_member) \
       (ring_pred(head) != (head) ? \
	RING_TO_APPL(ring_pred(head), app_type, ring_member) : 0)

#define	ring_last_appl	RING_LAST_APPL

/**
 * 将一个新元素添加进环的尾部
 * @param ring {RING*} 数据环
 * @param entry {RING*} 新的元素
 */
#define	RING_APPEND(ring_in, entry_in) do {  \
	RING *ring_ptr = (ring_in), *entry_ptr = (entry_in);  \
        entry_ptr->succ      = ring_ptr->succ;  \
        entry_ptr->pred      = ring_ptr;  \
        entry_ptr->parent    = ring_ptr->parent;  \
        ring_ptr->succ->pred = entry_ptr;  \
        ring_ptr->succ       = entry_ptr;  \
        ring_ptr->parent->len++;  \
} while (0)

/**
 * 将一个新元素添加进环的头部
 * @param ring {RING*} 数据环
 * @param entry {RING*} 新的元素
 */
#define	RING_PREPEND(ring_in, entry_in) do {  \
	RING *ring_ptr = (ring_in), *entry_ptr = (entry_in);  \
	entry_ptr->pred      = ring_ptr->pred;  \
	entry_ptr->succ      = ring_ptr;  \
	entry_ptr->parent    = ring_ptr->parent;  \
	ring_ptr->pred->succ = entry_ptr;  \
	ring_ptr->pred       = entry_ptr;  \
	ring_ptr->parent->len++;  \
} while (0)

/**
 * 将一个环元素从数据环中删除
 * @param entry {RING*} 环元素
 */
#define	RING_DETACH(entry_in) do {  \
	RING   *succ, *pred, *entry_ptr = (entry_in);  \
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

