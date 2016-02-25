#ifndef ACL_VSTRING_INCLUDE_H
#define ACL_VSTRING_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include <stdarg.h>
#include "acl_vbuf.h"
#include "acl_dbuf_pool.h"
#include "acl_slice.h"

/**
 * 封装了 ACL_VBUF，ACL_VSTRING 结构类型定义
 */
typedef struct ACL_VSTRING {
    ACL_VBUF        vbuf;
    ssize_t         maxlen;
    ACL_SLICE_POOL *slice;
    ACL_DBUF_POOL  *dbuf;
    ACL_FILE_HANDLE fd;
#if defined(_WIN32) || defined(_WIN64)
    ACL_FILE_HANDLE hmap;
#endif
} ACL_VSTRING;

/**
 * 初始化 ACL_VSTRING 结构并指定缺省的缓冲区大小，当用户在自己的函数或
 * 内部以 ACL_VSTRING str 方式（非动态分配方式）使用时需要用此函数进行初始化,
 * 另外，必须用 acl_vstring_free_buf 方式来释放由该函数分配的内部缓冲区
 * @param vp {ACL_VSTRING*} 对象地址，不能为空
 * @param len {size_t} 初始时缓冲区大小
 */
ACL_API void acl_vstring_init(ACL_VSTRING *vp, size_t len);

/**
 * 当以 acl_vstring_init 初始化 ACL_VSTRING 对象时需要调用此函数释放缓冲区内存
 * @param vp {ACL_VSTRING*} 对象地址，不能为空
 */
ACL_API void acl_vstring_free_buf(ACL_VSTRING *vp);

/**
 * 动态分配一个 ACL_VSTRING 对象并指定内部缓冲区的初始化大小
 * @param len {size_t} 初始时缓冲区大小
 * @return {ACL_VSTRING*} 新分配的 ACL_VSTRING 对象
 */
ACL_API ACL_VSTRING *acl_vstring_alloc(size_t len);

/**
 * 动态分配一个 ACL_VSTRING 对象并指定内部缓冲区的初始化大小，
 * 同时指定内存池对象，优化内存分配
 * @param slice {ACL_SLICE_POOL*} 切片内存池管理对象
 * @param len {size_t} 初始时缓冲区大小
 * @return {ACL_VSTRING*} 新分配的 ACL_VSTRING 对象
 */
ACL_API ACL_VSTRING *acl_vstring_slice_alloc(ACL_SLICE_POOL *slice, size_t len);

/**
 * 动态分配一个 ACL_VSTRING 对象并指定内部缓冲区的初始化大小，
 * 同时指定内存池对象，优化内存分配
 * @param slice {ACL_SLICE_POOL*} 切片内存池管理对象
 * @param len {size_t} 初始时缓冲区大小
 * @return {ACL_VSTRING*} 新分配的 ACL_VSTRING 对象
 */
ACL_API ACL_VSTRING *acl_vstring_dbuf_alloc(ACL_DBUF_POOL *dbuf, size_t len);

/**
 * 采用内存映射文件方式分配内存时，调用此函数创建 ACL_VSTRING 动态缓冲区对象
 * @param fd {ACL_FILE_HANDLE} 有效的文件句柄
 * @param max_len {ssize_t} 所映射的最大内存大小
 * @param init_len {ssize_t} 初始化时的内存映射大小
 * @return {ACL_VSTRING*} 新创建的 ACL_VSTRING 对象
 */
ACL_API ACL_VSTRING *acl_vstring_mmap_alloc(ACL_FILE_HANDLE fd,
	ssize_t max_len, ssize_t init_len);

/**
 * 设置 ACL_VSTRING 对象的属性, 目前该函数的功能还不够完善
 * @param vp {ACL_VSTRING*}
 * @param ... 由 ACL_VSTRING_CTL_XXX 表示的控制参数，结束标志为
 *  ACL_VSTRING_CTL_END
 */
ACL_API void acl_vstring_ctl(ACL_VSTRING *vp,...);

#define ACL_VSTRING_CTL_MAXLEN      1
#define ACL_VSTRING_CTL_END         0

