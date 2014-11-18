#ifndef	__PRIVATE_ARRAY_INCLUDE_H_
#define	__PRIVATE_ARRAY_INCLUDE_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include "stdlib/acl_define.h"
#include "stdlib/acl_iterator.h"
#include "stdlib/acl_array.h"

/**
 * 创建一个动态数组
 * @param init_size {int} 动态数组的初始大小
 * @return {ACL_ARRAY*} 动态数组指针
 */
ACL_ARRAY *private_array_create(int init_size);

/**
 * 释放掉动态数组内的成员变量，但并不释放动态数组对象
 * @param a {ACL_ARRAY*} 动态数组指针
 * @param free_fn {void (*)(void*)} 用于释放动态数组内成员变量的释放函数指针
 */
void private_array_clean(ACL_ARRAY *a, void (*free_fn)(void *));

/**
 * 释放掉动态数组内的成员变量，并释放动态数组对象
 * @param a {ACL_ARRAY*} 动态数组指针
 * @param free_fn {void (*)(void*)} 用于释放动态数组内成员变量的释放函数指针
 */
void private_array_destroy(ACL_ARRAY *a, void (*free_fn)(void *));

/**
 * 向动态数组尾部添加动态成员变量
 * @param a {ACL_ARRAY*} 动态数组指针
 * @param obj {void*} 动态成员变量
 * @return {int} 0: 成功；-1: 失败
 */
int private_array_push(ACL_ARRAY *a, void *obj);

/**
 * 从动态数组中弹出最尾一个对象
 * @param a {ACL_ARRAY*} 动态数组指针
 * @return {void*}, NULL 表示数组为空
 */
void* private_array_pop(ACL_ARRAY *a);

/**
 * 从动态数组中的指定位置删除某个对象，删除后数组内元素的先后顺序有可能发生了改变,
 * 因为删除后会自动将数组中最后的元素移至该位置处
 * @param a {ACL_ARRAY*} 动态数组指针
 * @param position {int} 某个位置，不得越界
 * @param free_fn {void (*)(void*)} 用于释放动态数组内成员变量的释放函数指针，如果该
 *  指针为空，则不释放，否则用此函数进行释放动态对象
 * @return {int} 0: 成功；-1: 失败
 */
int private_array_delete(ACL_ARRAY *a, int idx, void (*free_fn)(void*));

int private_array_delete_obj(ACL_ARRAY *a, void *obj, void (*free_fn)(void*));

/**
 * 从动态数组中的某个下标位置取出动态对象
 * @param a {ACL_ARRAY*} 动态数组指针
 * @param idx {int} 下标位置，不能越界，否则返回-1
 * @return {void*} != NULL: 成功；== NULL: 不存在或失败
 */
void *private_array_index(const ACL_ARRAY *a, int idx);

/**
 * 获得当前动态数组中动态对象的个数
 * @param a {ACL_ARRAY*} 动态数组指针
 * @return {int} 动态数组中动态对象的个数
 */
int private_array_size(const ACL_ARRAY *a);

void private_array_grow(ACL_ARRAY *a, int min_capacity);

#define	PRIVATE_ARRAY_PUSH(a, ptr) do  \
{  \
	if ((a)->count >= (a)->capacity)  \
		private_array_grow((a), (a)->count + 16);  \
	(a)->items[(a)->count++] = (ptr);  \
} while (0)

#define	PRIVATE_ARRAY_POP(a, ptr) do  \
{  \
	if ((a)->count > 0) {  \
		(a)->count--;  \
		(ptr) = (a)->items[(a)->count];  \
		(a)->items[(a)->count] = NULL;  \
	} else {  \
		(ptr) = NULL;  \
	}  \
} while (0)


#ifdef  __cplusplus
}
#endif

#endif

