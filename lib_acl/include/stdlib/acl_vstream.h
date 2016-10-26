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

/* ���رջ��ͷ�һ��������ʱ, ��Ҫ�ص�һЩ�ͷź���, �˽�������˸ûص�
 * �����ľ������ ---add by zsx, 2006.6.20
 */
typedef struct ACL_VSTREAM_CLOSE_HANDLE {
	void (*close_fn)(ACL_VSTREAM*, void*);
	void *context;
} ACL_VSTREAM_CLOSE_HANDLE;

/* ���ݶ�д�����Ͷ��� */
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
#define	ACL_VSTREAM_FLAG_DEFER_FREE	(1 << 4)	/**< �ӳٹر� */

#define	ACL_VSTREAM_FLAG_ERR            (1 << 10)	/**< �������� */
#define	ACL_VSTREAM_FLAG_EOF            (1 << 11)	/**< ���� */
#define	ACL_VSTREAM_FLAG_TIMEOUT        (1 << 12)	/**< ��ʱ */
#define	ACL_VSTREAM_FLAG_RDSHORT        (1 << 13)	/**< ���Ĳ��� */
#define ACL_VSTREAM_FLAG_BAD  (ACL_VSTREAM_FLAG_ERR \
                               | ACL_VSTREAM_FLAG_EOF \
                               | ACL_VSTREAM_FLAG_TIMEOUT)
#define	ACL_VSTREAM_FLAG_CLIENT         (1 << 14)
#define	ACL_VSTREAM_FLAG_CONNECT        (1 << 15)
#define	ACL_VSTREAM_FLAG_SOCKPAIR       (1 << 16)

#define	ACL_VSTREAM_FLAG_TAGYES	        (1 << 17) /* ������Ҫ��ı�־λ����λ */

#define	ACL_VSTREAM_FLAG_CONNECTING     (1 << 18) /* �������ӹ����� */
#define	ACL_VSTREAM_FLAG_PREREAD	(1 << 19) /* ���� acl_vstream_can_read ���ù����Ƿ�����Ԥ�� */

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

	unsigned int omode;             /**< open mode, such as: 0600, 0777 */

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
#define ACL_VSTREAM_IN          (&acl_vstream_fstd[0]) /**< ��׼���� */
#define ACL_VSTREAM_OUT         (&acl_vstream_fstd[1]) /**< ��׼��� */
#define ACL_VSTREAM_ERR         (&acl_vstream_fstd[2]) /**< ��׼������� */

/*--------------------------------------------------------------------------*/
/**
 * ��ʼ��ACL_VSTREAM���ĺ�����
 * ����_WIN32��˵�������Ҫ�ñ�׼�������������Ҫ���ô˺������г�ʼ��
 */
ACL_API void acl_vstream_init(void);

/**
 * ����: ̽�������ж�������, �����������е�������ϵͳ������������
 * @param fp {ACL_VSTREAM*} ��ָ��, ����Ϊ��
 * @return ret {int}, ret > 0 OK; ret <= 0 Error
 * ע: ����Ӧ�������׽���
 */
ACL_API int acl_vstream_peekfd(ACL_VSTREAM *fp);

/**
 * ��¡һ��ACL_VSTREAM������ioctl_read_ctx, ioctl_write_ctx, fdp
 * ���������ݶ�����������Ƕ�̬�ڴ����ݣ����µ��������ڲ���̬����
 * �ڴ��ҽ�Դ���ݽ��п���
 * @param stream_src {ACL_VSTREAM*} Դ��ָ��
 * @return {ACL_VSTREAM*} Ŀ����ָ��
 */
ACL_API ACL_VSTREAM *acl_vstream_clone(const ACL_VSTREAM *stream_src);

/**
 * ���������������ͣ��ú������������������趨���ڸ��������ϵĶ���д���رպ���
 * @param fp {ACL_VSTREAM*} ��ָ��, ����Ϊ��
 * @param type {int} �����������ͣ�defined above: ACL_VSTREAM_TYPE_XXX
 * @return ret {int}, ret >= 0 OK; ret < 0 Error
 */
ACL_API int acl_vstream_set_fdtype(ACL_VSTREAM *fp, int type);

/**
 * ����һ�ļ��������Ӧ��������
 * @param fh {ACL_FILE_HANDLE} �ļ����
 * @param oflags {unsigned int} ��־λ, We're assuming that O_RDONLY: 0x0000,
 *  O_WRONLY: 0x0001, O_RDWR: 0x0002, O_APPEND: 0x0008, O_CREAT: 0x0100,
 *  O_TRUNC: 0x0200, O_EXCL: 0x0400; just for win32, O_TEXT: 0x4000,
 *  O_BINARY: 0x8000, O_RAW: O_BINARY, O_SEQUENTIAL: 0x0020, O_RANDOM: 0x0010.
 *  ͬʱ����
 * @return {ACL_VSTREAM*} ���������
 */
ACL_API ACL_VSTREAM *acl_vstream_fhopen(ACL_FILE_HANDLE fh, unsigned int oflags);

