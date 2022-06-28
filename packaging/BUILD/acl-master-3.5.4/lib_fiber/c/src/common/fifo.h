#ifndef FIFO_INCLUDE_H
#define FIFO_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "iterator.h"

typedef struct FIFO_INFO FIFO_INFO;
typedef struct FIFO_ITER FIFO_ITER;
typedef struct FIFO FIFO;

struct FIFO_INFO {
	void *data;     
	FIFO_INFO *prev;    
	FIFO_INFO *next;
};

struct FIFO_ITER {
	FIFO_INFO *ptr;
};

struct FIFO {
	FIFO_INFO *head;
	FIFO_INFO *tail;
	int   cnt;

	/* 添加及弹出 */

	/* 向队列尾部添加动态对象 */
	void  (*push_back)(struct FIFO*, void*);
	/* 向队列头部添加动态对象 */
	void  (*push_front)(struct FIFO*, void*);
	/* 弹出队列尾部动态对象 */
	void *(*pop_back)(struct FIFO*);
	/* 弹出队列头部动态对象 */
	void *(*pop_front)(struct FIFO*);

	/* for iterator */

	/* 取迭代器头函数 */
	void *(*iter_head)(ITER*, struct FIFO*);
	/* 取迭代器下一个函数 */
	void *(*iter_next)(ITER*, struct FIFO*);
	/* 取迭代器尾函数 */
	void *(*iter_tail)(ITER*, struct FIFO*);
	/* 取迭代器上一个函数 */
	void *(*iter_prev)(ITER*, struct FIFO*);
	/* 取迭代器关联的当前容器成员结构对象 */
	FIFO_INFO *(*iter_info)(ITER*, struct FIFO*);
};

/**
 * 初始化一个给定队列，应用可以在栈上分配队列，而后调用该函数进行初始化
 * @param fifo {FIFO *}
 * @example:
 *   void test(void) {
 	FIFO fifo;

	fifo_init(&fifo);
 *   }
 */
void fifo_init(FIFO *fifo);

/**
 * 从内存堆中分配一个队列对象
 * @return {FIFO*}
 */
FIFO *fifo_new(void);

/**
 * 从内存堆中分配一个队列对象并传内存池对象做为分配器
 * @return {FIFO*}
 */
FIFO *fifo_new(void);

/**
 * 从队列中删除与所给值相同的对象
 * @param fifo {FIFO*}
 * @param data {const void*}
 */
int fifo_delete(FIFO *fifo, const void *data);
void fifo_delete_info(FIFO *fifo, FIFO_INFO *info);

/**
 * 释放以堆分配的队列对象
 * @param fifo {FIFO*}
 * @param free_fn {void (*)(void*)}, 如果该函数指针不为空则
 *  用来释放队列中动态分配的对象
 */
void fifo_free(FIFO *fifo, void (*free_fn)(void *));
void fifo_free2(FIFO *fifo, void (*free_fn)(FIFO_INFO *));

/**
 * 向队列中添加一个动态堆对象
 * @param fifo {FIFO*}
 * @param data {void*} 动态对象
 * @return {FIFO_INFO*} 如果 data 非空则返回队列中的新添加对象, 否则返回 NULL
 */
FIFO_INFO *fifo_push_back(FIFO *fifo, void *data);
#define fifo_push	fifo_push_back
void fifo_push_info_back(FIFO *fifo, FIFO_INFO *info);
#define fifo_push_info	fifo_push_info_back
FIFO_INFO *fifo_push_front(FIFO *fifo, void *data);

/**
 * 从队列中以先进先出方式弹出一个动态对象, 同时将该对象从队列中删除
 * @param fifo {FIFO*}
 * @return {void*}, 如果为空，则表示队列为空
 */
void *fifo_pop_front(FIFO *fifo);
#define fifo_pop	fifo_pop_front
FIFO_INFO *fifo_pop_info(FIFO *fifo);

/**
 * 从队列中以后进先出方式弹出一个动态对象， 同时该对象从队列中删除
 * @param fifo {FIFO*}
 * @return {void*}, 如果为空，则表示队列为空
 */
void *fifo_pop_back(FIFO *fifo);

/**
 * 返回队列中头部的动态对象
 * @param fifo {FIFO*}
 * @return {void*}, 如果为空，则表示队列为空
 */
void *fifo_head(FIFO *fifo);
FIFO_INFO *fifo_head_info(FIFO *fifo);

/**
 * 返回队列中尾部的动态对象
 * @param fifo {FIFO*}
 * @return {void*}, 如果为空，则表示队列为空
 */
void *fifo_tail(FIFO *fifo);
FIFO_INFO *fifo_tail_info(FIFO *fifo);

/**
 * 返回队列中动态对象的总个数
 * @param fifo {FIFO*}
 * @return {int}, >= 0
 */
int fifo_size(FIFO *fifo);

#ifdef __cplusplus
}
#endif

#endif
