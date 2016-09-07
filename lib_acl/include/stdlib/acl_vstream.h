#ifndef	ACL_VSTREAM_INCLUDE_H
#define	ACL_VSTREAM_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include <time.h>
#include <sys/types.h>

#ifdef	ACL_UNIX
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <netinet/in.h>
#endif

#include "acl_array.h"
#include "acl_htable.h"
#include "acl_vstring.h"

#define	ACL_VSTREAM_EOF		(-1)		/* no more space or data */

#ifdef	ACL_UNIX
# ifndef	O_RDONLY
#  define	O_RDONLY	0
# endif
# ifndef	O_WRONLY
#  define	O_WRONLY	1
# endif
# ifndef	O_RDWR
#  define	O_RDWR		2
# endif
#endif

#define	ACL_VSTREAM_BUFSIZE	4096

typedef struct ACL_VSTREAM	ACL_VSTREAM;

typedef int (*ACL_VSTREAM_RD_FN)(ACL_SOCKET fd, void *buf, size_t size,
	int timeout, ACL_VSTREAM *fp, void *context);
typedef int (*ACL_VSTREAM_WR_FN)(ACL_SOCKET fd, const void *buf,
	size_t size, int timeout, ACL_VSTREAM *fp, void *context);
typedef int (*ACL_VSTREAM_WV_FN)(ACL_SOCKET fd, const struct iovec *vec,
	int count, int timeout, ACL_VSTREAM *fp, void *context);
typedef int (*ACL_FSTREAM_RD_FN)(ACL_FILE_HANDLE fh, void *buf, size_t size,
	int timeout, ACL_VSTREAM *fp, void *context);
typedef int (*ACL_FSTREAM_WR_FN)(ACL_FILE_HANDLE fh, const void *buf,
	size_t size, int timeout, ACL_VSTREAM *fp, void *context);
typedef int (*ACL_FSTREAM_WV_FN)(ACL_FILE_HANDLE fh, const struct iovec *vec,
	int count, int timeout, ACL_VSTREAM *fp, void *context);

/* 当关闭或释放一个数据流时, 需要回调一些释放函数, 此结果定义了该回调
 * 函数的句柄类型 ---add by zsx, 2006.6.20
 */
typedef struct ACL_VSTREAM_CLOSE_HANDLE {
	void (*close_fn)(ACL_VSTREAM*, void*);
	void *context;
} ACL_VSTREAM_CLOSE_HANDLE;

/* 数据读写流类型定义 */
struct ACL_VSTREAM {
	union {
		ACL_SOCKET      sock;   /**< the master socket */
		ACL_FILE_HANDLE h_file; /**< the file handle */
	} fd;

	int   is_nonblock;              /**< just for WINDOWS, because the ioctlsocket is too weak */
	int   type;                     /**< defined as: ACL_VSTREAM_TYPE_XXX */
#define	ACL_VSTREAM_TYPE_SOCK           (1 << 0)
#define	ACL_VSTREAM_TYPE_FILE           (1 << 1)
#define	ACL_VSTREAM_TYPE_LISTEN		(1 << 2)
#define	ACL_VSTREAM_TYPE_LISTEN_INET    (1 << 3)
#define	ACL_VSTREAM_TYPE_LISTEN_UNIX    (1 << 4)
#define ACL_VSTREAM_TYPE_LISTEN_IOCP    (1 << 5)

	acl_off_t offset;               /**< cached seek info */
	acl_off_t sys_offset;           /**< cached seek info */

	unsigned char *wbuf;            /**< used when call acl_vstream_buffed_writen */
	int   wbuf_size;                /**< used when call acl_vstream_buffed_writen */
	int   wbuf_dlen;                /**< used when call acl_vstream_buffed_writen */

	unsigned char *read_buf;        /**< read buff */
	int   read_buf_len;             /**< read_buf's capacity */
	int   read_cnt;                 /**< data's length in read_buf */
	unsigned char *read_ptr;        /**< pointer to next position in read_buf */
	int   read_ready;               /**< if the system buffer has some data */

	acl_off_t total_read_cnt;       /**< total read count of the fp */
	acl_off_t total_write_cnt;      /**< total write count of the fp */

	void *ioctl_read_ctx;           /**< only for acl_ioctl_xxx in acl_ioctl.c */
	void *ioctl_write_ctx;          /**< only for acl_ioctl_xxx in acl_ioctl.c */
	void *fdp;                      /**< only for event */

	unsigned int flag;              /**< defined as: ACL_VSTREAM_FLAG_XXX */
#define	ACL_VSTREAM_FLAG_READ           (1 << 0)
#define	ACL_VSTREAM_FLAG_WRITE          (1 << 1)
#define	ACL_VSTREAM_FLAG_RW             (1 << 2)
#define ACL_VSTREAM_FLAG_CACHE_SEEK     (1 << 3)
#define	ACL_VSTREAM_FLAG_DEFER_FREE	(1 << 4)	/**< 延迟关闭 */

#define	ACL_VSTREAM_FLAG_ERR            (1 << 10)	/**< 其它错误 */
#define	ACL_VSTREAM_FLAG_EOF            (1 << 11)	/**< 结束 */
#define	ACL_VSTREAM_FLAG_TIMEOUT        (1 << 12)	/**< 超时 */
#define	ACL_VSTREAM_FLAG_RDSHORT        (1 << 13)	/**< 读的不够 */
#define ACL_VSTREAM_FLAG_BAD  (ACL_VSTREAM_FLAG_ERR \
                               | ACL_VSTREAM_FLAG_EOF \
                               | ACL_VSTREAM_FLAG_TIMEOUT)
#define	ACL_VSTREAM_FLAG_CLIENT         (1 << 14)
#define	ACL_VSTREAM_FLAG_CONNECT        (1 << 15)
#define	ACL_VSTREAM_FLAG_SOCKPAIR       (1 << 16)

#define	ACL_VSTREAM_FLAG_TAGYES	        (1 << 17) /* 若读到要求的标志位则置位 */