/**
 * 将缓冲区内的数据截短至指定长度，同时保证缓冲区数据以 '\0' 结尾
 * @param vp {ACL_VSTRING*}
 * @param len {size_t} 截短后的长度
 * @return {ACL_VSTRING*} 与 vp 相同
 */
ACL_API ACL_VSTRING *acl_vstring_truncate(ACL_VSTRING *vp, size_t len);

/**
 * 释放由 acl_vstring_alloc 动态分配的 ACL_VSTRING 对象
 * @param vp {ACL_VSTRING*}
 */
ACL_API void acl_vstring_free(ACL_VSTRING *vp);

/**
 * 拷贝字符串
 * @param vp {ACL_VSTRING*}
 * @param src {const char*} 源字符串
 * @return {ACL_VSTRING*} 与 vp 相同
 */
ACL_API ACL_VSTRING *acl_vstring_strcpy(ACL_VSTRING *vp, const char *src);

/**
 * 拷贝字符串，但不得超过规定长度限制
 * @param vp {ACL_VSTRING*}
 * @param src {const char*} 源字符串
 * @param len {size_t} 规定长度限制
 * @return {ACL_VSTRING*} 与 vp 相同
 */
ACL_API ACL_VSTRING *acl_vstring_strncpy(ACL_VSTRING *vp,
		const char *src, size_t len);

/**
 * 附加拷贝字符串
 * @param vp {ACL_VSTRING*}
 * @param src {const char*} 源字符串
 * @return {ACL_VSTRING*} 与 vp 相同
 */
ACL_API ACL_VSTRING *acl_vstring_strcat(ACL_VSTRING *vp, const char *src);

/**
 * 附加拷贝字符串，但不得超过规定长度限制
 * @param vp {ACL_VSTRING*}
 * @param src {const char*} 源字符串
 * @param len {size_t} 规定长度限制
 * @return {ACL_VSTRING*} 与 vp 相同
 */
ACL_API ACL_VSTRING *acl_vstring_strncat(ACL_VSTRING *vp,
		const char *src, size_t len);

/**
 * 拷贝内存区数据，同时保证目标缓冲区尾部置 '\0'
 * @param vp {ACL_VSTRING*}
 * @param src {const char*} 源数据地址
 * @param len {size_t} 源数据长度
 * @return {ACL_VSTRING*} 与 vp 相同
 */
ACL_API ACL_VSTRING *acl_vstring_memcpy(ACL_VSTRING *vp,
		const char *src, size_t len);

/**
 * 移动内存区数据, 源数据与目标地址的内存区可以是同一块内存区也可以不是
 * 同一块内存区，该函数保证目标地址尾部以 '\0' 结尾
 * @param vp {ACL_VSTRING*}
 * @param src {const char*} 源数据地址
 * @param len {size_t} 源数据长度
 * @return {ACL_VSTRING*} 与 vp 相同
 */
ACL_API ACL_VSTRING *acl_vstring_memmove(ACL_VSTRING *vp,
		const char *src, size_t len);

/**
 * 拷贝内存区，该函数保证目标缓冲区以 '\0' 结尾
 * @param vp {ACL_VSTRING*}
 * @param src {const char*} 源数据地址
 * @param len {size_t} 源数据长度
 * @return {ACL_VSTRING*} 与 vp 相同
 */
ACL_API ACL_VSTRING *acl_vstring_memcat(ACL_VSTRING *vp,
		const char *src, size_t len);

/**
 * 查找某个字符
 * @param vp {ACL_VSTRING*}
 * @param ch {int} 要查找的字符
 * @return {char*} 目标字符所在位置的地址, 如果未查到则返回 NULL, 
 *  注：该返回地址是不能被单独释放的，因为其由 ACL_VSTRING 对象统一进行管理
 */
ACL_API char *acl_vstring_memchr(ACL_VSTRING *vp, int ch);

