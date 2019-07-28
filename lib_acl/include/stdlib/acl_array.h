#ifndef	ACL_ARRAY_INCLUDE_H
#define	ACL_ARRAY_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include "acl_dbuf_pool.h"
#include "acl_iterator.h"

/**
 * 动态数组类型定义
 */
typedef	struct ACL_ARRAY ACL_ARRAY;
struct ACL_ARRAY{
	ACL_DBUF_POOL *dbuf;	/**< 内存池对象 */
	int     capacity;	/**< items 数组空间大小 */
	int     count;		/**< items 中含有元素的个数 */
	void    **items;	/**< 动态数组 */

	/* 添加及弹出 */

	/* 向数组尾部添加动态对象 */
	void  (*push_back)(struct ACL_ARRAY*, void*);
	/* 向数组头部添加动态对象 */
	void  (*push_front)(struct ACL_ARRAY*, void*);
	/* 弹出数组尾部动态对象 */
	void *(*pop_back)(struct ACL_ARRAY*);
	/* 弹出数组头部动态对象 */
	void *(*pop_front)(struct ACL_ARRAY*);

	/* for acl_iterator */

	/* 取迭代器头函数 */
	void *(*iter_head)(ACL_ITER*, struct ACL_ARRAY*);
	/* 取迭代器下一个函数 */
	void *(*iter_next)(ACL_ITER*, struct ACL_ARRAY*);
	/* 取迭代器尾函数 */
	void *(*iter_tail)(ACL_ITER*, struct ACL_ARRAY*);
	/* 取迭代器上一个函数 */
	void *(*iter_prev)(ACL_ITER*, struct ACL_ARRAY*);
};

/**
 * 创建一个动态数组
 * @param init_size {int} 动态数组的初始大小
 * @return {ACL_ARRAY*} 动态数组指针
 */
ACL_API ACL_ARRAY *acl_array_create(int init_size);

/**
 * 创建一个动态数组
 * @param init_size {int} 动态数组的初始大小
 * @param dbuf {ACL_DBUF_POOL*} 非空时, 则内存(含数组对象本身)均在上面分配
 * @return {ACL_ARRAY*} 动态数组指针
 */
ACL_API ACL_ARRAY *acl_array_dbuf_create(int init_size, ACL_DBUF_POOL *dbuf);

/**
 * 释放掉动态数组内的成员变量，但并不释放动态数组对象
 * @param a {ACL_ARRAY*} 动态数组指针
 * @param free_fn {void (*)(void*)} 用于释放动态数组内成员变量的释放函数指针
 */
ACL_API void acl_array_clean(ACL_ARRAY *a, void (*free_fn)(void *));

/**
 * 释放掉动态数组内的成员变量，并释放动态数组对象，当数组对象创建 dbuf 对象
 * 时，则该数组对象的释放将会在释放 dbuf 时被释放
 * @param a {ACL_ARRAY*} 动态数组指针
 * @param free_fn {void (*)(void*)} 用于释放动态数组内成员变量的释放函数指针
 */
ACL_API void acl_array_free(ACL_ARRAY *a, void (*free_fn)(void *));
#define acl_array_destroy acl_array_free

/**
 * 向动态数组尾部添加动态成员变量
 * @param a {ACL_ARRAY*} 动态数组指针
 * @param obj {void*} 动态成员变量
 * @return {int} >=0: 成功, 返回值为该元素在数组中的下标位置；-1: 失败
 */
ACL_API int acl_array_append(ACL_ARRAY *a, void *obj);

/**
 * 向动态数组头部添加动态成员变量
 * @param a {ACL_ARRAY*} 动态数组指针
 * @param obj {void*} 动态成员变量
 * @return {int} >=0: 成功, 返回值为该元素在数组中的下标位置；-1: 失败
 */
ACL_API int acl_array_prepend(ACL_ARRAY *a, void *obj);

/**
 * 向动态数组中指定位置前添加动态成员变量(该结点及以后所有结点都后移一个位置)
 * @param a {ACL_ARRAY*} 动态数组指针
 * @param position {int} 某个位置，不得越界
 * @param obj {void*} 动态成员变量
 * @return {int} 0: 成功；-1: 失败
 */
ACL_API int acl_array_pred_insert(ACL_ARRAY *a, int position, void *obj);

/**
 * 向动态数组中指定位置后添加动态成员变量(该结点以后所有结点都后移一个位置)
 * @param a {ACL_ARRAY*} 动态数组指针
 * @param position {int} 某个位置，不得越界
 * @param obj {void*} 动态成员变量
 * @return {int} 0: 成功；-1: 失败
 */