#define	ACL_VSTREAM_FLAG_CONNECTING     (1 << 18) /* 正在连接过程中 */
#define	ACL_VSTREAM_FLAG_PREREAD	(1 << 19) /* 对于 acl_vstream_can_read 调用过程是否允许预读 */

	char  errbuf[128];              /**< error info */
	int   errnum;                   /**< record the system errno here */
	int   rw_timeout;               /**< read/write timeout */
	char *addr_local;               /**< the local addr of the fp */
	char *addr_peer;                /**< the peer addr of the fp */
	struct sockaddr_in *sa_local;
	struct sockaddr_in *sa_peer;
	size_t sa_local_size;
	size_t sa_peer_size;
	size_t sa_local_len;
	size_t sa_peer_len;
	char *path;                     /**< the path just for file operation */
	void *context;                  /**< the application's special data */

	ACL_ARRAY *close_handle_lnk;    /**< before this fp is free,
	                                 * function in close_handle_lnk
	                                 * will be called.
	                                 * add by zsx, 2006.6.20
	                                 */
	int (*sys_getc)(ACL_VSTREAM*);  /**< called by ACL_VSTREAM_GETC()/1 */
	ACL_VSTREAM_RD_FN read_fn;      /**< system socket read API */
	ACL_VSTREAM_WR_FN write_fn;     /**< system socket write API */
	ACL_VSTREAM_WV_FN writev_fn;    /**< system socket writev API */

	ACL_FSTREAM_RD_FN fread_fn;     /**< system file read API */
	ACL_FSTREAM_WR_FN fwrite_fn;    /**< system file write API */
	ACL_FSTREAM_WV_FN fwritev_fn;   /**< system file writev API */

	int (*close_fn)(ACL_SOCKET);    /**< system socket close API */
	int (*fclose_fn)(ACL_FILE_HANDLE);  /**< system file close API */

	unsigned int oflags;            /**< the system's open flags */
	/* general flags(ANSI):
	 * O_RDONLY: 0x0000, O_WRONLY: 0x0001, O_RDWR: 0x0002, O_APPEND: 0x0008,
	 * O_CREAT: 0x0100, O_TRUNC: 0x0200, O_EXCL: 0x0400;
	 * just for win32:
	 * O_TEXT: 0x4000, O_BINARY: 0x8000, O_RAW: O_BINARY,
	 * O_SEQUENTIAL: 0x0020, O_RANDOM: 0x0010.
	 */

	int   nrefer;                   /**< refer count, used for engine moudle */

#if defined(_WIN32) || defined(_WIN64)
	int   pid;
	HANDLE hproc;
	ACL_SOCKET iocp_sock;
#elif defined(ACL_UNIX)
	pid_t pid;
#endif
	ACL_HTABLE *objs_table;
};

extern ACL_API ACL_VSTREAM acl_vstream_fstd[];  /**< pre-defined streams */
#define ACL_VSTREAM_IN          (&acl_vstream_fstd[0]) /**< 标准输入 */
#define ACL_VSTREAM_OUT         (&acl_vstream_fstd[1]) /**< 标准输出 */
#define ACL_VSTREAM_ERR         (&acl_vstream_fstd[2]) /**< 标准错误输出 */

/*--------------------------------------------------------------------------*/
/**
 * 初始化ACL_VSTREAM流的函数库
 * 对于_WIN32来说，如果想要用标准输入输出，则需要调用此函数进行初始化
 */
ACL_API void acl_vstream_init(void);

/**
 * 功能: 探测流中有多少数据, 包含缓冲区中的数据与系统缓冲区的数据
 * @param fp {ACL_VSTREAM*} 流指针, 不能为空
 * @return ret {int}, ret > 0 OK; ret <= 0 Error
 * 注: 仅适应于网络套接字
 */
ACL_API int acl_vstream_peekfd(ACL_VSTREAM *fp);

/**
 * 克隆一个ACL_VSTREAM流，除ioctl_read_ctx, ioctl_write_ctx, fdp
 * 外所有数据都拷贝，如果是动态内存数据，则新的流将在内部动态分配
 * 内存且将源数据进行拷贝
 * @param stream_src {ACL_VSTREAM*} 源流指针
 * @return {ACL_VSTREAM*} 目的流指针
 */
ACL_API ACL_VSTREAM *acl_vstream_clone(const ACL_VSTREAM *stream_src);

/**
 * 设置数据流的类型，该函数将根据所给类型设定用于该数据流上的读、写、关闭函数
 * @param fp {ACL_VSTREAM*} 流指针, 不能为空
 * @param type {int} 数据流的类型，defined above: ACL_VSTREAM_TYPE_XXX
 * @return ret {int}, ret >= 0 OK; ret < 0 Error
 */
ACL_API int acl_vstream_set_fdtype(ACL_VSTREAM *fp, int type);

/**
 * 分配一文件句柄所对应的数据流
 * @param fh {ACL_FILE_HANDLE} 文件句柄
 * @param oflags {unsigned int} 标志位, We're assuming that O_RDONLY: 0x0000,
 *  O_WRONLY: 0x0001, O_RDWR: 0x0002, O_APPEND: 0x0008, O_CREAT: 0x0100,
 *  O_TRUNC: 0x0200, O_EXCL: 0x0400; just for win32, O_TEXT: 0x4000,
 *  O_BINARY: 0x8000, O_RAW: O_BINARY, O_SEQUENTIAL: 0x0020, O_RANDOM: 0x0010.
 *  同时设置
 * @return {ACL_VSTREAM*} 数据流句柄
 */
ACL_API ACL_VSTREAM *acl_vstream_fhopen(ACL_FILE_HANDLE fh, unsigned int oflags);

