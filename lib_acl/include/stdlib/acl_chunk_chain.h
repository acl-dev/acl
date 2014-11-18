#ifndef ACL_CHUNK_CHAIN_INCLUDE_H
#define ACL_CHUNK_CHAIN_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "acl_define.h"

/**
 * 数据链类型定义
 */
typedef struct ACL_CHAIN ACL_CHAIN;

/**
 * 创建一个数据链对象
 * @param init_size {size_t} 连续数据动态内存的初始尺寸大小
 * @param off_begin {acl_int64} 连续数据块的起始位置
 * @return {ACL_CHAIN*} 数据链对象
 */
ACL_API ACL_CHAIN *acl_chain_new(size_t init_size, acl_int64 off_begin);

/**
 * 释放数据链对象
 * @param chain {ACL_CHAIN*} 数据链对象
 */
ACL_API void acl_chain_free(ACL_CHAIN *chain);

/**
 * 设置连续数据块的下一个偏移位置
 * @param chain {ACL_CHAIN*} 数据链对象
 * @param from_next {acl_int64} 连续数据块的下一个偏移位置
 */
ACL_API void acl_chain_set_from_next(ACL_CHAIN *chain, acl_int64 from_next);

/**
 * 重置数据链对象，并将起始位置重置为给定值
 * @param chain {ACL_CHAIN*} 数据链对象
 * @param off_begin {acl_int64} 给定连接数据块的起始位置
 */
ACL_API void acl_chain_reset(ACL_CHAIN *chain, acl_int64 off_begin);

/**
 * 获得当前数据链对象中连续数据块中的下一个位置
 * @param chain {ACL_CHAIN*} 数据链对象
 * @return {acl_int64} 连接数据块的下一个位置
 */
ACL_API acl_int64 acl_chain_from_next(ACL_CHAIN *chain);

/**
 * 获得当前数据链对象的起始位置
 * @param chain {ACL_CHAIN*} 数据链对象
 * @return {acl_int64} 数据链的起始位置
 */
ACL_API acl_int64 acl_chain_off_begin(ACL_CHAIN *chain);

/**
 * 获得当前数据链中连续数据块的起始存储指针地址
 * @param chain {ACL_CHAIN*} 数据链对象
 * @return {const char*} 连续数据块的起始存储指针地址
 */
ACL_API const char *acl_chain_data(ACL_CHAIN *chain);

/**
 * 获得当前数据链中连续数据块的数据长度
 * @param chain {ACL_CHAIN*} 数据链对象
 * @return {int} 连续数据块的数据长度
 */
ACL_API int acl_chain_data_len(ACL_CHAIN *chain);

/**
 * 当前数据链中非连续数据块的个数
 * @param chain {ACL_CHAIN*} 数据链对象
 * @return {int} 非连续数据块的个数
 */
ACL_API int acl_chain_size(ACL_CHAIN *chain);

/**
 * 获得当前数据链中非连续数据块的所有数据总长度
 * @param chain {ACL_CHAIN*} 数据链对象
 * @return {int} 非连续数据块的总长度
 */
ACL_API int acl_chain_chunk_data_len(ACL_CHAIN *chain);

/**
 * 向数据链中添加一个数据块，内部自动去掉重叠数据
 * @param chain {ACL_CHAIN*} 数据链对象
 * @param data {const void*} 数据块指针
 * @param from {acl_int64} 该新数据块的起始位置
 * @param dlen {int} 数据块的长度
 */
ACL_API void acl_chain_add(ACL_CHAIN *chain, const void *data,
	acl_int64 from, int dlen);

/**
 * 打印输出当前数据链的连续数据块及非连续数据块的起始位置信息
 * @param chain {ACL_CHAIN*} 数据链对象
 */
ACL_API void acl_chain_list(ACL_CHAIN *chain);

#ifdef __cplusplus
}
#endif

#endif