/**
 * ����һ��������
 * @param fd {ACL_SOCKET} ������(����Ϊ����������Ҳ����Ϊ�ļ�������)
 * @param oflags {unsigned int} ��־λ, We're assuming that O_RDONLY: 0x0000,
 *  O_WRONLY: 0x0001, O_RDWR: 0x0002, O_APPEND: 0x0008, O_CREAT: 0x0100,
 *  O_TRUNC: 0x0200, O_EXCL: 0x0400; just for win32, O_TEXT: 0x4000,
 *  O_BINARY: 0x8000, O_RAW: O_BINARY, O_SEQUENTIAL: 0x0020, O_RANDOM: 0x0010.
 * @param buflen {size_t} ���û������Ĵ�С
 * @param rw_timeo {int} ��д��ʱʱ��(����Ϊ��λ)
 * @param fdtype {int} ACL_VSTREAM_TYPE_FILE, ACL_VSTREAM_TYPE_SOCK,
 *  ACL_VSTREAM_TYPE_LISTEN | ACL_VSTREAM_TYPE_LISTEN_INET | ACL_VSTREAM_TYPE_LISTEN_UNIX
 * @return ret {ACL_VSTREAM*}, ret == NULL: ����, ret != NULL: OK
 */
ACL_API ACL_VSTREAM *acl_vstream_fdopen(ACL_SOCKET fd, unsigned int oflags,
		size_t buflen, int rw_timeo, int fdtype);

/**
 * ��һ���ļ���������
 * @param path {const char*} �ļ���
 * @param oflags {unsigned int} ��־λ, We're assuming that O_RDONLY: 0x0000,
 *  O_WRONLY: 0x0001, O_RDWR: 0x0002, O_APPEND: 0x0008, O_CREAT: 0x0100,
 *  O_TRUNC: 0x0200, O_EXCL: 0x0400; just for win32, O_TEXT: 0x4000,
 *  O_BINARY: 0x8000, O_RAW: O_BINARY, O_SEQUENTIAL: 0x0020, O_RANDOM: 0x0010.
 * @param mode {int} ���ļ����ʱ��ģʽ(��: 0600)
 * @param buflen {size_t} ���û������Ĵ�С
 * @return ret {ACL_VSTREAM*}, ret== NULL: ����, ret != NULL: OK
 */
ACL_API ACL_VSTREAM *acl_vstream_fopen(const char *path, unsigned int oflags,
		int mode, size_t buflen);

/**
 * ��ȡ�����ļ��������ڴ���
 * @param path {const char*} �ļ���, ��: /opt/acl/conf/service/test.cf
 * @return {char*} �����ļ�ȫ�����ݵĻ�����, ������û���Ҫ���� acl_myfree
 *  �ͷŸ��ڴ���
 */
ACL_API char *acl_vstream_loadfile(const char *path);

/**
 * ��ȡ�����ļ��������ڴ���
 * @param path {const char*} �ļ���, ��: /opt/acl/conf/service/test.cf
 * @param size {ssize_t*} ����ǿգ����ֵ�洢���صĻ�������С�������ȡ����
 *  �������ֵ�ᱻ�� -1
 * @return {char*} �����ļ�ȫ�����ݵĻ�����, ������û���Ҫ���� acl_myfree
 *  �ͷŸ��ڴ���
 */
ACL_API char *acl_vstream_loadfile2(const char *path, ssize_t *size);

/**
 * �������ĸ�������
 * @param fp {ACL_VSTREAM*} ��ָ��
 * @param name {int} �����õĲ��������еĵ�һ������������,
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
 * ��λ�ļ�ָ��
 * @param fp {ACL_VSTREAM*} ������ָ��
 * @param offset {acl_off_t} ƫ����
 * @param whence {int} ƫ�Ʒ���, SEEK_SET, SEEK_CUR, SEEK_END
 * @return ret {acl_off_t}, ret >= 0: ��ȷ, ret < 0: ����
 * ע�� acl_vstream_fseek() Ч�ʸ���Щ, ���������˻������Ĺ���,
 *      �ұ� acl_vstream_fseek2() �ٵ���һ�� lseek() ϵͳ����.
 */
ACL_API acl_off_t acl_vstream_fseek(ACL_VSTREAM *fp, acl_off_t offset, int whence);

/**
 * ��λ�ļ�ָ��
 * @param fp {ACL_VSTREAM*} ������ָ��
 * @param offset {acl_off_t} ƫ����
 * @param whence {int} �ƶ�����SEEK_SET�����ļ���ʼλ�ú��ƶ���,
 *  SEEK_CUR���ӵ�ǰ�ļ�ָ��λ������ƶ���, SEEK_END�����ļ�β��ǰ�ƶ���
 * @return ret {acl_off_t}, ret >= 0: ��ȷ, ret < 0: ����
 * @deprecated �ú�����Ч�ʽϵ�
 */
ACL_API acl_off_t acl_vstream_fseek2(ACL_VSTREAM *fp, acl_off_t offset, int whence);

/**
 * ���ص�ǰ�ļ�ָ������λ��
 * @param fp {ACL_VSTREAM*} ������ָ��
 * @return {acl_off_t} ��ǰ�ļ�ָ������λ��, -1 ��ʾ����
 */
ACL_API acl_off_t acl_vstream_ftell(ACL_VSTREAM *fp);

/**
 * ��Դ�ļ����̽ض�
 * @param fp {ACL_VSTREAM*} ������ָ��
 * @param length {acl_off_t} ���ݳ���(>=0)
 * @return {int} 0: ok, -1: error
 */