/**
 * 分配一个数据流
 * @param fd {ACL_SOCKET} 描述符(可以为网络描述字也可以为文件描述字)
 * @param oflags {unsigned int} 标志位, We're assuming that O_RDONLY: 0x0000,
 *  O_WRONLY: 0x0001, O_RDWR: 0x0002, O_APPEND: 0x0008, O_CREAT: 0x0100,
 *  O_TRUNC: 0x0200, O_EXCL: 0x0400; just for win32, O_TEXT: 0x4000,
 *  O_BINARY: 0x8000, O_RAW: O_BINARY, O_SEQUENTIAL: 0x0020, O_RANDOM: 0x0010.
 * @param buflen {size_t} 内置缓冲区的大小
 * @param rw_timeo {int} 读写超时时间(以秒为单位)
 * @param fdtype {int} ACL_VSTREAM_TYPE_FILE, ACL_VSTREAM_TYPE_SOCK,
 *  ACL_VSTREAM_TYPE_LISTEN | ACL_VSTREAM_TYPE_LISTEN_INET | ACL_VSTREAM_TYPE_LISTEN_UNIX
 * @return ret {ACL_VSTREAM*}, ret == NULL: 出错, ret != NULL: OK
 */
ACL_API ACL_VSTREAM *acl_vstream_fdopen(ACL_SOCKET fd, unsigned int oflags,
		size_t buflen, int rw_timeo, int fdtype);

/**
 * 打开一个文件的数据流
 * @param path {const char*} 文件名
 * @param oflags {unsigned int} 标志位, We're assuming that O_RDONLY: 0x0000,
 *  O_WRONLY: 0x0001, O_RDWR: 0x0002, O_APPEND: 0x0008, O_CREAT: 0x0100,
 *  O_TRUNC: 0x0200, O_EXCL: 0x0400; just for win32, O_TEXT: 0x4000,
 *  O_BINARY: 0x8000, O_RAW: O_BINARY, O_SEQUENTIAL: 0x0020, O_RANDOM: 0x0010.
 * @param mode {int} 打开文件句柄时的模式(如: 0600)
 * @param buflen {size_t} 内置缓冲区的大小
 * @return ret {ACL_VSTREAM*}, ret== NULL: 出错, ret != NULL: OK
 */
ACL_API ACL_VSTREAM *acl_vstream_fopen(const char *path, unsigned int oflags,
		int mode, size_t buflen);

/**
 * 读取整个文件内容于内存中
 * @param path {const char*} 文件名, 如: /opt/acl/conf/service/test.cf
 * @return {char*} 存有文件全部内容的缓冲区, 用完后用户需要调用 acl_myfree
 *  释放该内存区
 */
ACL_API char *acl_vstream_loadfile(const char *path);

/**
 * 读取整个文件内容于内存中
 * @param path {const char*} 文件名, 如: /opt/acl/conf/service/test.cf
 * @param size {ssize_t*} 如果非空，则该值存储返回的缓冲区大小，如果读取内容
 *  出错，则该值会被置 -1
 * @return {char*} 存有文件全部内容的缓冲区, 用完后用户需要调用 acl_myfree
 *  释放该内存区
 */
ACL_API char *acl_vstream_loadfile2(const char *path, ssize_t *size);

/**
 * 设置流的各个参数
 * @param fp {ACL_VSTREAM*} 流指针
 * @param name {int} 所设置的参数类型中的第一个参数类型名,
 *  defined as ACL_VSTREAM_CTL_
 */
ACL_API void acl_vstream_ctl(ACL_VSTREAM *fp, int name,...);
#define ACL_VSTREAM_CTL_END         0
#define ACL_VSTREAM_CTL_READ_FN     1
#define ACL_VSTREAM_CTL_WRITE_FN    2
#define ACL_VSTREAM_CTL_PATH        3
#define ACL_VSTREAM_CTL_FD          4
#define ACL_VSTREAM_CTL_TIMEOUT     5
#define ACL_VSTREAM_CTL_CONTEXT     6
#define	ACL_VSTREAM_CTL_CTX         ACL_VSTREAM_CTL_CONTEXT
#define ACL_VSTREAM_CTL_CACHE_SEEK  7

/**
 * 定位文件指针
 * @param fp {ACL_VSTREAM*} 数据流指针
 * @param offset {acl_off_t} 偏移量
 * @param whence {int} 偏移方向, SEEK_SET, SEEK_CUR, SEEK_END
 * @return ret {acl_off_t}, ret >= 0: 正确, ret < 0: 出错
 * 注： acl_vstream_fseek() 效率更高些, 其充分利用了缓冲区的功能,
 *      且比 acl_vstream_fseek2() 少调用一次 lseek() 系统调用.
 */
ACL_API acl_off_t acl_vstream_fseek(ACL_VSTREAM *fp, acl_off_t offset, int whence);

/**
 * 定位文件指针
 * @param fp {ACL_VSTREAM*} 数据流指针
 * @param offset {acl_off_t} 偏移量
 * @param whence {int} 移动方向：SEEK_SET（从文件起始位置后移动）,
 *  SEEK_CUR（从当前文件指针位置向后移动）, SEEK_END（从文件尾向前移动）
 * @return ret {acl_off_t}, ret >= 0: 正确, ret < 0: 出错
 * @deprecated 该函数的效率较低
 */
ACL_API acl_off_t acl_vstream_fseek2(ACL_VSTREAM *fp, acl_off_t offset, int whence);

/**
 * 返回当前文件指针所在位置
 * @param fp {ACL_VSTREAM*} 数据流指针
 * @return {acl_off_t} 当前文件指针所在位置, -1 表示出错
 */
ACL_API acl_off_t acl_vstream_ftell(ACL_VSTREAM *fp);

/**
 * 将源文件进程截断
 * @param fp {ACL_VSTREAM*} 数据流指针
 * @param length {acl_off_t} 数据长度(>=0)
 * @return {int} 0: ok, -1: error
 */
ACL_API int acl_file_ftruncate(ACL_VSTREAM *fp, acl_off_t length);

/**
 * 将源文件进程截断
 * @param path {const char*} 文件名(可以是全路径或相对路径)
 * @param length {acl_off_t} 数据长度(>=0)
 * @return {int} 0: ok, -1: error
 */
ACL_API int acl_file_truncate(const char *path, acl_off_t length);

/**
 * 查看一个文件流句柄的属性
 * @param fp {ACL_VSTREAM *} 文件流句柄
 * @param buf {acl_stat *} 存储结果的结构地址
 * @return {int} 0: ok; -1: error
 */
ACL_API int acl_vstream_fstat(ACL_VSTREAM *fp, struct acl_stat *buf);

