#ifndef ACL_DLINK_INCLUDE
#define ACL_DLINK_INCLUDE

#ifdef __cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include "acl_array.h"
#include "acl_iterator.h"

/**
 * 二分块数据链元素类型定义
 */
typedef	struct {
	acl_int64 begin;
	acl_int64 end;
	void *pnode;
} ACL_DITEM;

/**
 * 二分块数据链类型定义
 */
typedef	struct ACL_DLINK {
	ACL_ARRAY *parray;
	void *call_back_data;

	/* for acl_iterator */

	/* 取迭代器头函数 */
	void *(*iter_head)(ACL_ITER*, struct ACL_DLINK*);
	/* 取迭代器下一个函数 */
	void *(*iter_next)(ACL_ITER*, struct ACL_DLINK*);
	/* 取迭代器尾函数 */
	void *(*iter_tail)(ACL_ITER*, struct ACL_DLINK*);
	/* 取迭代器上一个函数 */
	void *(*iter_prev)(ACL_ITER*, struct ACL_DLINK*);
} ACL_DLINK;

/**
 * 创建一个二分块数据链对象
 * @param nsize {int} 初始数组大小
 * @return {ACL_DLINK*} 二分块数据链对象
 */
ACL_API ACL_DLINK *acl_dlink_create(int nsize);

/**
 * 释放一个二分块数据链对象
 * @param plink {ACL_DLINK*} 二分块数据链对象指针
 */
ACL_API void acl_dlink_free(ACL_DLINK *plink);

/**
 * 根据二分块数据元素查找其是否正在于二分块数据链中
 * @param plink {ACL_DLINK*} 二分块数据链对象指针
 * @param pitem {ACL_DITEM*} 数据块元素
 * @return {ACL_DITEM*} 数据块元素
 */
ACL_API ACL_DITEM *acl_dlink_lookup_by_item(const ACL_DLINK *plink,
	ACL_DITEM *pitem);

/**
 * 根据数据元素查找二分块数据链中的数据元素
 * @param plink {const ACL_DLINK*} 二分块数据链对象指针
 * @param pitem {ACL_DITEM*} 数据块元素
 * @param pidx {int*} 存储查询数据元素结果在二分数据链中的下标位置
 * @return {ACL_DITEM*} 数据块元素
 */
ACL_API ACL_DITEM *acl_dlink_lookup2_by_item(const ACL_DLINK *plink,
	ACL_DITEM *pitem, int *pidx);

/**
 * 从二分块数据链中查询某个值所对应的数据块元素地址
 * @param plink {const ACL_DLINK*} 二分块数据链对象指针
 * @param n {acl_int64} 查询值
 * @return {ACL_DITEM*} 数据块元素
 */
ACL_API ACL_DITEM *acl_dlink_lookup(const ACL_DLINK *plink, acl_int64 n);

/**
 * 从二分块数据链中查询某个值所对应的数据块元素地址并记录其下标位置
 * @param plink {const ACL_DLINK*} 二分块数据链对象指针
 * @param n {acl_int64} 查询值
 * @param pidx {int*} 存储查询数据元素结果在二分数据链中的下标位置
 * @return {ACL_DITEM*} 数据块元素
 */
ACL_API ACL_DITEM *acl_dlink_lookup2(const ACL_DLINK *plink,
	acl_int64 n, int *pidx);

/**
 * 从二分块数据链中查询某个范围的数据块元素地址并记录其下标位置
 * @param plink {const ACL_DLINK*} 二分块数据链对象指针
 * @param begin {acl_int64} 查询范围的起始位置值
 * @param end {acl_int64} 查询范围的结束位置值
 * @param pidx {int*} 存储查询数据元素结果在二分数据链中的下标位置
 * @return {ACL_DITEM*} 数据块元素
 */
ACL_API ACL_DITEM *acl_dlink_lookup_range(const ACL_DLINK *plink,
	acl_int64 begin, acl_int64 end, int *pidx);