ACL_API int acl_file_ftruncate(ACL_VSTREAM *fp, acl_off_t length);

/**
 * ��Դ�ļ����̽ض�
 * @param path {const char*} �ļ���(������ȫ·�������·��)
 * @param length {acl_off_t} ���ݳ���(>=0)
 * @return {int} 0: ok, -1: error
 */
ACL_API int acl_file_truncate(const char *path, acl_off_t length);

/**
 * �鿴һ���ļ������������
 * @param fp {ACL_VSTREAM *} �ļ������
 * @param buf {acl_stat *} �洢����Ľṹ��ַ
 * @return {int} 0: ok; -1: error
 */
ACL_API int acl_vstream_fstat(ACL_VSTREAM *fp, struct acl_stat *buf);

/**
 * �鿴һ���ļ��Ĵ�С
 * @param fp {ACL_VSTREAM *} �ļ������
 * @return {int} >= 0: ok;  -1: error
 */
ACL_API acl_int64 acl_vstream_fsize(ACL_VSTREAM *fp);

/**
 * ��fp ���ж�ȡһ���ֽ�
 * @param fp {ACL_VSTREAM*} ������ָ��
 * @return {int} ACL_VSTREAM_EOF(����) ����������ĳ���ֽڵ�ASCII
 *  ��Ϊ ACL_VSTREAM_EOF: �������Է��ر�������, Ӧ�ùرո�������
 */
ACL_API int acl_vstream_getc(ACL_VSTREAM *fp);
#define	acl_vstream_get_char	acl_vstream_getc

/**
 * �� fp ���з�������һ��������ȡ size ���ֽ�
 * @param fp {ACL_VSTREAM*} ������ָ��
 * @param buf {char*} �û��������ڴ滺����
 * @param size {int} buf �������Ŀռ��С
 * @return {int} ����ȡ���ֽ��� n, ��� n == ACL_VSTREAM_EOF ��������, ����
 *         n >= 0 ��ȷ.
 */
ACL_API int acl_vstream_nonb_readn(ACL_VSTREAM *fp, char *buf, int size);

/**
 * �ж�һ���������������Ƿ��Ѿ���ϵͳ�ر��ˣ���������������û������ʱ��
 * �ú��������ϵͳ�Ķ�����������һ���ֽڣ����ж��Ƿ�socket������Ѿ�
 * �رգ���ɹ���ȡһ���ֽڣ���˵��socket������ͬʱ�����������ݷŻػ���
 * ��, ���������ACL_VSTREAM_EOF, ����Ҫ�жϴ�����Ƿ񱻹ر�
 * @param fp {ACL_VSTREAM*} ������ָ��
 * @return {int}, 0 ˵����socket����; -1 ��socket������Ѿ���ϵͳ�ر�
 */
ACL_API int acl_vstream_probe_status(ACL_VSTREAM *fp);

/**
 * ��һ���ַ��Ż���������
 * @param fp {ACL_VSTREAM*} ������ָ��
 * @param ch {int} �ַ��� ASCII �� 
 * @return {int} �ַ��� ASCII ��, �ú���Ӧ�������, �����ڲ��ڴ����ʧ�ܶ�����
 *  core �ļ�.
 */
ACL_API int acl_vstream_ungetc(ACL_VSTREAM *fp, int ch);

/**
 * ��ָ�����ȵ����ݷŻ�����������
 * @param fp {ACL_VSTREAM*} ������ָ��
 * @param ptr {const void *} ��Ҫ�Ż������е����ݵ���ʼ��ַ
 * @param length {size_t} ��Ҫ�Ż������е����ݵĳ���
 * @return {int} ���ɹ��Ż������е����ݳ���, Ӧ�����������, �����ڲ��ڴ����
 *  ʧ�ܶ��Զ����� core �ļ�!
 */
ACL_API int acl_vstream_unread(ACL_VSTREAM *fp, const void *ptr, size_t length);

/**
 * ���������ж�ȡһ������, ֱ������  "\n" �������Ϊֹ, ��������°��� "\n"
 * @param fp {ACL_VSTREAM*} ������
 * @param vptr {void*} �û��������ڴ滺����ָ��
 * @param maxlen {size_t} vptr �������Ĵ�С
 * @return  ret {int}, ret == ACL_VSTREAM_EOF:  �������Է��ر�������, 
 *  Ӧ�ùرձ���������; n > 0:  ���� �� n ���ֽڵ�����, ����� n ������
 *  �����һ���� 0 �ַ�Ϊ "\n" ����������һ����������, ������������� n
 *  �����ݵ��Է�δ���� "\n" �͹ر�������; ������ͨ�����
 *  (fp->flag & ACL_VSTREAM_FLAG_TAGYES)
 *	������ 0 ���ж��Ƿ������ "\n", ����� 0 ���ʾ������ "\n".
 */
ACL_API int acl_vstream_gets(ACL_VSTREAM *fp, void *vptr, size_t maxlen);
#define	acl_vstream_readline	acl_vstream_gets
#define	acl_vstream_fgets	acl_vstream_gets

