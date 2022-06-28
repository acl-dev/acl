#ifndef ACL_FIFO_INCLUDE_H
#define ACL_FIFO_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include "acl_slice.h"
#include "acl_iterator.h"

typedef struct ACL_FIFO_INFO ACL_FIFO_INFO;
typedef struct ACL_FIFO_ITER ACL_FIFO_ITER;
typedef struct ACL_FIFO ACL_FIFO;

struct ACL_FIFO_INFO {
	void *data;     
	ACL_FIFO_INFO *prev;    
	ACL_FIFO_INFO *next;
};

struct ACL_FIFO_ITER {
	ACL_FIFO_INFO *ptr;
};

struct ACL_FIFO {
	ACL_FIFO_INFO *head;
	ACL_FIFO_INFO *tail;
	int   cnt;

	/* 添加及弹出 */

	/* 向队列尾部添加动态对象 */
	void  (*push_back)(struct ACL_FIFO*, void*);
	/* 向队列头部添加动态对象 */
	void  (*push_front)(struct ACL_FIFO*, void*);
	/* 弹出队列尾部动态对象 */
	void *(*pop_back)(struct ACL_FIFO*);
	/* 弹出队列头部动态对象 */
	void *(*pop_front)(struct ACL_FIFO*);

	/* for acl_iterator */

	/* 取迭代器头函数 */
	void *(*iter_head)(ACL_ITER*, struct ACL_FIFO*);
	/* 取迭代器下一个函数 */
	void *(*iter_next)(ACL_ITER*, struct ACL_FIFO*);
	/* 取迭代器尾函数 */
	void *(*iter_tail)(ACL_ITER*, struct ACL_FIFO*);
	/* 取迭代器上一个函数 */
	void *(*iter_prev)(ACL_ITER*, struct ACL_FIFO*);
	/* 取迭代器关联的当前容器成员结构对象 */
	ACL_FIFO_INFO *(*iter_info)(ACL_ITER*, struct ACL_FIFO*);

	/* private */
	ACL_SLICE_POOL *slice;
};

/**
 * 初始化一个给定队列，应用可以在栈上分配队列，而后调用该函数进行初始化
 * @param fifo {ACL_FIFO *}
 * @example:
 *   void test(void) {
 	ACL_FIFO fifo;

	acl_fifo_init(&fifo);
 *   }
 */
ACL_API void acl_fifo_init(ACL_FIFO *fifo);

/**
 * 从内存堆中分配一个队列对象
 * @return {ACL_FIFO*}
 */
ACL_API ACL_FIFO *acl_fifo_new(void);

/**
 * 从内存堆中分配一个队列对象并传内存池对象做为分配器
 * @param slice {ACL_SLICE_POOL*}
 * @return {ACL_FIFO*}
 */
ACL_API ACL_FIFO *acl_fifo_new1(ACL_SLICE_POOL *slice);

/**
 * 从队列中删除与所给值相同的对象
 * @param fifo {ACL_FIFO*}
 * @param data {const void*}
 */
ACL_API int acl_fifo_delete(ACL_FIFO *fifo, const void *data);
ACL_API void acl_fifo_delete_info(ACL_FIFO *fifo, ACL_FIFO_INFO *info);

/**
 * 释放以堆分配的队列对象
 * @param fifo {ACL_FIFO*}
 * @param free_fn {void (*)(void*)}, 如果该函数指针不为空则
 *  用来释放队列中动态分配的对象
 */
ACL_API void acl_fifo_free(ACL_FIFO *fifo, void (*free_fn)(void *));
ACL_API void acl_fifo_free2(ACL_FIFO *fifo, void (*free_fn)(ACL_FIFO_INFO *));

/**
 * 向队列中添加一个动态堆对象
 * @param fifo {ACL_FIFO*}
 * @param data {void*} 动态对象
 * @return {ACL_FIFO_INFO*} 如果 data 非空则返回队列中的新添加对象, 否则返回 NULL
 */
ACL_API ACL_FIFO_INFO *acl_fifo_push_back(ACL_FIFO *fifo, void *data);
#define acl_fifo_push	acl_fifo_push_back
ACL_API void acl_fifo_push_info_back(ACL_FIFO *fifo, ACL_FIFO_INFO *info);
#define acl_fifo_push_info	acl_fifo_push_info_back
ACL_API ACL_FIFO_INFO *acl_fifo_push_front(ACL_FIFO *fifo, void *data);

/**
 * 从队列中以先进先出方式弹出一个动态对象, 同时将该对象从队列中删除
 * @param fifo {ACL_FIFO*}
 * @return {void*}, 如果为空，则表示队列为空
 */