/**
 * 从二分块数据链中查询第一个大于某个给定值的数据块元素并记录下标位置
 * @param plink {const ACL_DLINK*} 二分块数据链对象指针
 * @param off {acl_int64} 给定比较值
 * @param pidx {int*} 存储查询数据元素结果在二分数据链中的下标位置
 * @return {ACL_DITEM*} 数据块元素
 */
ACL_API ACL_DITEM *acl_dlink_lookup_larger(const ACL_DLINK *plink,
	acl_int64 off, int *pidx);

/**
 * 从二分块数据链中查询第一个小于某个给定值的数据块元素并记录下标位置
 * @param plink {const ACL_DLINK*} 二分块数据链对象指针
 * @param off {acl_int64} 给定比较值
 * @param pidx {int*} 存储查询数据元素结果在二分数据链中的下标位置
 * @return {ACL_DITEM*} 数据块元素
 */
ACL_API ACL_DITEM *acl_dlink_lookup_lower(const ACL_DLINK *plink,
	acl_int64 off, int *pidx);

/**
 * 向二分块数据链中添加起始、结束数据块
 * @param plink {ACL_DLINK*} 二分块数据链对象指针
 * @param begin {acl_int64} 给定起始位置值
 * @param end {acl_int64} 给定结束位置值
 * @return {ACL_DITEM*} 新创建的数据块元素
 */
ACL_API ACL_DITEM *acl_dlink_insert(ACL_DLINK *plink,
	acl_int64 begin, acl_int64 end);

/**
 * 从二分块数据链中删除包含某个给定值的数据块元素
 * @param plink {ACL_DLINK*} 二分块数据链对象指针
 * @param n {acl_int64} 给定位置值
 * @return {int} 0：表示OK，-1: 表示输入参数非法或不存在
 */
ACL_API int acl_dlink_delete(ACL_DLINK *plink, acl_int64 n);

/**
 * 根据数据块元素从二分块数据链中删除该数据块元素
 * @param plink {ACL_DLINK*} 二分块数据链对象指针
 * @param pitem {ACL_DITEM*} 数据块元素
 * @return {int} 0：表示OK，-1: 表示输入参数非法
 */
ACL_API int acl_dlink_delete_by_item(ACL_DLINK *plink, ACL_DITEM *pitem);

/**
 * 功能同 acl_dlink_insert
 * @DEPRECATED 此函数将来也许不再提供
 */
ACL_API ACL_DITEM *acl_dlink_modify(ACL_DLINK *plink,
	acl_int64 begin, acl_int64 end);

/**
 * 从二分数据链中删除某个数值范围的数据块集合, 
 * 删除后有可能会在内部增加新的数据块元素
 * @param plink {ACL_DLINK*} 二分块数据链对象指针
 * @param begin {acl_int64} 需要删除范围的起始位置
 * @param end {acl_int64} 需要删除范围的结束位置
 * @return {int} 0：表示OK，-1: 表示输入参数非法
 */
ACL_API int acl_dlink_delete_range(ACL_DLINK *plink,
	acl_int64 begin, acl_int64 end);

/**
 * 返回某下标位置的数据块元素地址
 * @param plink {const ACL_DLINK*} 二分块数据链对象指针
 * @param idx {int} 下标位置
 * @return {ACL_DITEM*} NULL: 下标越界; != NULL: 数据块元素地址
 */
ACL_API ACL_DITEM *acl_dlink_index(const ACL_DLINK *plink, int idx);

/**
 * 获得当前二分数据链中所有数据块的个数总和
 * @param plink {const ACL_DLINK*} 二分块数据链对象指针
 * @return {int} 数据块的个数
 */
ACL_API int acl_dlink_size(const ACL_DLINK *plink);

/**
 * (调试用)打印二分数据链中所有数据块的起始、结束位置等信息
 * @param plink {const ACL_DLINK*} 二分块数据链对象指针
 * @return {int} 0：表示OK，-1: 表示输入参数非法
 */
ACL_API int acl_dlink_list(const ACL_DLINK *plink);

#ifdef __cplusplus
}
#endif
#endif