/**
 * ���������ж�ȡһ������, ֱ������ "\n" �������Ϊֹ, ���صĽ���в����� "\n"
 * @param fp {ACL_VSTREAM*} ������ 
 * @param vptr {void*} �û��������ڴ滺����ָ��
 * @param maxlen {size_t} vptr �������Ĵ�С
 * @return ret {int}, ret == ACL_VSTREAM_EOF:  �������Է��ر�������,
 *  Ӧ�ùرձ���������, n == 0: ������һ������, ���������ݽ��� "\r\n",
 *  n > 0:  ���� �� n ���ֽڵ�����.
 */
ACL_API int acl_vstream_gets_nonl(ACL_VSTREAM *fp, void *vptr, size_t maxlen);

/**
 * ���������л�����ַ���Ϊ��־����λ������
 * @param fp {ACL_VSTREAM*} ����ָ��
 * @param vptr {void*} ���ݴ洢������
 * @param maxlen {size_t} vptr ��������С
 * @param tag {const char*} �ַ�����־
 * @param taglen {size_t} tag �����ݵĳ��ȴ�С
 * @return ret {int}, ret == ACL_VSTREAM_EOF:  �������Է��ر�������, 
 *  Ӧ�ùرձ���������, n > 0:  ���� �� n ���ֽڵ�����, �������������Ҫ��
 *  ��־��, �� fp ���� (fp->flag & ACL_VSTREAM_FLAG_TAGYES) ������ 0.
 */
ACL_API int acl_vstream_readtags(ACL_VSTREAM *fp, void *vptr, size_t maxlen,
		const char *tag, size_t taglen);

/**
 * ѭ����ȡ maxlen ������, ֱ������ maxlen ���ֽ�Ϊֹ�������
 * @param fp {ACL_VSTREAM*} ������
 * @param vptr {void*} �û������ݻ�����ָ���ַ
 * @param maxlen {size_t} vptr ���ݻ������Ŀռ��С
 * @return ret {int}, ret == ACL_VSTREAM_EOF:  �������Է��ر�������, Ӧ��
 *  �رձ��������� n > 0:  �ɹ���ȡ�� maxlen ���ֽڵ�����
 *  ���ʵ�ʶ�ȡ���ֽ����� maxlen �����Ҳ���ش���(ACL_VSTREAM_EOF)
 */
ACL_API int acl_vstream_readn(ACL_VSTREAM *fp, void *vptr, size_t maxlen);

/**
 * �� fp �������ڵ����ݿ����� vptr ��
 * @param fp {ACL_VSTREAM*} ������
 * @param vptr {void*} �û������ݻ�����ָ���ַ
 * @param maxlen {size_t} vptr ���ݻ������Ŀռ��С
 * @return ret {int}, ret == ACL_VSTREAM_EOF: ��ʾ����, Ӧ�ùرձ���������,
 *  ret >= 0: �ɹ��� fp �������Ļ������ж�ȡ�� ret ���ֽڵ�����
 */
ACL_API int acl_vstream_bfcp_some(ACL_VSTREAM *fp, void *vptr, size_t maxlen);

/**
 * ����������һ���Զ�ȡ n ������, �� n �п��ܻ�С���û�����Ҫ�� maxlen
 * @param fp {ACL_VSTREAM*} ������ 
 * @param vptr {void*} �û������ݻ�����ָ���ַ
 * @param maxlen {size_t} vptr ���ݻ������Ŀռ��С
 * @return ret {int}, ret == ACL_VSTREAM_EOF: ��ʾ����, Ӧ�ùرձ���������,
 *  ret > 0:  ��ʾ������ ret ���ֽڵ�����
 *  ע: �����������������, ��ֱ�Ӱѻ������ڵ����ݸ��Ƶ��û��Ļ�����Ȼ��ֱ�ӷ���;
 *     �����������������, ����Ҫ����ϵͳ������(�п��ܻ�������ϵͳ��������), ��
 *     �ε��÷��غ���Ѷ������ݸ��Ƶ��û�����������.
 *     ������������¶����ܱ�֤�������ֽ���������Ҫ����ֽ���, ���������Ҫ���
 *     �ֽں�ŷ���������� vstream_loop_readn() ����.
 */
ACL_API int acl_vstream_read(ACL_VSTREAM *fp, void *vptr, size_t maxlen);

/**
 * һ���Դ� ACL_VSTREAM ����ϵͳ�������ж�ȡһ������, �����س����з�
 * (���������н��WINDOWS��UNIX���ڻس����еļ���������), ���δ����
 * �س����з�, Ҳ�����ݿ������û����ڴ滺����.
 * @param fp {ACL_VSTREAM*} ������ 
 * @param buf {ACL_VSTRING*} ���ݻ��������� buf->maxlen > 0 ʱ��������ÿ������
 *  �ĳ��ȣ����� buf �е����ݳ��ȴﵽ maxlen ʱ����ʹû�ж�������һ�����ݣ���
 *  ����Ҳ�᷵�أ��һὫ ready �� 1������������� fp->flag ��־λ���Ƿ����
 *  ACL_VSTREAM_FLAG_TAGYES ���ж��Ƿ����һ������
 * @param ready {int*} �Ƿ�Ҫ������������ݵı�־λָ��, ����Ϊ��
 * @return ret {int}, ret == ACL_VSTREAM_EOF: ��ʾ����, Ӧ�ùرձ���������,
 *  ret >= 0: �ɹ��� fp �������Ļ������ж�ȡ�� ret ���ֽڵ�����
 */