ACL_API int acl_array_succ_insert(ACL_ARRAY *a, int position, void *obj);
#define acl_array_insert acl_array_succ_insert

/**
 * 从动态数组中的指定位置删除某个动态对象, 删除后数组内元素的先后顺序保持不变,
 * 如果被删除位置在中间某个位置，为了保证元素的顺序性，内部将被删除元素后的所有元素
 * 都前移一个位置
 * @param a {ACL_ARRAY*} 动态数组指针
 * @param position {int} 某个位置，不得越界
 * @param free_fn {void (*)(void*)} 用于释放动态数组内成员变量的释放函数指针，如果该
 *  指针为空，则不释放，否则用此函数进行释放动态对象
 * @return {int} 0: 成功；-1: 失败
 */
ACL_API int acl_array_delete_idx(ACL_ARRAY *a, int position, void (*free_fn)(void *));

/**
 * 从动态数组中的指定位置删除某个对象，删除后数组内元素的先后顺序有可能发生了改变,
 * 因为删除后会自动将数组中最后的元素移至该位置处
 * @param a {ACL_ARRAY*} 动态数组指针
 * @param position {int} 某个位置，不得越界
 * @param free_fn {void (*)(void*)} 用于释放动态数组内成员变量的释放函数指针，如果该
 *  指针为空，则不释放，否则用此函数进行释放动态对象
 * @return {int} 0: 成功；-1: 失败
 */
ACL_API int acl_array_delete(ACL_ARRAY *a, int position, void (*free_fn)(void*));

/**
 * 从动态数组中删除指定指针地址的动态对象, 删除后数组内元素的先后顺序保持不变
 * 如果被删除位置在中间某个位置，为了保证元素的顺序性内部，将被删除元素后的所有元素
 * 都前移一个位置
 * @param a {ACL_ARRAY*} 动态数组指针
 * @param obj {void*} 动态对象指针地址
 * @param free_fn {void (*)(void*)} 用于释放动态数组内成员变量的释放函数指针，如果该
 *  指针为空，则不释放，否则用此函数进行释放动态对象
 * @return {int} 0: 成功；-1: 失败
 */
ACL_API int acl_array_delete_obj(ACL_ARRAY *a, void *obj, void (*free_fn)(void *));

/**
 * 从动态数组中删除某个下标范围的动态对象
 * @param a {ACL_ARRAY*} 动态数组指针
 * @param ibegin {int} 开始下标位置
 * @param iend {int} 结束下标位置
 * @param free_fn {void (*)(void*)} 用于释放动态数组内成员变量的释放函数指针，如果该
 *  指针为空，则不释放，否则用此函数进行释放动态对象
 * @return {int} 0: 成功；-1: 失败
 */
ACL_API int acl_array_delete_range(ACL_ARRAY *a, int ibegin, int iend, void (*free_fn)(void*));

/**
 * 移动动态数组中的对象
 * @param a {ACL_ARRAY*} 动态数组指针
 * @param ito {int} 移动至目标下标位置
 * @param ifrom {int} 从此下标位置开始移动
 * @param free_fn {void (*)(void*)} 用于释放动态数组内成员变量的释放函数指针，如果该
 *  指针为空，则不释放，否则用此函数进行释放动态对象被释放的动态对象区间为
 *  [idx_obj_begin, idx_src_begin), 为一半开半闭区间
 * @return {int} 0: 成功；-1: 失败
 */
ACL_API int acl_array_mv_idx(ACL_ARRAY *a, int ito, int ifrom, void (*free_fn)(void *) );

/**
 * 预先保证动态数组的空间长度
 * @param a {ACL_ARRAY*} 动态数组指针
 * @param app_count {int} 需要动态数组至少有 app_count 个空闲位置
 */
ACL_API void acl_array_pre_append(ACL_ARRAY *a, int app_count);

/**
 * 从动态数组中的某个下标位置取出动态对象
 * @param a {ACL_ARRAY*} 动态数组指针
 * @param idx {int} 下标位置，不能越界，否则返回-1
 * @return {void*} != NULL: 成功；== NULL: 不存在或失败
 */
ACL_API void *acl_array_index(const ACL_ARRAY *a, int idx);

/**
 * 获得当前动态数组中动态对象的个数
 * @param a {ACL_ARRAY*} 动态数组指针
 * @return {int} 动态数组中动态对象的个数
 */
ACL_API int acl_array_size(const ACL_ARRAY *a);

#ifdef  __cplusplus
}
#endif

#endif

