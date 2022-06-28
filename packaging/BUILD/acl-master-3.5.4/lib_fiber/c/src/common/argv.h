#ifndef __ARGV_INCLUDE_H__
#define __ARGV_INCLUDE_H__

# ifdef	__cplusplus
extern "C" {
# endif
#include <stdarg.h>
#include "iterator.h"

/**
 * External interface.
 */
typedef struct ARGV {
	int     len;			/**< number of array elements */
	int     argc;			/**< array elements in use */
	char  **argv;			/**< string array */

	/* 添加及弹出 */

	/* 向数组尾部添加字符串 (内部动态拷贝该字符串) */
	void  (*push_back)(struct ARGV*, const char*);
	/* 向数组头部添加动态对象 (内部动态拷贝该字符串)*/
	void  (*push_front)(struct ARGV*, const char*);
	/* 弹出数组尾部字符串 (用完后需调用 myfree 释放) */
	char *(*pop_back)(struct ARGV*);
	/* 弹出数组头部字符串 (用完后需调用 myfree 释放) */
	char *(*pop_front)(struct ARGV*);

	/* for iterator */

	/* 取迭代器头函数 */
	void *(*iter_head)(ITER*, struct ARGV*);
	/* 取迭代器下一个函数 */
	void *(*iter_next)(ITER*, struct ARGV*);
	/* 取迭代器尾函数 */
	void *(*iter_tail)(ITER*, struct ARGV*);
	/* 取迭代器上一个函数 */
	void *(*iter_prev)(ITER*, struct ARGV*);
} ARGV;

/* in argv.c */
/**
 * 分配一个字符串动态数组
 * @param size {int} 动态数组的初始大小
 * @return {ARGV*}
 */
ARGV *argv_alloc(int size);

/**
 * 向字符串动态数组中添加一至多个字符串，最后一个NULL字符串表示结束
 * @param argvp {ARGV*} 字符串动态数组指针
 * @param ... 字符串列表，最后一个为NULL, 格式如：{s1}, {s2}, ..., NULL
 */
void argv_add(ARGV *argvp,...);

/**
 * 向字符串动态数组中添加字符串列表
 * @param argvp {ARGV*} 字符串动态数组指针
 * @param ap {va_list} 由多个字符串组成的变参列表
 */
void argv_addv(ARGV *argvp, va_list ap);

/**
 * 释放字符串动态数组
 * @param argvp {ARGV*} 字符串动态数组指针
 */
ARGV *argv_free(ARGV *argvp);

/**
 * 设置字符串动态数组的结束位置
 * @param argvp {ACL_ARGV*} 字符串动态数组指针
 */
void argv_terminate(ARGV *argvp);

/**
 * 返回当前字符串动态数组中已经存放的字符串个数
 * @param argvp {ARGV*} 字符串动态数组指针
 * @return {int}
 */
int argv_size(ARGV *argvp);

/**
 * 根据源字符串及分隔字符串生成一个字符串动态数组
 * @param str {const char*} 源字符串
 * @param delim {const char*} 分隔字符串
 * @return {ARGV*}
 */
ARGV *argv_split(const char *str, const char *delim);

#define ARGV_END	((char *) 0)

# ifdef	__cplusplus
}
# endif

#endif

