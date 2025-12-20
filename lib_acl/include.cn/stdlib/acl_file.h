#ifndef	ACL_FILE_INCLUDE_H
#define	ACL_FILE_INCLUDE_H

#include "acl_define.h"
#include "acl_vstream.h"
#include <stdarg.h>

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * 文件流类型定义
 */
typedef struct ACL_FILE {
	ACL_VSTREAM *fp;	/**< 流指针 */
	unsigned int status;	/**< 文件流状态 */
#define	ACL_FILE_EOF		(1 << 0)
	int   errnum;		/**< 文件流的出错错误号 */
} ACL_FILE;

#define	ACL_FPATH(fp)	ACL_VSTREAM_PATH((fp)->fp)
#define	ACL_FSTREAM(fp)	((fp)->fp)

/**
 * 打开或创建一个文件进行读写操作
 * @param filename {const char*} 文件名
 * @param mode {const char*} 打开标志.
 *  r or rb: 只读方式打开已经存在文件
 *  w or wb: 只写方式打开已存在文件并清空或只写方式创建新文件
 *  a or ab: 尾部附加只写方式打开已存在文件或创建新文件
 *  r+ or rb+: 以读写方式打开已存在文件
 *  w+ or wb+: 以读写方式打开已存在文件并清空或创建新文件
 *  a+ or ab+: 以尾部附加读写方式打开已存在文件或创建新文件
 */
ACL_API ACL_FILE *acl_fopen(const char *filename, const char *mode);

/**
 * 关闭一个文件流
 * @param fp {ACL_FILE*} 文件流
 */
ACL_API int acl_fclose(ACL_FILE *fp);

/**
 * 清除文件流的错误号
 * @param fp {ACL_FILE*} 文件流
 */
ACL_API void acl_clearerr(ACL_FILE *fp);

/**
 * 判断是否到达文件尾部
 * @param fp {ACL_FILE*} 文件流
 * @return {int} 0: 否; !0: 是
 */
ACL_API int acl_feof(ACL_FILE *fp);

/**
 * 从文件流中读取一些固定长度的数据块
 * @param buf {void*} 内存缓冲区地址
 * @param size {size_t} 每个数据块长度
 * @param nitems {size_t} 数据块个数
 * @param fp {ACL_FILE*} 文件流
 * @return {size_t} 数据块个数, 若出错则返回 EOF
 */
ACL_API size_t acl_fread(void *buf, size_t size, size_t nitems, ACL_FILE *fp);

/**
 * 从文件流中读取一行数据
 * @param buf {char*} 缓冲区地址
 * @param size {int} buf 空间大小
 * @param fp {ACL_FILE*} 文件流
 * @return {char*} NULL: 未读到完整行数据; !NULL: 读到完整行数据
 */
ACL_API char *acl_fgets(char *buf, int size, ACL_FILE *fp);

/**
 * 从文件流中读取一行数据，但返回的数据尾部不包含 "\r\n"
 * @param buf {char*} 缓冲区地址
 * @param size {int} buf 空间大小
 * @param fp {ACL_FILE*} 文件流
 * @return {char*} NULL: 未读到完整行数据; !NULL: 读到完整行数据
 */
ACL_API char *acl_fgets_nonl(char *buf, int size, ACL_FILE *fp);

/**
 * 从文件流中读取一个字符
 * @param fp {ACL_FILE*} 文件流
 * @return {int} EOF: 到达文件尾或出错; !EOF: 正确读到一个字符的 ASCII 码
 */
ACL_API int acl_fgetc(ACL_FILE *fp);
#define	acl_getc	acl_fgetc

/**
 * 从标准输入中读取一行数据
 * @param buf {char*} 缓存区地址
 * @param size {int} buf 空间大小
 * @return {char*} NULL: 读结束或出错; !NULL: 应与 buf 相同地址
 */
ACL_API char *acl_gets(char *buf, size_t size);

/**
 * 从标准输入中读取一行数据, 但数据尾部不包含 "\r\n"
 * @param buf {char*} 缓存区地址
 * @param size {int} buf 空间大小
 * @return {char*} NULL: 读结束或出错; !NULL: 应与 buf 相同地址
 */