/**
 * 查看一个文件的大小
 * @param fp {ACL_VSTREAM *} 文件流句柄
 * @return {int} >= 0: ok;  -1: error
 */
ACL_API acl_int64 acl_vstream_fsize(ACL_VSTREAM *fp);

/**
 * 从fp 流中读取一个字节
 * @param fp {ACL_VSTREAM*} 数据流指针
 * @return {int} ACL_VSTREAM_EOF(出错) 或所读到的某个字节的ASCII
 *  或为 ACL_VSTREAM_EOF: 读出错或对方关闭了连接, 应该关闭该数据流
 */
ACL_API int acl_vstream_getc(ACL_VSTREAM *fp);
#define	acl_vstream_get_char	acl_vstream_getc

/**
 * 从 fp 流中非阻塞地一次性最大读取 size 个字节
 * @param fp {ACL_VSTREAM*} 数据流指针
 * @param buf {char*} 用户传来的内存缓存区
 * @param size {int} buf 缓存区的空间大小
 * @return {int} 所读取的字节数 n, 如果 n == ACL_VSTREAM_EOF 表明出错, 否则
 *         n >= 0 正确.
 */
ACL_API int acl_vstream_nonb_readn(ACL_VSTREAM *fp, char *buf, int size);

/**
 * 判断一个给定的数据流是否已经被系统关闭了，当数据流缓存区没有数据时，
 * 该函数会调用系统的读函数（即读一个字节）来判断是否socket出错或已经
 * 关闭；如成功读取一个字节，则说明socket正常，同时将所读的数据放回缓存
 * 区, 如果读返回ACL_VSTREAM_EOF, 便需要判断错误号是否被关闭
 * @param fp {ACL_VSTREAM*} 数据流指针
 * @return {int}, 0 说明该socket正常; -1 该socket出错或已经被系统关闭
 */
ACL_API int acl_vstream_probe_status(ACL_VSTREAM *fp);

/**
 * 将一个字符放回数据流中
 * @param fp {ACL_VSTREAM*} 数据流指针
 * @param ch {int} 字符的 ASCII 码 
 * @return {int} 字符的 ASCII 码, 该函数应不会出错, 除非内部内存分配失败而产生
 *  core 文件.
 */
ACL_API int acl_vstream_ungetc(ACL_VSTREAM *fp, int ch);

/**
 * 将指定长度的数据放回至数据流中
 * @param fp {ACL_VSTREAM*} 数据流指针
 * @param ptr {const void *} 需要放回至流中的数据的起始地址
 * @param length {size_t} 需要放回至流中的数据的长度
 * @return {int} 被成功放回至流中的数据长度, 应该永不会出错, 除非内部内存分配
 *  失败而自动产生 core 文件!
 */
ACL_API int acl_vstream_unread(ACL_VSTREAM *fp, const void *ptr, size_t length);

/**
 * 从数据流中读取一行数据, 直到读到  "\n" 或读结束为止, 正常情况下包括 "\n"
 * @param fp {ACL_VSTREAM*} 数据流
 * @param vptr {void*} 用户所给的内存缓冲区指针
 * @param maxlen {size_t} vptr 缓冲区的大小
 * @return  ret {int}, ret == ACL_VSTREAM_EOF:  读出错或对方关闭了连接, 
 *  应该关闭本地数据流; n > 0:  读到 了 n 个字节的数据, 如果该 n 个数据
 *  的最后一个非 0 字符为 "\n" 表明读到了一个完整的行, 否则表明读到了 n
 *  个数据但对方未发送 "\n" 就关闭了连接; 还可以通过检查
 *  (fp->flag & ACL_VSTREAM_FLAG_TAGYES)
 *	不等于 0 来判断是否读到了 "\n", 如果非 0 则表示读到了 "\n".
 */
ACL_API int acl_vstream_gets(ACL_VSTREAM *fp, void *vptr, size_t maxlen);
#define	acl_vstream_readline	acl_vstream_gets
#define	acl_vstream_fgets	acl_vstream_gets

/**
 * 从数据流中读取一行数据, 直到读到 "\n" 或读结束为止, 返回的结果中不包括 "\n"
 * @param fp {ACL_VSTREAM*} 数据流 
 * @param vptr {void*} 用户所给的内存缓冲区指针
 * @param maxlen {size_t} vptr 缓冲区的大小
 * @return ret {int}, ret == ACL_VSTREAM_EOF:  读出错或对方关闭了连接,
 *  应该关闭本地数据流, n == 0: 读到了一行数据, 但该行数据仅有 "\r\n",
 *  n > 0:  读到 了 n 个字节的数据.
 */
ACL_API int acl_vstream_gets_nonl(ACL_VSTREAM *fp, void *vptr, size_t maxlen);

/**
 * 从数据流中获得以字符串为标志结束位的内容
 * @param fp {ACL_VSTREAM*} 类型指针
 * @param vptr {void*} 数据存储缓冲区
 * @param maxlen {size_t} vptr 缓冲区大小
 * @param tag {const char*} 字符串标志
 * @param taglen {size_t} tag 中内容的长度大小
 * @return ret {int}, ret == ACL_VSTREAM_EOF:  读出错或对方关闭了连接, 
 *  应该关闭本地数据流, n > 0:  读到 了 n 个字节的数据, 如果读到了所需要的
 *  标志串, 则 fp 流中 (fp->flag & ACL_VSTREAM_FLAG_TAGYES) 不等于 0.
 */
ACL_API int acl_vstream_readtags(ACL_VSTREAM *fp, void *vptr, size_t maxlen,
		const char *tag, size_t taglen);

/**
 * 循环读取 maxlen 个数据, 直到读到 maxlen 个字节为止或读出错
 * @param fp {ACL_VSTREAM*} 数据流
 * @param vptr {void*} 用户的数据缓冲区指针地址
 * @param maxlen {size_t} vptr 数据缓冲区的空间大小
 * @return ret {int}, ret == ACL_VSTREAM_EOF:  读出错或对方关闭了连接, 应该
 *  关闭本地数据流 n > 0:  成功读取了 maxlen 个字节的数据
 *  如果实际读取的字节数与 maxlen 不相等也返回错误(ACL_VSTREAM_EOF)
 */