ACL_API int acl_vstream_gets_peek(ACL_VSTREAM *fp, ACL_VSTRING *buf, int *ready);

/**
 * һ���Դ� ACL_VSTREAM ����ϵͳ�������ж�ȡһ������, ���δ�����س����з�,
 * Ҳ�����ݿ������û����ڴ滺����, ��������س����з��㽫�س����з��Զ�ȥ��,
 * �����س����з�ǰ�����ݿ������û��ڴ���.
 * @param fp {ACL_VSTREAM*} ������ 
 * @param buf {ACL_VSTRING*} ���ݻ��������� buf->maxlen > 0 ʱ��������ÿ������
 *  �ĳ��ȣ����� buf �е����ݳ��ȴﵽ maxlen ʱ����ʹû�ж�������һ�����ݣ���
 *  ����Ҳ�᷵�أ��һὫ ready �� 1������������� fp->flag ��־λ���Ƿ����
 *  ACL_VSTREAM_FLAG_TAGYES ���ж��Ƿ����һ������
 * @param ready {int*} �Ƿ�Ҫ������������ݵı�־λָ��, ����Ϊ��
 * @return ret {int}, ret == ACL_VSTREAM_EOF: ��ʾ����, Ӧ�ùرձ���������,
 *  ret >= 0: �ɹ��� fp �������Ļ������ж�ȡ�� ret ���ֽڵ�����, �����
 *  ������һ������, �� ret == 0.
 */
ACL_API int acl_vstream_gets_nonl_peek(ACL_VSTREAM *fp, ACL_VSTRING *buf, int *ready);

/**
 * һ���Դ� ACL_VSTREAM ����ϵͳ�������ж�ȡ�̶����ȵ�����, ���δ������Ҫ���
 * ����, Ҳ�����ݿ������û��ڴ滺����, ���������Ҫ�������, �� ready ��־λ��λ.
 * @param fp {ACL_VSTREAM*} ������ 
 * @param buf {ACL_VSTRING*} ���ݻ�����
 * @param cnt {int} ����Ҫ�������ݵĳ���
 * @param ready {int*} �Ƿ�Ҫ������������ݵı�־λָ��, ����Ϊ��
 * @return ret {int}, ret == ACL_VSTREAM_EOF: ��ʾ����, Ӧ�ùرձ���������,
 *  ret >= 0: �ɹ��� fp �������Ļ������ж�ȡ�� ret ���ֽڵ�����, 
 *  (*ready) != 0: ��ʾ��������Ҫ�󳤶ȵ�����.
 */
ACL_API int acl_vstream_readn_peek(ACL_VSTREAM *fp, ACL_VSTRING *buf, int cnt, int *ready);

/**
 * һ���Դ� ACL_VSTREAM ����ϵͳ�������ж�ȡ���̶����ȵ�����,
 * ֻҪ�ܶ������� 0 ���ֽڵ������� ready ��־λ��λ
 * @param fp {ACL_VSTREAM*} ������ 
 * @param buf {ACL_VSTRING*} ���ݻ�����
 * @return  ret {int}, ret == ACL_VSTREAM_EOF: ��ʾ����, Ӧ�ùرձ���������,
 *  ret >= 0: �ɹ��� fp �������Ļ������ж�ȡ�� ret ���ֽڵ�����.
 */
ACL_API int acl_vstream_read_peek(ACL_VSTREAM *fp, ACL_VSTRING *buf);

/**
 * ��� ACL_VSTREAM ���Ƿ�ɶ������
 * @param fp {ACL_VSTREAM*} ������ 
 * @return {int} 0: ��ʾ�����ݿɶ�; ACL_VSTREAM_EOF ��ʾ����; > 0 ��ʾ�����ݿɶ�
 */
ACL_API int acl_vstream_can_read(ACL_VSTREAM *fp);

/**
 * ���ļ����е�ϵͳ�����������������е����ݶ�ֱ��ͬ����Ӳ��
 * @param fp {ACL_VSTREAM*} �ļ���ָ��
 * @return {int} 0: ok; ACL_VSTREAM_EOF: error
 */
ACL_API int acl_vstream_fsync(ACL_VSTREAM *fp);

/**
 * ���ڴ����巽ʽ��д���ú�����֤�������ռ�ǿ�
 * @param fp {ACL_VSTREAM*} ������
 */
ACL_API void acl_vstream_buffed_space(ACL_VSTREAM *fp);

/**
 * ˢ��д�������������
 * @param fp: socket ������
 * @return ˢ��д�������������������� ACL_VSTREAM_EOF
 */
ACL_API int acl_vstream_fflush(ACL_VSTREAM *fp);

/**
 * ������ʽд
 * @param fp {ACL_VSTREAM*} ������
 * @param vptr {const void*} ����ָ����ʼλ��
 * @param dlen {size_t} Ҫд���������
 * @return {int} д�������������� ACL_VSTREAM_EOF
 */
ACL_API int acl_vstream_buffed_writen(ACL_VSTREAM *fp, const void *vptr, size_t dlen);
#define	acl_vstream_buffed_fwrite	acl_vstream_buffed_writen