/**
 * 查找某个字符串，字符串大小写敏感
 * @param vp {ACL_VSTRING*}
 * @param needle {const char*} 要查找的字符
 * @return {char*} 目标字符所在位置的地址, 如果未查到则返回 NULL, 
 *  注：该返回地址是不能被单独释放的，因为其由 ACL_VSTRING 对象统一进行管理
 */
ACL_API char *acl_vstring_strstr(ACL_VSTRING *vp, const char *needle);

/**
 * 查找某个字符串，忽略字符串大小写
 * @param vp {ACL_VSTRING*}
 * @param needle {const char*} 要查找的字符
 * @return {char*} 目标字符所在位置的地址, 如果未查到则返回 NULL, 
 *  注：该返回地址是不能被单独释放的，因为其由 ACL_VSTRING 对象统一进行管理
 */
ACL_API char *acl_vstring_strcasestr(ACL_VSTRING *vp, const char *needle);

/**
 * 从后向前查找字符串，字符串大小写敏感
 * @param vp {ACL_VSTRING*}
 * @param needle {const char*} 要查找的字符
 * @return {char*} 目标字符所在位置的地址, 如果未查到则返回 NULL, 
 *  注：该返回地址是不能被单独释放的，因为其由 ACL_VSTRING 对象统一进行管理
 */
ACL_API char *acl_vstring_rstrstr(ACL_VSTRING *vp, const char *needle);

/**
 * 从后向前查找字符串，字符串大小写不敏感
 * @param vp {ACL_VSTRING*}
 * @param needle {const char*} 要查找的字符
 * @return {char*} 目标字符所在位置的地址, 如果未查到则返回 NULL,
 *  注：该返回地址是不能被单独释放的，因为其由 ACL_VSTRING 对象统一进行管理
 */
ACL_API char *acl_vstring_rstrcasestr(ACL_VSTRING *vp, const char *needle);

/**
 * 向缓冲区的某个指定位置后添加数据，同时保证目标缓冲区数据以 '\0' 结尾
 * @param vp {ACL_VSTRING*}
 * @param start {size_t} 指定的位置
 * @param buf {const char*} 数据地址
 * @param len {size_t} 数据长度
 * @return {ACL_VSTRING*} 与 vp 相同
 */
ACL_API ACL_VSTRING *acl_vstring_insert(ACL_VSTRING *vp, size_t start,
		const char *buf, size_t len);

/**
 * 向缓冲区的头部添加数据，同时保证目标缓冲区数据以 '\0' 结尾
 * @param vp {ACL_VSTRING*}
 * @param buf {const char*} 数据地址
 * @param len {size_t} 数据长度
 * @return {ACL_VSTRING*} 与 vp 相同
 */
ACL_API ACL_VSTRING *acl_vstring_prepend(ACL_VSTRING *vp,
		const char *buf, size_t len);

/**
 * 向缓冲区按格式方式添加数据
 * @param vp {ACL_VSTRING*}
 * @param format {const char*} 格式化字符串
 * @param ... 变参序列
 * @return {ACL_VSTRING*} 与 vp 相同
 */
ACL_API ACL_VSTRING *ACL_PRINTF(2, 3) acl_vstring_sprintf(ACL_VSTRING *vp,
		const char *format,...);

/**
 * 以附加方式向缓冲区按格式方式添加数据
 * @param vp {ACL_VSTRING*}
 * @param format {const char*} 格式化字符串
 * @param ... 变参序列
 * @return {ACL_VSTRING*} 与 vp 相同
 */
ACL_API ACL_VSTRING *ACL_PRINTF(2, 3) acl_vstring_sprintf_append(
		ACL_VSTRING *vp, const char *format,...);

/**
 * 导出缓冲区内的数据区同时将 ACL_VSTRING 对象释放，用户需要单独调用
 * acl_myfree 来释放返回的数据区内存
 * @param vp {ACL_VSTRING*}
 * @return {char*} 数据区地址，当返回值不为 NULL 时用户需要单独调用
 *  acl_myfree 来释放该地址，否则会造成内存泄漏
 */
ACL_API char *acl_vstring_export(ACL_VSTRING *vp);