ACL_API int acl_vstream_readn(ACL_VSTREAM *fp, void *vptr, size_t maxlen);

/**
 * 将 fp 缓冲区内的数据拷贝到 vptr 中
 * @param fp {ACL_VSTREAM*} 数据流
 * @param vptr {void*} 用户的数据缓冲区指针地址
 * @param maxlen {size_t} vptr 数据缓冲区的空间大小
 * @return ret {int}, ret == ACL_VSTREAM_EOF: 表示出错, 应该关闭本地数据流,
 *  ret >= 0: 成功从 fp 数据流的缓冲区中读取了 ret 个字节的数据
 */
ACL_API int acl_vstream_bfcp_some(ACL_VSTREAM *fp, void *vptr, size_t maxlen);

/**
 * 从数据流中一次性读取 n 个数据, 该 n 有可能会小于用户所需要的 maxlen
 * @param fp {ACL_VSTREAM*} 数据流 
 * @param vptr {void*} 用户的数据缓冲区指针地址
 * @param maxlen {size_t} vptr 数据缓冲区的空间大小
 * @return ret {int}, ret == ACL_VSTREAM_EOF: 表示出错, 应该关闭本地数据流,
 *  ret > 0:  表示读到了 ret 个字节的数据
 *  注: 如果缓冲区内有数据, 则直接把缓冲区内的数据复制到用户的缓冲区然后直接返回;
 *     如果缓冲区内无数据, 则需要调用系统读操作(有可能会阻塞在系统读操作上), 该
 *     次调用返回后则把读到数据复制到用户缓冲区返回.
 *     在这两种情况下都不能保证读到的字节数等于所要求的字节数, 若想读到所要求的
 *     字节后才返回则请调用 vstream_loop_readn() 函数.
 */
ACL_API int acl_vstream_read(ACL_VSTREAM *fp, void *vptr, size_t maxlen);

/**
 * 一次性从 ACL_VSTREAM 流或系统缓存区中读取一行数据, 包括回车换行符
 * (调用者自行解决WINDOWS与UNIX对于回车换行的兼容性问题), 如果未读到
 * 回车换行符, 也将数据拷贝至用户的内存缓冲区.
 * @param fp {ACL_VSTREAM*} 数据流 
 * @param buf {ACL_VSTRING*} 数据缓冲区，当 buf->maxlen > 0 时，则限制每行数据
 *  的长度，即当 buf 中的数据长度达到 maxlen 时，即使没有读到完整一行数据，该
 *  函数也会返回，且会将 ready 置 1，调用者需调用 fp->flag 标志位中是否包含
 *  ACL_VSTREAM_FLAG_TAGYES 来判断是否读到一行数据
 * @param ready {int*} 是否按要求读到所需数据的标志位指针, 不能为空
 * @return ret {int}, ret == ACL_VSTREAM_EOF: 表示出错, 应该关闭本地数据流,
 *  ret >= 0: 成功从 fp 数据流的缓冲区中读取了 ret 个字节的数据
 */
ACL_API int acl_vstream_gets_peek(ACL_VSTREAM *fp, ACL_VSTRING *buf, int *ready);

/**
 * 一次性从 ACL_VSTREAM 流或系统缓存区中读取一行数据, 如果未读到回车换行符,
 * 也将数据拷贝至用户的内存缓冲区, 如果读到回车换行符便将回车换行符自动去掉,
 * 并将回车换行符前的数据拷贝至用户内存区.
 * @param fp {ACL_VSTREAM*} 数据流 
 * @param buf {ACL_VSTRING*} 数据缓冲区，当 buf->maxlen > 0 时，则限制每行数据
 *  的长度，即当 buf 中的数据长度达到 maxlen 时，即使没有读到完整一行数据，该
 *  函数也会返回，且会将 ready 置 1，调用者需调用 fp->flag 标志位中是否包含
 *  ACL_VSTREAM_FLAG_TAGYES 来判断是否读到一行数据
 * @param ready {int*} 是否按要求读到所需数据的标志位指针, 不能为空
 * @return ret {int}, ret == ACL_VSTREAM_EOF: 表示出错, 应该关闭本地数据流,
 *  ret >= 0: 成功从 fp 数据流的缓冲区中读取了 ret 个字节的数据, 如果仅
 *  读到了一个空行, 则 ret == 0.
 */
ACL_API int acl_vstream_gets_nonl_peek(ACL_VSTREAM *fp, ACL_VSTRING *buf, int *ready);

/**
 * 一次性从 ACL_VSTREAM 流或系统缓存区中读取固定长度的数据, 如果未读到所要求的
 * 数据, 也将数据拷贝至用户内存缓冲区, 如果读到所要求的数据, 则将 ready 标志位置位.
 * @param fp {ACL_VSTREAM*} 数据流 
 * @param buf {ACL_VSTRING*} 数据缓冲区
 * @param cnt {int} 所需要读的数据的长度
 * @param ready {int*} 是否按要求读到所需数据的标志位指针, 不能为空
 * @return ret {int}, ret == ACL_VSTREAM_EOF: 表示出错, 应该关闭本地数据流,
 *  ret >= 0: 成功从 fp 数据流的缓冲区中读取了 ret 个字节的数据, 
 *  (*ready) != 0: 表示读到了所要求长度的数据.
 */
ACL_API int acl_vstream_readn_peek(ACL_VSTREAM *fp, ACL_VSTRING *buf, int cnt, int *ready);

/**
 * 一次性从 ACL_VSTREAM 流或系统缓存区中读取不固定长度的数据,
 * 只要能读到大于 0 个字节的数据则将 ready 标志位置位
 * @param fp {ACL_VSTREAM*} 数据流 
 * @param buf {ACL_VSTRING*} 数据缓冲区
 * @return  ret {int}, ret == ACL_VSTREAM_EOF: 表示出错, 应该关闭本地数据流,
 *  ret >= 0: 成功从 fp 数据流的缓冲区中读取了 ret 个字节的数据.
 */
ACL_API int acl_vstream_read_peek(ACL_VSTREAM *fp, ACL_VSTRING *buf);