ACL_API void *acl_fifo_pop_front(ACL_FIFO *fifo);
#define acl_fifo_pop	acl_fifo_pop_front
ACL_API ACL_FIFO_INFO *acl_fifo_pop_info(ACL_FIFO *fifo);

/**
 * 从队列中以后进先出方式弹出一个动态对象， 同时该对象从队列中删除
 * @param fifo {ACL_FIFO*}
 * @return {void*}, 如果为空，则表示队列为空
 */
ACL_API void *acl_fifo_pop_back(ACL_FIFO *fifo);

/**
 * 返回队列中头部的动态对象
 * @param fifo {ACL_FIFO*}
 * @return {void*}, 如果为空，则表示队列为空
 */
ACL_API void *acl_fifo_head(ACL_FIFO *fifo);
ACL_API ACL_FIFO_INFO *acl_fifo_head_info(ACL_FIFO *fifo);

/**
 * 返回队列中尾部的动态对象
 * @param fifo {ACL_FIFO*}
 * @return {void*}, 如果为空，则表示队列为空
 */
ACL_API void *acl_fifo_tail(ACL_FIFO *fifo);
ACL_API ACL_FIFO_INFO *acl_fifo_tail_info(ACL_FIFO *fifo);

/**
 * 返回队列中动态对象的总个数
 * @param fifo {ACL_FIFO*}
 * @return {int}, >= 0
 */
ACL_API int acl_fifo_size(ACL_FIFO *fifo);

/*--------------------  一些方便快捷的宏操作 --------------------------------*/

/**
 * 获得当前 iter 所包含的对象地址
 * @param iter {ACL_FIFO_ITER}
 */
#define	ACL_FIFO_ITER_VALUE(iter)	((iter).ptr->data)
#define	acl_fifo_iter_value		ACL_FIFO_ITER_VALUE

/**
 * 遍历 ACL_FIFO
 * @param iter {ACL_FIFO_ITER}
 * @param fifo {ACL_FIFO}
 * @example:
        -- 仅是本容器支持的遍历方式
	void test()
	{
		ACL_FIFO *fifo_ptr = acl_fifo_new();
		ACL_FIFO_ITER iter;
		char *data;
		int   i;

		for (i = 0; i < 10; i++) {
			data = acl_mymalloc(32);
			snprintf(data, 32, "data: %d", i);
			acl_fifo_push(fifo_ptr, data);
		}
		acl_fifo_foreach(iter, fifo_ptr) {
	                printf("%s\n", (char*) acl_fifo_iter_value(iter));
	        }

		acl_fifo_free(fifo_ptr, acl_myfree_fn);
	}

	-- 通用容器遍历方式
	void test2()
	{
		ACL_FIFO *fifo_ptr = acl_fifo_new();
		ACL_ITER iter;
		char *data;
		int   i;

		for (i = 0; i < 10; i++) {
			data = acl_mymalloc(32);
			snprintf(data, 32, "data: %d", i);
			acl_fifo_push(fifo_ptr, data);
		}
		acl_foreach(iter, fifo) {
			printf("%s\n", (char*) iter.data);
		}
		acl_fifo_free(fifo_ptr, acl_myfree_fn);
	}
 */
#define	ACL_FIFO_FOREACH(iter, fifo_ptr) \
	for ((iter).ptr = (fifo_ptr)->head; (iter).ptr; (iter).ptr = (iter).ptr->next)
#define	acl_fifo_foreach	ACL_FIFO_FOREACH

/**
 * 反向遍历 ACL_FIFO
 * @param iter {ACL_FIFO_ITER}
 * @param fifo {ACL_FIFO}
 * @example:
	void test()
	{
		ACL_FIFO fifo;
		ACL_FIFO_ITER iter;
		char *data;
		int   i;

		acl_fifo_init(&fifo);

		for (i = 0; i < 10; i++) {
			data = acl_mymalloc(32);
			snprintf(data, 32, "data: %d", i);
			acl_fifo_push(&fifo, data);
		}
		acl_fifo_foreach_reverse(iter, &fifo) {
	                printf("%s\n", (char*) iter.ptr->data);
	        }

		while (1) {
			data = acl_fifo_pop(&fifo);
			if (data == NULL)
				break;
		}
	}
 */
#define	ACL_FIFO_FOREACH_REVERSE(iter, fifo_ptr) \
	for ((iter).ptr = (fifo_ptr)->tail; (iter).ptr; (iter).ptr = (iter).ptr->prev)
#define	acl_fifo_foreach_reverse	ACL_FIFO_FOREACH_REVERSE

#ifdef __cplusplus
}
#endif

#endif

