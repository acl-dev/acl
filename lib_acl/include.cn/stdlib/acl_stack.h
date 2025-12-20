#ifndef	ACL_STACK_INCLUDE_H
#define	ACL_STACK_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include "acl_iterator.h"

/* 说明：该函数库内部所用的内存分配未采用内存池方式 */

typedef struct ACL_STACK ACL_STACK;

/**
 * 栈类型定义
 */
struct ACL_STACK {
	int   capacity;
	int   count;
	void **items;

	/* 添加及弹出 */

	/* 向栈尾部添加动态对象 */
	void  (*push_back)(struct ACL_STACK*, void*);
	/* 向栈头部添加动态对象 */
	void  (*push_front)(struct ACL_STACK*, void*);
	/* 弹出栈尾部动态对象 */
	void *(*pop_back)(struct ACL_STACK*);
	/* 弹出栈头部动态对象 */
	void *(*pop_front)(struct ACL_STACK*);

	/* for acl_iterator */

	/* 取迭代器头函数 */
	void *(*iter_head)(ACL_ITER*, struct ACL_STACK*);
	/* 取迭代器下一个函数 */
	void *(*iter_next)(ACL_ITER*, struct ACL_STACK*);
	/* 取迭代器尾函数 */
	void *(*iter_tail)(ACL_ITER*, struct ACL_STACK*);
	/* 取迭代器上一个函数 */
	void *(*iter_prev)(ACL_ITER*, struct ACL_STACK*);
};

/**
 * 增加栈空间大小
 * @param s {ACL_STACK*} 创建的栈容器对象
 * @param count {int} 增加的空间大小
 */
ACL_API void acl_stack_space(ACL_STACK *s, int count);

/**
 * 创建一个栈容器对象
 * @param init_size {int} 栈的初始化空间大小，必须 > 0
 * @return {ACL_STACK*} 新创建的栈容器对象
 */
ACL_API ACL_STACK *acl_stack_create(int init_size);

/**
 * 清空栈里的对象，但不销毁栈容器对象
 * @param s {ACL_STACK*} 创建的栈容器对象
 * @param free_fn {void (*)(void*)} 如果不为空，则会用此函数回调栈里的每一个对象
 */
ACL_API void acl_stack_clean(ACL_STACK *s, void (*free_fn)(void *));

/**
 * 清空栈容器里的对象并销毁栈容器
 * @param s {ACL_STACK*} 创建的栈容器对象
 * @param free_fn {void (*)(void*)} 如果不为空，则会用此函数回调栈里的每一个对象
 */
ACL_API void acl_stack_destroy(ACL_STACK *s, void (*free_fn)(void *));

/**
 * 往栈容器尾部添加新的对象
 * @param s {ACL_STACK*} 创建的栈容器对象
 * @param obj {void*}
 */
ACL_API void acl_stack_append(ACL_STACK *s, void *obj);
#define	acl_stack_push	acl_stack_append

/**
 * 往栈容器的头部添加新的对象
 * @param s {ACL_STACK*} 创建的栈容器对象
 * @param obj {void*}
 * 注：此操作的效率要比 acl_stack_append 低，因为其内容需要移动所有的对象位置
 */
ACL_API void acl_stack_prepend(ACL_STACK *s, void *obj);

/**
 * 从栈容器里删除某个对象
 * @param s {ACL_STACK*} 创建的栈容器对象
 * @param position {int} 栈容器的位置
 * @param free_fn {void (*)(void*)} 如果不为空，则用此函数回调被删除对象
 */
ACL_API void acl_stack_delete(ACL_STACK *s, int position, void (*free_fn)(void *));

/**
 * @param s {ACL_STACK*} 创建的栈容器对象
 * @param obj {void*} 被删除对象的地址
 * @param free_fn {void (*)(void*)} 如果不为空，则用此函数回调被删除对象
 */
ACL_API void acl_stack_delete_obj(ACL_STACK *s, void *obj, void (*free_fn)(void *));

/**
 * 返回栈容器中某个位置的对象地址
 * @param s {ACL_STACK*} 创建的栈容器对象
 * @param position {int} 栈容器中的位置
 * @return {void*} != NULL: ok; == NULL: error或不存在
 */
ACL_API void *acl_stack_index(ACL_STACK *s, int position);

/**
 * 返回栈容器中当前的对象个数
 * @param s {ACL_STACK*} 创建的栈容器对象
 * @return {int} 对象个数
 */
ACL_API int acl_stack_size(const ACL_STACK *s);

/**
 * 返回栈中尾部的对象地址, 同时将该对象从栈中移除
 * @param s {ACL_STACK*} 创建的栈容器对象
 * @return {void*} 对象地址, == NULL 表示当前对象为空
 */
ACL_API void *acl_stack_pop(ACL_STACK *s);
#define acl_stack_pop_tail acl_stack_pop

/**
 * 返回栈中最后添加的对象地址, 但不将该对象从栈中移除
 * @param s {ACL_STACK*} 创建的栈容器对象
 * @return {void*} 对象地址, == NULL 表示当前对象为空
 */
ACL_API void *acl_stack_top(ACL_STACK *s);

#ifdef	__cplusplus
}
#endif

#endif