/**
 * 检查 ACL_VSTREAM 流是否可读或出错
 * @param fp {ACL_VSTREAM*} 数据流 
 * @return {int} 0: 表示无数据可读; ACL_VSTREAM_EOF 表示出错; > 0 表示有数据可读
 */
ACL_API int acl_vstream_can_read(ACL_VSTREAM *fp);

/**
 * 将文件流中的系统缓冲区及流缓冲区中的数据都直接同步至硬盘
 * @param fp {ACL_VSTREAM*} 文件流指针
 * @return {int} 0: ok; ACL_VSTREAM_EOF: error
 */
ACL_API int acl_vstream_fsync(ACL_VSTREAM *fp);

/**
 * 对于带缓冲方式的写，该函数保证缓冲区空间非空
 * @param fp {ACL_VSTREAM*} 数据流
 */
ACL_API void acl_vstream_buffed_space(ACL_VSTREAM *fp);

/**
 * 刷新写缓冲区里的数据
 * @param fp: socket 数据流
 * @return 刷新写缓冲区里的数据量或出错 ACL_VSTREAM_EOF
 */
ACL_API int acl_vstream_fflush(ACL_VSTREAM *fp);

/**
 * 带缓冲式写
 * @param fp {ACL_VSTREAM*} 数据流
 * @param vptr {const void*} 数据指针起始位置
 * @param dlen {size_t} 要写入的数据量
 * @return {int} 写入的数据量或出错 ACL_VSTREAM_EOF
 */
ACL_API int acl_vstream_buffed_writen(ACL_VSTREAM *fp, const void *vptr, size_t dlen);
#define	acl_vstream_buffed_fwrite	acl_vstream_buffed_writen

/**
 * 缓冲带格式的流输出, 类似于 vfprintf()
 * @param fp {ACL_VSTREAM*} 数据流 
 * @param fmt {const char*} 数据格式
 * @param ap {va_list}
 * @return ret {int}, ret == ACL_VSTREAM_EOF: 表示写出错, 应该关闭本地数据流,
 *  ret > 0:  表示成功写了 dlen 个字节的数据
 */
ACL_API int acl_vstream_buffed_vfprintf(ACL_VSTREAM *fp, const char *fmt, va_list ap);

/**
 * 缓冲带格式的流输出, 类似于 fprintf()
 * @param fp {ACL_VSTREAM*} 数据流 
 * @param fmt {const char*} 数据格式 
 * @param ... 变参序列
 * @return ret {int}, ret == ACL_VSTREAM_EOF: 表示写出错, 应该关闭本地数据流,
 *  ret > 0:  表示成功写了 dlen 个字节的数据
 */
ACL_API int ACL_PRINTF(2, 3) acl_vstream_buffed_fprintf(ACL_VSTREAM *fp,
	const char *fmt, ...);

/**
 * 向标准输出打印信息
 * @param fmt {const char*} 数据格式 
 * @param ... 变参序列
 * @return {int}, ACL_VSTREAM_EOF: 表示写出错, > 0:  表示成功写了 dlen 个字节的数据
 */
ACL_API int acl_vstream_buffed_printf(const char*, ...);

/**
 * 向流缓冲区中写入一行数据
 * @param s {const char*} 源字符串
 * @param fp {ACL_VSTREAM*} 数据流
 * @return {int} 0 成功; ACL_VSTREAM_EOF 失败
 */
ACL_API int acl_vstream_buffed_fputs(const char *s, ACL_VSTREAM *fp);

/**
 * 向标准输出流缓冲区中写入一行数据
 * @param s {const char*} 源字符串
 * @return {int} 0 成功; ACL_VSTREAM_EOF 失败
 */
ACL_API int acl_vstream_buffed_puts(const char *s);

/**
* 一次性写入流操作, 返回实际写入的字节数.
* @param fp {ACL_VSTREAM*} 数据流 
* @param vptr {const void*} 数据区指针地址
* @param dlen {int} 待写的数据区数据长度
* @return ret {int}, ret == ACL_VSTREAM_EOF: 表示写出错, 应该关闭本地数据流,
*  ret > 0:  表示成功写了 ret 个字节的数据
*/
ACL_API int acl_vstream_write(ACL_VSTREAM *fp, const void *vptr, int dlen);

/**
 * 一次性写入流操作，采用 writev 模式，返回实际写入的字节数
 * @param fp {ACL_VSTREAM*}
 * @param vector {const struct iovec*}
 * @param count {int} vector 数组的长度
 * @return {int} 返回成功写入的字节数，如果出错，则返回CL_VSTREAM_EOF
 */
ACL_API int acl_vstream_writev(ACL_VSTREAM *fp, const struct iovec *vector, int count);

/**
 * 采用 writev 模式往流中写，直至全部数据写完为止或出错
 * @param fp {ACL_VSTREAM*}
 * @param vector {const struct iovec*}
 * @param count {int} vector 数组的长度
 * @return {int} 返回成功写入的字节数，如果出错，则返回CL_VSTREAM_EOF
 */
ACL_API int acl_vstream_writevn(ACL_VSTREAM *fp, const struct iovec *vector, int count);

/**
 * 带格式的流输出, 类似于 vfprintf()
 * @param fp {ACL_VSTREAM*} 数据流 
 * @param fmt {const char*} 数据格式
 * @param ap {va_list}
 * @return ret {int}, ret == ACL_VSTREAM_EOF: 表示写出错, 应该关闭本地数据流,
 *  ret > 0:  表示成功写了 dlen 个字节的数据
 */
ACL_API int acl_vstream_vfprintf(ACL_VSTREAM *fp, const char *fmt, va_list ap);

/**
 * 带格式的流输出, 类似于 fprintf()
 * @param fp {ACL_VSTREAM*} 数据流 
 * @param fmt {const char*} 数据格式 
 * @param ... 变参序列
 * @return ret {int}, ret == ACL_VSTREAM_EOF: 表示写出错, 应该关闭本地数据流,
 *  ret > 0:  表示成功写了 dlen 个字节的数据
 */
ACL_API int ACL_PRINTF(2, 3) acl_vstream_fprintf(ACL_VSTREAM *fp,
	const char *fmt, ...);

