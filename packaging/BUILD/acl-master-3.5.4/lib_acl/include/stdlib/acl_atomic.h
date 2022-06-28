#ifndef ACL_ATOMIC_INCLUDE_H
#define ACL_ATOMIC_INCLUDE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "acl_define.h"

typedef struct ACL_ATOMIC ACL_ATOMIC;

/**
 * 创建原子对象
 * @return {ACL_ATOMIC*} 返回新创建的对象
 */
ACL_API ACL_ATOMIC *acl_atomic_new(void);

/**
 * 释放原子对象
 * @param self {ACL_ATOMIC*} 原子对象
 */
ACL_API void acl_atomic_free(ACL_ATOMIC *self);

/**
 * 将指定对象与原子对象绑定，以便于对该对象进行原子操作
 * @param self {ACL_ATOMIC*} 原子对象
 * @param value {void*} 被操作的对象，通过原子对象实现对该对象的原子操作
 */
ACL_API void acl_atomic_set(ACL_ATOMIC *self, void *value);

/**
 * 比较并交换对象，当原子对象绑定的对象与给定比较对象相同时才设置新对象且
 * 返回之前绑定的对象
 * @param self {ACL_ATOMIC*} 原子对象
 * @param cmp {void*} 待比较对象指针
 * @param value {void*} 当原子对象与待比较对象相同时会将该对象与原子对象绑定
 * @return {void*} 返回原子对象之前绑定的对象
 */
ACL_API void *acl_atomic_cas(ACL_ATOMIC *self, void *cmp, void *value);

/**
 * 将原子对象与新对象进行绑定，并返回之前绑定的对象
 * @param self {ACL_ATOMIC*} 原子对象
 * @param value {void*} 将被绑定的新对象
 * @return {void*} 返回之前绑定的对象
 */
ACL_API void *acl_atomic_xchg(ACL_ATOMIC *self, void *value);

/**
 * 当调用 acl_atomic_set 绑定的对象为数值对象时，可以调用此函数设置被绑定对象
 * 的长整数值
 * @param self {ACL_ATOMIC*} 原子对象
 * @param n {long long} 被原子对象所绑定的对象将被赋值为此值
 */
ACL_API void acl_atomic_int64_set(ACL_ATOMIC *self, long long n);

/**
 * 先获得数值对象所存储的整数值，然后再增加指定的值存储于该数值对象中
 * @param self {ACL_ATOMIC*} 原子对象
 * @param n {long long} 增加值 
 * @return {long long} 返回增加之前数据数值对象的值
 */
ACL_API long long acl_atomic_int64_fetch_add(ACL_ATOMIC *self, long long n);

/**
 * 对数据对象存储的值增加指定的值，并返回结果值
 * @param self {ACL_ATOMIC*} 原子对象
 * @param n {long long} 增加值 
 * @return {long long} 返回增加之后的值
 */
ACL_API long long acl_atomic_int64_add_fetch(ACL_ATOMIC *self, long long n);

/**
 * 比较并交换整数值，当原子对象存储的整数值与给定比较整数值相同时才设置新整数
 * 值且返回之前存储的整数值
 * @param self {ACL_ATOMIC*} 原子对象
 * @param cmp {long long} 待比较整数值
 * @param n {long long} 当原子对象与待比较整数值相同时会将原子对象设置为此值
 * @return {long long} 返回原子对象之前存储的整数值
 */
ACL_API long long acl_atomic_int64_cas(ACL_ATOMIC *self, long long cmp, long long n);

/****************************************************************************/

typedef struct ACL_ATOMIC_CLOCK ACL_ATOMIC_CLOCK;

ACL_API ACL_ATOMIC_CLOCK *acl_atomic_clock_alloc(void);
ACL_API void acl_atomic_clock_free(ACL_ATOMIC_CLOCK *clk);
ACL_API long long acl_atomic_clock_count_add(ACL_ATOMIC_CLOCK *clk, int n);
ACL_API long long acl_atomic_clock_users_add(ACL_ATOMIC_CLOCK *clk, int n);
ACL_API void acl_atomic_clock_users_count_inc(ACL_ATOMIC_CLOCK *clk);
ACL_API long long acl_atomic_clock_count(ACL_ATOMIC_CLOCK *clk);
ACL_API long long acl_atomic_clock_atime(ACL_ATOMIC_CLOCK *clk);
ACL_API long long acl_atomic_clock_users(ACL_ATOMIC_CLOCK *clk);

#ifdef __cplusplus
}
#endif

#endif