/**
 * 将用户的存储字符串的动态分配内存区导入并生成新的 ACL_VSTRING 对象
 * @param str {char*} 外部动态分配的存储字符串的内存地址
 * @return {ACL_VSTRING*} 新分配的 ACL_VSTRING 对象
 */
ACL_API ACL_VSTRING *acl_vstring_import(char *str);

/**
 * 将动态内存区与 ACL_VSTRING 粘合
 * 注：vp 不能是调用 acl_vstring_alloc 产生的，并且不能调用 acl_vstring_init
 *     进行过初始化, vp 可以是由 acl_mymalloc 产生或位于栈上的一个变量
 *    （如: ACL_VSTRING v）
 * @param vp {ACL_VSTRING*} 需由 acl_mymalloc 生成或是一个栈变量, 当以
 *  acl_mymalloc 方式生成的时应该通过 acl_myfree 释放它
 * @param buf {void*} 用户传递的内存区, 可以是栈变量
 * @param len {size_t} buf 内存区的长度
 */
ACL_API void acl_vstring_glue(ACL_VSTRING *vp, void *buf, size_t len);

/**
 * 取得某个位置的字符
 * @param vp {ACL_VSTRING*}
 * @param len {size_t} 位置，如果该值越界，则函数内部会 fatal
 * @return {char} 查找的字符
 */
ACL_API char acl_vstring_charat(ACL_VSTRING *vp, size_t len);

/**
 * 按规定格式添加数据
 * @param vp {ACL_VSTRING*}
 * @param format {const char*}
 * @param ap {va_list}
 * @return {ACL_VSTRING*} 与 vp 相同
 * @see acl_vstring_sprintf
 */
ACL_API ACL_VSTRING *acl_vstring_vsprintf(ACL_VSTRING *vp,
		const char *format, va_list ap);

/**
 * 按规定格式向尾部添加数据
 * @param vp {ACL_VSTRING*}
 * @param format {const char*}
 * @param ap {va_list}
 * @return {ACL_VSTRING*} 与 vp 相同
 */
ACL_API ACL_VSTRING *acl_vstring_vsprintf_append(ACL_VSTRING *vp,
		const char *format, va_list ap);

/**
 * 按规定格式向头部添加数据
 * @param vp {ACL_VSTRING*}
 * @param format {const char*}
 * @param ... 变参序列
 * @return {ACL_VSTRING*} 与 vp 相同
 */
ACL_API ACL_VSTRING *ACL_PRINTF(2, 3) acl_vstring_sprintf_prepend(
		ACL_VSTRING *vp, const char *format, ...);

/**
 * 从源串中获得一行数据(不含 "\r\n" 和 "\n")，同时将剩余数据缓存起来, 如果
 * 未获得完整行，则只缓存源串
 * @param vp {ACL_VSTRING*} 字符串数据缓存区
 * @param src {const char**} 源字符串指针地址, 返回后指针地址移动至下一位置
 * @param dlen {size_t} 源字符串数据长度
 * @return {const ACL_VSTRING*} NULL, 表示未找到 "\r\n" 或 "\n"，但会将剩余
 *  的数据拷贝至缓冲区内，应用需要通过 ACL_VSTRING_LEN 判断缓冲区中是否还有
 *  数据；!NULL，表示读到完整行
 *  注：读到完整行后应该调用 ACL_VSTRING_RESET(vp) 将缓冲区清空
 */
ACL_API const ACL_VSTRING *acl_buffer_gets_nonl(ACL_VSTRING *vp,
		const char **src, size_t dlen);

/**
 * 从源串中获得一行数据(包含 "\r\n" 或 "\n")，同时将剩余数据缓存起来, 
 * 如果未获得完整行，则只缓存源串
 * @param vp {ACL_VSTRING*} 字符串数据缓存区
 * @param src {const char**} 源字符串指针地址, 返回后指针地址移动至下一位置
 * @param dlen {size_t} 源字符串数据长度
 * @return {const ACL_VSTRING*} NULL, 表示未找到 "\r\n" 或 "\n"，但会将剩余
 *  的数据拷贝至缓冲区内，应用需要通过 ACL_VSTRING_LEN 判断缓冲区中是否还有
 *  数据；!NULL，表示读到完整行
 *  注：读到完整行后应该调用 ACL_VSTRING_RESET(vp) 将缓冲区清空
 */