/**
 * 向标准输出打印信息
 * @param fmt {const char*} 数据格式 
 * @param ... 变参序列
 * @return {int}, ACL_VSTREAM_EOF: 表示写出错, > 0:  表示成功写了 dlen 个字节的数据
 */
ACL_API int acl_vstream_printf(const char*, ...);

/**
 * 向流中写入一行数据
 * @param s {const char*} 源字符串
 * @param fp {ACL_VSTREAM*} 数据流
 * @return {int} 0 成功; ACL_VSTREAM_EOF 失败
 */
ACL_API int acl_vstream_fputs(const char *s, ACL_VSTREAM *fp);

/**
 * 向标准输出流中写入一行数据
 * @param s {const char*} 源字符串
 * @return {int} 0 成功; ACL_VSTREAM_EOF 失败
 */
ACL_API int acl_vstream_puts(const char *s);

/**
 * 循环向数据流中写 dlen 个字节的数据直至写完或出错为止
 * @param fp {ACL_VSTREAM*} 数据流 
 * @param vptr {const char*} 数据区指针地址
 * @param dlen {size_t} 待写的数据区数据长度
 * @return ret {int}, ret == ACL_VSTREAM_EOF: 表示写出错, 应该关闭本地数据流,
 *  ret > 0:  表示成功写了 dlen 个字节的数据
 */
ACL_API int acl_vstream_writen(ACL_VSTREAM *fp, const void *vptr, size_t dlen);
#define	acl_vstream_fwrite	acl_vstream_writen

/**
 * 释放一个数据流的内存空间, 但并不关闭 socket 描述符
 * @param fp {ACL_VSTREAM*} 数据流
 */
ACL_API void acl_vstream_free(ACL_VSTREAM *fp);

/**
 * 释放一个数据流的内存空间并关闭其所携带的 socket 描述符
 * @param fp {ACL_VSTREAM*} 数据流
 */
ACL_API int acl_vstream_close(ACL_VSTREAM *fp);
#define	acl_vstream_fclose	acl_vstream_close

/**
 * 调用数据流中的所有关闭回调函数同时清除这些回调函数
 * @param fp {ACL_VSTREAM*} 数据流
 */
ACL_API void acl_vstream_call_close_handles(ACL_VSTREAM *fp);

/**
 * 注册一个关闭函数
 * @param fp {ACL_VSTREAM*} 数据流
 * @param close_fn {void (*)(ACL_VSTREAM*, void*)} 关闭函数指针
 * @param context {void*} close_fn 所需要的参数
 */
ACL_API void acl_vstream_add_close_handle(ACL_VSTREAM *fp,
		void (*close_fn)(ACL_VSTREAM*, void*), void *context);

/**
 * 删除一个关闭句柄.
 * @param fp {ACL_VSTREAM*} 数据流
 * @param close_fn {void (*)(ACL_VSTREAM*, void*)} 关闭函数指针
 * @param context {void*} close_fn 所需要的参数
 */
ACL_API void acl_vstream_delete_close_handle(ACL_VSTREAM *fp,
		void (*close_fn)(ACL_VSTREAM*, void*), void *context);
/**
 * 清除一个数据流中所有的关闭句柄
 * @param fp {ACL_VSTREAM*} 数据流
 */
ACL_API void acl_vstream_clean_close_handle(ACL_VSTREAM *fp);

/**
 * 重新复位数据流的内部数据指针及计数值
 * @param fp {ACL_VSTREAM*} 数据流
 */
ACL_API void acl_vstream_reset(ACL_VSTREAM *fp);

/**
 * 取得当前数据流的错误状态
 * @param fp {ACL_VSTREAM*} 数据流
 * @return {const char*} 错误描述
 */
ACL_API const char *acl_vstream_strerror(ACL_VSTREAM *fp);

/*-----------------------  以下为常用的宏函数 ------------------------------*/
/**
 * 从fp 流中读取一个字节的宏实现，效率要比 acl_vstream_getc()/1 高
 * @param fp {ACL_VSTREAM*} 数据流指针
 * @return {int} ACL_VSTREAM_EOF(出错) 或所读到的某个字节的ASCII,
 *  若为 ACL_VSTREAM_EOF: 读出错或对方关闭了连接, 应该关闭该数据流
 */
#define ACL_VSTREAM_GETC(stream_ptr) (                        \
    (stream_ptr)->read_cnt > 0 ?                              \
        (stream_ptr)->read_cnt--,                             \
        (stream_ptr)->offset++,                               \
        *(stream_ptr)->read_ptr++:                            \
        (stream_ptr)->sys_getc((stream_ptr)))

/**
 * 向 fp 流中写一个字节的宏实现
 * @param fp {ACL_VSTREAM*} 数据流指针
 * @return {int} ACL_VSTREAM_EOF(出错) 或所写入字节的 ASCII
 */
#define ACL_VSTREAM_PUTC(ch, stream_ptr) (                                   \
  (stream_ptr)->wbuf_size == 0 ?                                             \
    (acl_vstream_buffed_space((stream_ptr)),                                 \
        ((stream_ptr)->wbuf[(size_t) (stream_ptr)->wbuf_dlen++] = (ch)))     \
    : ((stream_ptr)->wbuf_dlen == stream_ptr->wbuf_size ?                    \
        (acl_vstream_fflush((stream_ptr)) == ACL_VSTREAM_EOF ?               \
          ACL_VSTREAM_EOF                                                    \
          : ((stream_ptr)->wbuf[(size_t) (stream_ptr)->wbuf_dlen++] = (ch))) \
        : ((stream_ptr)->wbuf[(size_t) (stream_ptr)->wbuf_dlen++] = (ch))))

/**
 * 由流获得套接字
 * @param stream_ptr {ACL_VSTREAM*}
 */
#define ACL_VSTREAM_SOCK(stream_ptr) ((stream_ptr)->fd.sock)

/**
 * 由流获得文件句柄
 * @param stream_ptr {ACL_VSTREAM*}
 */
#define ACL_VSTREAM_FILE(stream_ptr) ((stream_ptr)->fd.h_file)

