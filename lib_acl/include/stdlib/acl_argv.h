#ifndef ACL_ARGV_INCLUDE_H
#define ACL_ARGV_INCLUDE_H

# ifdef	__cplusplus
extern "C" {
# endif
#include "acl_define.h"
//#include <stdarg.h>
#include "acl_dbuf_pool.h"
#include "acl_iterator.h"

#define ACL_ARGV_END	((char *) 0)

/**
 * External interface.
 */
typedef struct ACL_ARGV ACL_ARGV;

struct ACL_ARGV {
	int     len;			/**< number of array elements */
	int     argc;			/**< array elements in use */
	char  **argv;			/**< string array */

	/* 添加及弹出 */

	/* 向数组尾部添加字符串 (内部动态拷贝该字符串) */
	void  (*push_back)(ACL_ARGV*, const char*);
	/* 向数组头部添加动态对象 (内部动态拷贝该字符串)*/
	void  (*push_front)(ACL_ARGV*, const char*);
	/* 弹出数组尾部字符串 (用完后需调用 acl_myfree 释放) */
	char *(*pop_back)(ACL_ARGV*);
	/* 弹出数组头部字符串 (用完后需调用 acl_myfree 释放) */
	char *(*pop_front)(ACL_ARGV*);

	/* for acl_iterator */

	/* 取迭代器头函数 */
	void *(*iter_head)(ACL_ITER*, const ACL_ARGV*);
	/* 取迭代器下一个函数 */
	void *(*iter_next)(ACL_ITER*, const ACL_ARGV*);
	/* 取迭代器尾函数 */
	void *(*iter_tail)(ACL_ITER*, const ACL_ARGV*);
	/* 取迭代器上一个函数 */
	void *(*iter_prev)(ACL_ITER*, const ACL_ARGV*);

	/* private */
	ACL_DBUF_POOL *dbuf;
};

/* in acl_argv.c */
/**
 * 分配一个字符串动态数组
 * @param size {int} 动态数组的初始大小
 * @return {ACL_ARGV*}
 */
ACL_API ACL_ARGV *acl_argv_alloc(int size);
ACL_API ACL_ARGV *acl_argv_alloc2(int size, ACL_DBUF_POOL *dbuf);

/**
 * 设置 ACL_ARGV 对象的遍历函数
 * @param argvp {ACL_ARGV*}
 */
ACL_API void acl_argv_iter_init(ACL_ARGV *argvp);

/**
 * 向字符串动态数组中添加一至多个字符串，最后一个NULL字符串表示结束
 * @param argvp {ACL_ARGV*} 字符串动态数组指针
 * @param ... 字符串列表，最后一个为NULL, 格式如：{s1}, {s2}, ..., NULL
 */
ACL_API void acl_argv_add(ACL_ARGV *argvp,...);

/**
 * 在指定位置设置指定的字符串，同时释放旧的字符串
 * @param argvp {ACL_ARGV *} 字符串动态数组
 * @param idx {int} 指定下标位置，不应越界
 * @param value {const char *} 非 NULL 字符串
 * @return {int} 返回 -1 表示下标越界或 value 为 NULL，0 表示成功
 */
ACL_API int acl_argv_set(const ACL_ARGV *argvp, int idx, const char *value);

/**
 * 向字符串动态数组中添加字符串列表
 * @param argvp {ACL_ARGV*} 字符串动态数组指针
 * @param ap {va_list} 由多个字符串组成的变参列表
 */
ACL_API void acl_argv_addv(ACL_ARGV *argvp, va_list ap);

/**
 * 向字符串动态数组中添加字段长度有限制的字符串列表
 * @param argvp {ACL_ARGV*} 字符串动态数组指针
 * @param ... 一组有长度限制的字符串列表，
 *  如: {s1}, {len1}, {s2}, {len2}, ... NULL
 */
ACL_API void acl_argv_addn(ACL_ARGV *argvp,...);

/**
 * 向字符串动态数组中添加字段长度有限制的字符串列表
 * @param argvp {ACL_ARGV*} 字符串动态数组指针
 * @param ap {va_list} 一组有长度限制的字符串组成的变参列表
 */
ACL_API void acl_argv_addnv(ACL_ARGV *argvp, va_list ap);

/**
 * 设置字符串动态数组的结束位置
 * @param argvp {ACL_ARGV*} 字符串动态数组指针
 */
ACL_API void acl_argv_terminate(const ACL_ARGV *argvp);

/**
 * 释放字符串动态数组
 * @param argvp {ACL_ARGV*} 字符串动态数组指针
 */
ACL_API void acl_argv_free(ACL_ARGV *argvp);

/**
 * 根据数组下标位置返回相对应的字符串指针
 * @param argvp {ACL_ARGV*} 字符串动态数组指针
 * @param idx {int} 下标位置
 * @return {char*} NULL: 下标越界；!= NULL: 字符串指针位置
 */
ACL_API char *acl_argv_index(const ACL_ARGV *argvp, int idx);

/**
 * 返回当前字符串动态数组中已经存放的字符串个数
 * @param argvp {ACL_ARGV*} 字符串动态数组指针
 * @return {int}
 */
ACL_API int acl_argv_size(const ACL_ARGV *argvp);

/* in acl_argv_split.c */
/**
 * 根据源字符串及分隔字符串生成一个字符串动态数组
 * @param str {const char*} 源字符串
 * @param delim {const char*} 分隔字符串
 * @return {ACL_ARGV*}
 */
ACL_API ACL_ARGV *acl_argv_split(const char *str, const char *delim);

/**
 * 根据源字符串及分隔字符串生成一个字符串动态数组，同时将传入的内存池
 * 对象做为内存分配器
 * @param str {const char*} 源字符串
 * @param delim {const char*} 分隔字符串
 * @param dbuf {ACL_DBUF_POOL*} 内存池对象，可以为空，当为空时则采用
 *  缺省的内存分配方式
 * @return {ACL_ARGV*}
 */
ACL_API ACL_ARGV *acl_argv_split3(const char *str, const char *delim,
	ACL_DBUF_POOL *dbuf);

/**
 * 根据源字符串及分隔字符串生成一个字符串动态数组, 但限定最大分隔次数
 * @param str {const char*} 源字符串
 * @param delim {const char*} 分隔字符串
 * @param n {size_t} 最大分隔次数
 * @return {ACL_ARGV*}
 */
ACL_API ACL_ARGV *acl_argv_splitn(const char *str, const char *delim, size_t n);

/**
 * 根据源字符串及分隔字符串生成一个字符串动态数组, 但限定最大分隔次数，
 * 同时传入内存池对象做为内存分配器
 * @param str {const char*} 源字符串
 * @param delim {const char*} 分隔字符串
 * @param n {size_t} 最大分隔次数
 * @param dbuf {ACL_DBUF_POOL*} 内存池对象，可以为空，当为空时则采用
 *  缺省的内存分配方式
 * @return {ACL_ARGV*}
 */
ACL_API ACL_ARGV *acl_argv_splitn4(const char *str, const char *delim,
	size_t n, ACL_DBUF_POOL *dbuf);

/**
 * 源字符串经分隔符分解后，其结果被附加至一个字符串动态数组
 * @param argvp {ACL_ARGV*} 字符串动态数组指针
 * @param str {const char*} 源字符串
 * @param delim {const char*} 分隔字符串
 * @return {ACL_ARGV*}
 */
ACL_API ACL_ARGV *acl_argv_split_append(ACL_ARGV *argvp, const char *str,
	const char *delim);

/**
 * 源字符串经分隔符分解后，其结果被附加至一个字符串动态数组, 但限定最大分隔次数
 * @param argvp {ACL_ARGV*} 字符串动态数组指针
 * @param str {const char*} 源字符串
 * @param delim {const char*} 分隔字符串
 * @param n {size_t} 最大分隔次数
 * @return {ACL_ARGV*}
 */
ACL_API ACL_ARGV *acl_argv_splitn_append(ACL_ARGV *argvp, const char *str,
	const char *delim, size_t n);

/**
 * 根据源字符串及分隔字符串生成一个字符串动态数组，针对由 "" 或 '' 引用的
 * 字符串不做分隔
 * @param str {const char*} 源字符串
 * @param delim {const char*} 分隔字符串
 * @return {ACL_ARGV*}
 */
ACL_API ACL_ARGV *acl_argv_quote_split(const char *str, const char *delim);

/**
 * 根据源字符串及分隔字符串生成一个字符串动态数组，针对由 "" 或 '' 引用的
 * 字符串不做分隔，其中将传入的内存池对象做为内存分配器
 * @param str {const char*} 源字符串
 * @param delim {const char*} 分隔字符串
 * @param dbuf {ACL_DBUF_POOL*} 内存池对象，可以为空，当为空时则采用
 *  缺省的内存分配方式
 * @return {ACL_ARGV*}
 */
ACL_API	ACL_ARGV *acl_argv_quote_split4(const char *str, const char *delim,
	ACL_DBUF_POOL *dbuf);

///////////////////////////////////////////////////////////////////////////////
// 更高效的字符串分割算法。

typedef struct ACL_ARGV_VIEW ACL_ARGV_VIEW;

struct ACL_ARGV_VIEW {
	ACL_ARGV argv;

	/* 取迭代器头函数 */
	void *(*iter_head)(ACL_ITER*, const ACL_ARGV_VIEW*);
	/* 取迭代器下一个函数 */
	void *(*iter_next)(ACL_ITER*, const ACL_ARGV_VIEW*);
	/* 取迭代器尾函数 */
	void *(*iter_tail)(ACL_ITER*, const ACL_ARGV_VIEW*);
	/* 取迭代器上一个函数 */
	void *(*iter_prev)(ACL_ITER*, const ACL_ARGV_VIEW*);
};

/**
 * 使用分割字符串中的每一个字符做为分割符，对源字符串进行分割，返回的对象只读禁止修改.
 * @param str {const char*} 源字符串
 * @param delim {const char*} 用来对源字符串进行分割的字符串，该字符串中出现的每一个
 *  字符都将做为分割符
 * @return {ACL_ARGV_VIEW*} 分割后的对象（永远非NULL），返回对象只读不能修改，
 *  可以使用 ACL_ITER 进行遍历.
 */
ACL_API ACL_ARGV_VIEW *acl_argv_view_split(const char *str, const char *delim);

/**
 *  释放分割对象
 * @param view {ACL_ARGV_VIEW*} 由 acl_argv_view_split 创建的分割对象
 */
ACL_API void acl_argv_view_free(ACL_ARGV_VIEW *view);

# ifdef	__cplusplus
}
# endif

#endif