/**
 * �������ʽ�������, ������ vfprintf()
 * @param fp {ACL_VSTREAM*} ������ 
 * @param fmt {const char*} ���ݸ�ʽ
 * @param ap {va_list}
 * @return ret {int}, ret == ACL_VSTREAM_EOF: ��ʾд����, Ӧ�ùرձ���������,
 *  ret > 0:  ��ʾ�ɹ�д�� dlen ���ֽڵ�����
 */
ACL_API int acl_vstream_buffed_vfprintf(ACL_VSTREAM *fp, const char *fmt, va_list ap);

/**
 * �������ʽ�������, ������ fprintf()
 * @param fp {ACL_VSTREAM*} ������ 
 * @param fmt {const char*} ���ݸ�ʽ 
 * @param ... �������
 * @return ret {int}, ret == ACL_VSTREAM_EOF: ��ʾд����, Ӧ�ùرձ���������,
 *  ret > 0:  ��ʾ�ɹ�д�� dlen ���ֽڵ�����
 */
ACL_API int ACL_PRINTF(2, 3) acl_vstream_buffed_fprintf(ACL_VSTREAM *fp,
	const char *fmt, ...);

/**
 * ���׼�����ӡ��Ϣ
 * @param fmt {const char*} ���ݸ�ʽ 
 * @param ... �������
 * @return {int}, ACL_VSTREAM_EOF: ��ʾд����, > 0:  ��ʾ�ɹ�д�� dlen ���ֽڵ�����
 */
ACL_API int acl_vstream_buffed_printf(const char*, ...);

/**
 * ������������д��һ������
 * @param s {const char*} Դ�ַ���
 * @param fp {ACL_VSTREAM*} ������
 * @return {int} 0 �ɹ�; ACL_VSTREAM_EOF ʧ��
 */
ACL_API int acl_vstream_buffed_fputs(const char *s, ACL_VSTREAM *fp);

/**
 * ���׼�������������д��һ������
 * @param s {const char*} Դ�ַ���
 * @return {int} 0 �ɹ�; ACL_VSTREAM_EOF ʧ��
 */
ACL_API int acl_vstream_buffed_puts(const char *s);

/**
* һ����д��������, ����ʵ��д����ֽ���.
* @param fp {ACL_VSTREAM*} ������ 
* @param vptr {const void*} ������ָ���ַ
* @param dlen {int} ��д�����������ݳ���
* @return ret {int}, ret == ACL_VSTREAM_EOF: ��ʾд����, Ӧ�ùرձ���������,
*  ret > 0:  ��ʾ�ɹ�д�� ret ���ֽڵ�����
*/
ACL_API int acl_vstream_write(ACL_VSTREAM *fp, const void *vptr, int dlen);

/**
 * һ����д�������������� writev ģʽ������ʵ��д����ֽ���
 * @param fp {ACL_VSTREAM*}
 * @param vector {const struct iovec*}
 * @param count {int} vector ����ĳ���
 * @return {int} ���سɹ�д����ֽ�������������򷵻ءACL_VSTREAM_EOF
 */
ACL_API int acl_vstream_writev(ACL_VSTREAM *fp, const struct iovec *vector, int count);

/**
 * ���� writev ģʽ������д��ֱ��ȫ������д��Ϊֹ�����
 * @param fp {ACL_VSTREAM*}
 * @param vector {const struct iovec*}
 * @param count {int} vector ����ĳ���
 * @return {int} ���سɹ�д����ֽ�������������򷵻ءACL_VSTREAM_EOF
 */
ACL_API int acl_vstream_writevn(ACL_VSTREAM *fp, const struct iovec *vector, int count);

/**
 * ����ʽ�������, ������ vfprintf()
 * @param fp {ACL_VSTREAM*} ������ 
 * @param fmt {const char*} ���ݸ�ʽ
 * @param ap {va_list}
 * @return ret {int}, ret == ACL_VSTREAM_EOF: ��ʾд����, Ӧ�ùرձ���������,
 *  ret > 0:  ��ʾ�ɹ�д�� dlen ���ֽڵ�����
 */
ACL_API int acl_vstream_vfprintf(ACL_VSTREAM *fp, const char *fmt, va_list ap);

/**
 * ����ʽ�������, ������ fprintf()
 * @param fp {ACL_VSTREAM*} ������ 
 * @param fmt {const char*} ���ݸ�ʽ 
 * @param ... �������
 * @return ret {int}, ret == ACL_VSTREAM_EOF: ��ʾд����, Ӧ�ùرձ���������,
 *  ret > 0:  ��ʾ�ɹ�д�� dlen ���ֽڵ�����
 */
ACL_API int ACL_PRINTF(2, 3) acl_vstream_fprintf(ACL_VSTREAM *fp,
	const char *fmt, ...);

/**
 * ���׼�����ӡ��Ϣ
 * @param fmt {const char*} ���ݸ�ʽ 
 * @param ... �������
 * @return {int}, ACL_VSTREAM_EOF: ��ʾд����, > 0:  ��ʾ�ɹ�д�� dlen ���ֽڵ�����
 */
ACL_API int acl_vstream_printf(const char*, ...);

/**
 * ������д��һ������
 * @param s {const char*} Դ�ַ���
 * @param fp {ACL_VSTREAM*} ������
 * @return {int} 0 �ɹ�; ACL_VSTREAM_EOF ʧ��
 */
ACL_API int acl_vstream_fputs(const char *s, ACL_VSTREAM *fp);