ACL_API const ACL_VSTRING *acl_buffer_gets(ACL_VSTRING *vp,
		const char **src, size_t dlen);

 /*
  * Macros. Unsafe macros have UPPERCASE names.
  */
#define ACL_VSTRING_SPACE(vp, len) ((vp)->vbuf.space(&(vp)->vbuf, len))

/**
 * 取得当前 ACL_VSTRING 数据存储地址
 * @param vp {ACL_VSTRING*}
 * @return {char*}
 */
#define acl_vstring_str(vp) ((char *) (vp)->vbuf.data)

/**
 * 取得当前 ACL_VSTRING 所存储的数据的长度
 * @param vp {ACL_VSTRING*}
 * @return {int}
 */
#define ACL_VSTRING_LEN(vp) (size_t) ((vp)->vbuf.ptr - (vp)->vbuf.data)

/**
 * 取得当前 ACL_VSTRING 内部缓冲区的总大小
 * @param vp {ACL_VSTRING*}
 * @return {int}
 */
#define	ACL_VSTRING_SIZE(vp) ((vp)->vbuf.len)

/**
 * 取得当前 ACL_VSTRING 的数据偏移指针位置
 * @param vp {ACL_VSTRING*}
 * @return {char*}
 */
#define acl_vstring_end(vp) ((char *) (vp)->vbuf.ptr)

/**
 * 将 ACL_VSTRING 的数据偏移指针位置置 0
 * @param vp {ACL_VSTRING*}
 */
#define ACL_VSTRING_TERMINATE(vp) { \
	if ((vp)->vbuf.cnt <= 0) \
		ACL_VSTRING_SPACE((vp), 1); \
	if ((vp)->vbuf.cnt > 0) \
		*(vp)->vbuf.ptr = 0; \
	else if ((vp)->vbuf.ptr > (vp)->vbuf.data) { \
		(vp)->vbuf.ptr--; \
		*(vp)->vbuf.ptr = 0; \
		(vp)->vbuf.cnt++; \
	} \
}

/**
 * 重置 ACL_VSTRING 内部缓冲区指针地址起始位置，但不会将尾部数据置 0，应用可以
 * 通过调用 ACL_VSTRING_TERMINATE 将缓冲数据尾部置 0
 * @param vp {ACL_VSTRING*}
 */
#define ACL_VSTRING_RESET(vp) {	\
	(vp)->vbuf.ptr = (vp)->vbuf.data; \
	(vp)->vbuf.cnt = (vp)->vbuf.len; \
	acl_vbuf_clearerr(&(vp)->vbuf); \
}

/**
 * 添加一个字符至 ACL_VSTRING 缓冲区
 * @param vp {ACL_VSTRING*}
 * @param ch {int} 字符
 */
#define	ACL_VSTRING_ADDCH(vp, ch) ACL_VBUF_PUT(&(vp)->vbuf, ch)

/**
 * 移动数据偏移指针至内部缓冲区尾部
 * @param vp {ACL_VSTRING*}
 */
#define ACL_VSTRING_SKIP(vp) { \
	while ((vp)->vbuf.cnt > 0 && *(vp)->vbuf.ptr) \
		(vp)->vbuf.ptr++, (vp)->vbuf.cnt--; \
}

/**
 * 当前 ACL_VSTRING 中还有多少数据可用
 * @param vp {ACL_VSTRING*}
 */
#define acl_vstring_avail(vp) ((vp)->vbuf.cnt)

 /**
  * The following macro is not part of the public interface, because it can
  * really screw up a buffer by positioning past allocated memory.
  */
#define ACL_VSTRING_AT_OFFSET(vp, offset) { \
	(vp)->vbuf.ptr = (vp)->vbuf.data + (offset); \
	(vp)->vbuf.cnt = (vp)->vbuf.len - (offset); \
}

#ifdef  __cplusplus
}
#endif

#endif