/**
 * 获得文件流句柄的文件路径名
 * @param stream_ptr {ACL_VSTREAM*}
 */
#define	ACL_VSTREAM_PATH(stream_ptr) ((stream_ptr)->path)

/**
 * 当 ACL_VSTREAM 为文件流时，设置文件流的路径
 * @param fp {ACL_VSTREAM*} 文件流
 * @param path {const char*} 文件路径
 */
ACL_API void acl_vstream_set_path(ACL_VSTREAM *fp, const char *path);

/**
 * 当 ACL_VSTREAM 为网络流时，用此宏取得对方的地址
 */
#define	ACL_VSTREAM_PEER(stream_ptr) ((stream_ptr)->addr_peer)

/**
 * 当 ACL_VSTREAM 为网络流时，此函数设置远程连接地址
 * @param fp {ACL_VSTREAM*} 网络流，非空
 * @param addr {const char*} 远程连接地址，非空
 */
ACL_API void acl_vstream_set_peer(ACL_VSTREAM *fp, const char *addr);

/**
 * 当 ACL_VSTREAM 为网络流时，此函数设置远程连接地址
 * @param fp {ACL_VSTREAM*} 网络流，非空
 * @param sa {const struct sockaddr_in *} 远程连接地址，非空
 */
ACL_API void acl_vstream_set_peer_addr(ACL_VSTREAM *fp,
	const struct sockaddr_in *sa);

/**
 * 当 ACL_VSTREAM 为网络流时，用此宏取得本地的地址
 */
#define	ACL_VSTREAM_LOCAL(stream_ptr) ((stream_ptr)->addr_local)

/**
 * 当 ACL_VSTREAM 为网络流时，此函数设置本地地址
 * @param fp {ACL_VSTREAM*} 网络流，非空
 * @param addr {const char*} 本地地址，非空
 */
ACL_API void acl_vstream_set_local(ACL_VSTREAM *fp, const char *addr);

/**
 * 当 ACL_VSTREAM 为网络流时，此函数设置本地地址
 * @param fp {ACL_VSTREAM*} 网络流，非空
 * @param sa {const sockaddr_in*} 本地地址，非空
 */
ACL_API void acl_vstream_set_local_addr(ACL_VSTREAM *fp,
	const struct sockaddr_in *sa);

ACL_API int acl_vstream_add_object(ACL_VSTREAM *fp, const char *key, void *obj);
ACL_API int acl_vstream_del_object(ACL_VSTREAM *fp, const char *key);
ACL_API void *acl_vstream_get_object(ACL_VSTREAM *fp, const char *key);

ACL_API void acl_socket_read_hook(ACL_VSTREAM_RD_FN read_fn);
ACL_API void acl_socket_write_hook(ACL_VSTREAM_WR_FN write_fn);
ACL_API void acl_socket_writev_hook(ACL_VSTREAM_WV_FN writev_fn);
ACL_API void acl_socket_close_hook(int (*close_fn)(ACL_SOCKET));

/**
 * 设定流的读/写套接字
 * @param stream_ptr {ACL_VSTREAM*}
 * @param _fd {ACL_SOCKET} 套接字
 */
#define	ACL_VSTREAM_SET_SOCK(stream_ptr, _fd) do {            \
        ACL_VSTREAM *__stream_ptr = stream_ptr;               \
        __stream_ptr->fd.sock   = _fd;                        \
} while (0)

/**
 * 设置流中的文件句柄
 * @param stream_ptr {ACL_VSTREAM*}
 * @param _fh {ACL_FILE_HANDLE}
 */
#define	ACL_VSTREAM_SET_FILE(stream_ptr, _fh) do {            \
        ACL_VSTREAM *__stream_ptr = stream_ptr;               \
        __stream_ptr->fd.h_file = _fh;                        \
} while (0)

/* 一些比较快速的宏的运算模式 */

/**
 * 流中在读缓冲区中的数据量大小
 * @param stream_ptr {ACL_VSTREAM*) 类型的指针
 * @return -1: 表示出错, >= 0 此值即为流读缓冲区中的数据量大小
 */
#define	ACL_VSTREAM_BFRD_CNT(stream_ptr)                      \
	((stream_ptr) == NULL ? -1 : (stream_ptr)->read_cnt)

/**
 * 设定流的读写超时值
 * @param stream_ptr {ACL_VSTREAM*) 类型的指针
 * @param _rw_timeo {int} 超时值大小(以秒为单位)
 */
#define	ACL_VSTREAM_SET_RWTIMO(stream_ptr, _rw_timeo) do {    \
        ACL_VSTREAM *__stream_ptr  = stream_ptr;              \
        __stream_ptr->rw_timeout = _rw_timeo;                 \
} while (0)

/**
 * 将流置为结束状态
 * @param stream_ptr {ACL_VSTREAM*) 类型的指针
 */
#define	ACL_VSTREAM_SET_EOF(stream_ptr) do {                  \
        ACL_VSTREAM *__stream_ptr = stream_ptr;               \
        __stream_ptr->flag |= ACL_VSTREAM_FLAG_EOF;           \
} while (0)

/**
 * 判断数据流是否出了错
 * @param stream_ptr: ACL_VSTREAM 类型的指针
 * @return 0表示正常, 非0表示出错
 */
#define ACL_IF_VSTREAM_ERR(stream_ptr)                        \
	((stream_ptr)->flag & ACL_VSTREAM_FLAG_BAD)

#ifdef  __cplusplus
}
#endif

/**
 * 从数据流中取出与该流读写有关的系统错误号
 * @param stream_ptr {ACL_VSTREAM*) 类型的指针
 * @return err {int} 整形错误号，调用者可以用 strerror(err) 的方式查看具体含义
 */
#define	ACL_VSTREAM_ERRNO(stream_ptr) ((stream_ptr)->errnum)

/**
 * 判断一个流是否超时
 * @param stream_ptr {ACL_VSTREAM*) 类型的指针
 * @return {int} 0: 否; != 0: 是
 */
#define	acl_vstream_ftimeout(stream_ptr) \
        ((stream_ptr)->flag & ACL_VSTREAM_FLAG_TIMEOUT)

#endif