/**
 * ���׼�������д��һ������
 * @param s {const char*} Դ�ַ���
 * @return {int} 0 �ɹ�; ACL_VSTREAM_EOF ʧ��
 */
ACL_API int acl_vstream_puts(const char *s);

/**
 * ѭ������������д dlen ���ֽڵ�����ֱ��д������Ϊֹ
 * @param fp {ACL_VSTREAM*} ������ 
 * @param vptr {const char*} ������ָ���ַ
 * @param dlen {size_t} ��д�����������ݳ���
 * @return ret {int}, ret == ACL_VSTREAM_EOF: ��ʾд����, Ӧ�ùرձ���������,
 *  ret > 0:  ��ʾ�ɹ�д�� dlen ���ֽڵ�����
 */
ACL_API int acl_vstream_writen(ACL_VSTREAM *fp, const void *vptr, size_t dlen);
#define	acl_vstream_fwrite	acl_vstream_writen

/**
 * �ͷ�һ�����������ڴ�ռ�, �������ر� socket ������
 * @param fp {ACL_VSTREAM*} ������
 */
ACL_API void acl_vstream_free(ACL_VSTREAM *fp);

/**
 * �ͷ�һ�����������ڴ�ռ䲢�ر�����Я���� socket ������
 * @param fp {ACL_VSTREAM*} ������
 */
ACL_API int acl_vstream_close(ACL_VSTREAM *fp);
#define	acl_vstream_fclose	acl_vstream_close

/**
 * �����������е����йرջص�����ͬʱ�����Щ�ص�����
 * @param fp {ACL_VSTREAM*} ������
 */
ACL_API void acl_vstream_call_close_handles(ACL_VSTREAM *fp);

/**
 * ע��һ���رպ���
 * @param fp {ACL_VSTREAM*} ������
 * @param close_fn {void (*)(ACL_VSTREAM*, void*)} �رպ���ָ��
 * @param context {void*} close_fn ����Ҫ�Ĳ���
 */
ACL_API void acl_vstream_add_close_handle(ACL_VSTREAM *fp,
		void (*close_fn)(ACL_VSTREAM*, void*), void *context);

/**
 * ɾ��һ���رվ��.
 * @param fp {ACL_VSTREAM*} ������
 * @param close_fn {void (*)(ACL_VSTREAM*, void*)} �رպ���ָ��
 * @param context {void*} close_fn ����Ҫ�Ĳ���
 */
ACL_API void acl_vstream_delete_close_handle(ACL_VSTREAM *fp,
		void (*close_fn)(ACL_VSTREAM*, void*), void *context);
/**
 * ���һ�������������еĹرվ��
 * @param fp {ACL_VSTREAM*} ������
 */
ACL_API void acl_vstream_clean_close_handle(ACL_VSTREAM *fp);

/**
 * ���¸�λ���������ڲ�����ָ�뼰����ֵ
 * @param fp {ACL_VSTREAM*} ������
 */
ACL_API void acl_vstream_reset(ACL_VSTREAM *fp);

/**
 * ȡ�õ�ǰ�������Ĵ���״̬
 * @param fp {ACL_VSTREAM*} ������
 * @return {const char*} ��������
 */
ACL_API const char *acl_vstream_strerror(ACL_VSTREAM *fp);

/*-----------------------  ����Ϊ���õĺ꺯�� ------------------------------*/
/**
 * ��fp ���ж�ȡһ���ֽڵĺ�ʵ�֣�Ч��Ҫ�� acl_vstream_getc()/1 ��
 * @param fp {ACL_VSTREAM*} ������ָ��
 * @return {int} ACL_VSTREAM_EOF(����) ����������ĳ���ֽڵ�ASCII,
 *  ��Ϊ ACL_VSTREAM_EOF: �������Է��ر�������, Ӧ�ùرո�������
 */
#define ACL_VSTREAM_GETC(stream_ptr) (                        \
    (stream_ptr)->read_cnt > 0 ?                              \
        (stream_ptr)->read_cnt--,                             \
        (stream_ptr)->offset++,                               \
        *(stream_ptr)->read_ptr++:                            \
        (stream_ptr)->sys_getc((stream_ptr)))

/**
 * �� fp ����дһ���ֽڵĺ�ʵ��
 * @param fp {ACL_VSTREAM*} ������ָ��
 * @return {int} ACL_VSTREAM_EOF(����) ����д���ֽڵ� ASCII
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
 * ��������׽���
 * @param stream_ptr {ACL_VSTREAM*}
 */
#define ACL_VSTREAM_SOCK(stream_ptr) ((stream_ptr)->fd.sock)

/**
 * ��������ļ����
 * @param stream_ptr {ACL_VSTREAM*}
 */
#define ACL_VSTREAM_FILE(stream_ptr) ((stream_ptr)->fd.h_file)

/**
 * ����ļ���������ļ�·����
 * @param stream_ptr {ACL_VSTREAM*}
 */
#define	ACL_VSTREAM_PATH(stream_ptr) ((stream_ptr)->path)

/**
 * �� ACL_VSTREAM Ϊ�ļ���ʱ�������ļ�����·��
 * @param fp {ACL_VSTREAM*} �ļ���
 * @param path {const char*} �ļ�·��
 */
ACL_API void acl_vstream_set_path(ACL_VSTREAM *fp, const char *path);