ACL_API char *acl_gets_nonl(char *buf, size_t size);

/**
 * 从标准输入中读取一个字符
 * @return {int} EOF: 到达文件尾或出错; !EOF: 正确读到一个字符的 ASCII 码
 */
ACL_API int acl_getchar(void);

/**
 * 向文件流中写入变参格式数据
 * @param fp {ACL_FILE*} 文件流句柄
 * @param fmt {const char*} 变参格式
 * @param ... 变参
 * @return {size_t} 数据长度, 若出错则返回 EOF
 */
ACL_API int ACL_PRINTF(2, 3) acl_fprintf(ACL_FILE *fp, const char *fmt, ...);

/**
 * 向文件流中写入变参格式数据
 * @param fp {ACL_FILE*} 文件流句柄
 * @param fmt {const char*} 变参格式
 * @param ap {va_list} 变参列表
 * @return {size_t} 数据长度, 若出错则返回 EOF
 */
ACL_API int acl_vfprintf(ACL_FILE *fp, const char *fmt, va_list ap);

/**
 * 向文件流中写入一些固定长度的数据块
 * @param ptr {const void*} 数据地址
 * @param size {size_t} 每个数据块长度
 * @param nitems {size_t} 数据块个数
 * @param fp {ACL_FILE*} 文件流指针
 * @return {size_t} 数据块个数, 若出错则返回 EOF
 */
ACL_API size_t acl_fwrite(const void *ptr, size_t size, size_t nitems, ACL_FILE *fp);

/**
 * 向文件流中写入数据并自动在尾部添加 "\r\n"
 * @param s {const char*} 字符串地址
 * @param fp {ACL_FILE*} 文件流指针
 * @return {int} 写入的数据量(包含 "\r\n"), 若出错则返回 EOF
 */
ACL_API int acl_fputs(const char *s, ACL_FILE *fp);

/**
 * 向标准输出流中写入变参格式数据
 * @param fmt {const char*} 变参格式
 * @param ... 变参
 * @return {size_t} 数据长度, 若出错则返回 EOF
 */
ACL_API int ACL_PRINTF(1, 2) acl_printf(const char *fmt, ...);

/**
 * 向标准输出流中写入变参格式数据
 * @param fmt {const char*} 变参格式
 * @param ap {va_list} 变参列表
 * @return {size_t} 数据长度, 若出错则返回 EOF
 */
ACL_API int acl_vprintf(const char *fmt, va_list ap);

/**
 * 向文件流中写入一个字节
 * @param c {int} 一个符的 ASCII 码
 * @param fp {ACL_FILE*} 文件流指针
 * @return {int} 写入的数据量, 若出错则返回 EOF
 */
ACL_API int acl_putc(int c, ACL_FILE *fp);
#define	acl_fputc	acl_putc

/**
 * 向标准输出中写入数据并自动在尾部添加 "\r\n"
 * @param s {const char*} 字符串地址
 * @return {int} 写入的数据量(包含 "\r\n"), 若出错则返回 EOF
 */
ACL_API int acl_puts(const char *s);

/**
 * 向文件流中写入一个字节
 * @param c {int} 一个符的 ASCII 码
 * @return {int} 写入的数据量, 若出错则返回 EOF
 */
ACL_API int acl_putchar(int c);

/**
 * 定位文件位置
 * @param fp {ACL_FILE*} 文件流
 * @param offset {acl_off_t} 偏移位置
 * @param whence {int} 偏移方向, SEEK_SET, SEEK_CUR, SEEK_END
 * @return ret {acl_off_t}, ret >= 0: 正确, ret < 0: 出错
 */
ACL_API acl_off_t acl_fseek(ACL_FILE *fp, acl_off_t offset, int whence);

/**
 * 获得当前文件指针在文件中的位置
 * @param fp {ACL_FILE*} 文件句柄
 * @return {acl_off_t} 返回值 -1 表示出错
 */
ACL_API acl_off_t acl_ftell(ACL_FILE *fp);

#ifdef	__cplusplus
}
#endif

#endif