/**
 * �� ACL_VSTREAM Ϊ������ʱ���ô˺�ȡ�öԷ��ĵ�ַ
 */
#define	ACL_VSTREAM_PEER(stream_ptr) ((stream_ptr)->addr_peer)

/**
 * �� ACL_VSTREAM Ϊ������ʱ���˺�������Զ�����ӵ�ַ
 * @param fp {ACL_VSTREAM*} ���������ǿ�
 * @param addr {const char*} Զ�����ӵ�ַ���ǿ�
 */
ACL_API void acl_vstream_set_peer(ACL_VSTREAM *fp, const char *addr);

/**
 * �� ACL_VSTREAM Ϊ������ʱ���˺�������Զ�����ӵ�ַ
 * @param fp {ACL_VSTREAM*} ���������ǿ�
 * @param sa {const struct sockaddr_in *} Զ�����ӵ�ַ���ǿ�
 */
ACL_API void acl_vstream_set_peer_addr(ACL_VSTREAM *fp,
	const struct sockaddr_in *sa);

/**
 * �� ACL_VSTREAM Ϊ������ʱ���ô˺�ȡ�ñ��صĵ�ַ
 */
#define	ACL_VSTREAM_LOCAL(stream_ptr) ((stream_ptr)->addr_local)

/**
 * �� ACL_VSTREAM Ϊ������ʱ���˺������ñ��ص�ַ
 * @param fp {ACL_VSTREAM*} ���������ǿ�
 * @param addr {const char*} ���ص�ַ���ǿ�
 */
ACL_API void acl_vstream_set_local(ACL_VSTREAM *fp, const char *addr);

/**
 * �� ACL_VSTREAM Ϊ������ʱ���˺������ñ��ص�ַ
 * @param fp {ACL_VSTREAM*} ���������ǿ�
 * @param sa {const sockaddr_in*} ���ص�ַ���ǿ�
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
 * �趨���Ķ�/д�׽���
 * @param stream_ptr {ACL_VSTREAM*}
 * @param _fd {ACL_SOCKET} �׽���
 */
#define	ACL_VSTREAM_SET_SOCK(stream_ptr, _fd) do {            \
        ACL_VSTREAM *__stream_ptr = stream_ptr;               \
        __stream_ptr->fd.sock   = _fd;                        \
} while (0)

/**
 * �������е��ļ����
 * @param stream_ptr {ACL_VSTREAM*}
 * @param _fh {ACL_FILE_HANDLE}
 */
#define	ACL_VSTREAM_SET_FILE(stream_ptr, _fh) do {            \
        ACL_VSTREAM *__stream_ptr = stream_ptr;               \
        __stream_ptr->fd.h_file = _fh;                        \
} while (0)

/* һЩ�ȽϿ��ٵĺ������ģʽ */

/**
 * �����ڶ��������е���������С
 * @param stream_ptr {ACL_VSTREAM*) ���͵�ָ��
 * @return -1: ��ʾ����, >= 0 ��ֵ��Ϊ�����������е���������С
 */
#define	ACL_VSTREAM_BFRD_CNT(stream_ptr)                      \
	((stream_ptr) == NULL ? -1 : (stream_ptr)->read_cnt)

/**
 * �趨���Ķ�д��ʱֵ
 * @param stream_ptr {ACL_VSTREAM*) ���͵�ָ��
 * @param _rw_timeo {int} ��ʱֵ��С(����Ϊ��λ)
 */
#define	ACL_VSTREAM_SET_RWTIMO(stream_ptr, _rw_timeo) do {    \
        ACL_VSTREAM *__stream_ptr  = stream_ptr;              \
        __stream_ptr->rw_timeout = _rw_timeo;                 \
} while (0)

/**
 * ������Ϊ����״̬
 * @param stream_ptr {ACL_VSTREAM*) ���͵�ָ��
 */
#define	ACL_VSTREAM_SET_EOF(stream_ptr) do {                  \
        ACL_VSTREAM *__stream_ptr = stream_ptr;               \
        __stream_ptr->flag |= ACL_VSTREAM_FLAG_EOF;           \
} while (0)

/**
 * �ж��������Ƿ���˴�
 * @param stream_ptr: ACL_VSTREAM ���͵�ָ��
 * @return 0��ʾ����, ��0��ʾ����
 */
#define ACL_IF_VSTREAM_ERR(stream_ptr)                        \
	((stream_ptr)->flag & ACL_VSTREAM_FLAG_BAD)

#ifdef  __cplusplus
}
#endif

/**
 * ����������ȡ���������д�йص�ϵͳ�����
 * @param stream_ptr {ACL_VSTREAM*) ���͵�ָ��
 * @return err {int} ���δ���ţ������߿����� strerror(err) �ķ�ʽ�鿴���庬��
 */
#define	ACL_VSTREAM_ERRNO(stream_ptr) ((stream_ptr)->errnum)

/**
 * �ж�һ�����Ƿ�ʱ
 * @param stream_ptr {ACL_VSTREAM*) ���͵�ָ��
 * @return {int} 0: ��; != 0: ��
 */
#define	acl_vstream_ftimeout(stream_ptr) \
        ((stream_ptr)->flag & ACL_VSTREAM_FLAG_TIMEOUT)

#endif

