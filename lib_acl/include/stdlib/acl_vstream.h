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
#include <fcntl.h>
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

/* When closing and freeing a stream, some cleanup functions need
 * to be called back. This structure provides this callback
 * mechanism ---add by zsx, 2006.6.20
 */
typedef struct ACL_VSTREAM_CLOSE_HANDLE {
	void (*close_fn)(ACL_VSTREAM*, void*);
	void *context;
} ACL_VSTREAM_CLOSE_HANDLE;

/* Virtual stream read/write object */
struct ACL_VSTREAM {
	union {
		ACL_SOCKET      sock;   /**< the master socket */
		ACL_FILE_HANDLE h_file; /**< the file handle */
	} fd;

	int   is_nonblock;              /**< just for WINDOWS, because
					 *   the ioctlsocket is too weak */
	int   type;                     /**< defined as: ACL_VSTREAM_TYPE_XXX */
#define	ACL_VSTREAM_TYPE_SOCK           (1 << 0)
#define	ACL_VSTREAM_TYPE_FILE           (1 << 1)
#define	ACL_VSTREAM_TYPE_LISTEN		(1 << 2)
#define	ACL_VSTREAM_TYPE_LISTEN_INET    (1 << 3)
#define	ACL_VSTREAM_TYPE_LISTEN_UNIX    (1 << 4)
#define ACL_VSTREAM_TYPE_LISTEN_IOCP    (1 << 5)
#define ACL_VSTREAM_TYPE_INET4		(1 << 6)
#define ACL_VSTREAM_TYPE_INET6		(1 << 7)
#define ACL_VSTREAM_TYPE_UNIX		(1 << 8)

	acl_off_t offset;               /**< cached seek info */
	acl_off_t sys_offset;           /**< cached seek info */

	unsigned char *wbuf;            /**< used when call
					 *   acl_vstream_buffed_writen */
	unsigned wbuf_size;             /**< used when call
					 *   acl_vstream_buffed_writen */
	int   wbuf_dlen;                /**< used when call
					 *   acl_vstream_buffed_writen */

	unsigned char *read_buf;        /**< read buff */
	unsigned read_buf_len;             /**< read_buf's capacity */
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
#define	ACL_VSTREAM_FLAG_DEFER_FREE	(1 << 4)	/**< Defer close */

#define	ACL_VSTREAM_FLAG_ERR            (1 << 10)	/**< Error occurred */
#define	ACL_VSTREAM_FLAG_EOF            (1 << 11)	/**< End of file */
#define	ACL_VSTREAM_FLAG_TIMEOUT        (1 << 12)	/**< Timeout */
#define	ACL_VSTREAM_FLAG_RDSHORT        (1 << 13)	/**< Read incomplete */
#define ACL_VSTREAM_FLAG_BAD  (ACL_VSTREAM_FLAG_ERR \
                               | ACL_VSTREAM_FLAG_EOF \
                               | ACL_VSTREAM_FLAG_TIMEOUT)
#define	ACL_VSTREAM_FLAG_CLIENT         (1 << 14)
#define	ACL_VSTREAM_FLAG_CONNECT        (1 << 15)
#define	ACL_VSTREAM_FLAG_SOCKPAIR       (1 << 16)

#define	ACL_VSTREAM_FLAG_TAGYES	        (1 << 17)	/* Flag bit
							 * indicating whether
							 * tag was found */

#define	ACL_VSTREAM_FLAG_CONNECTING     (1 << 18)	/* Currently connecting */
#define	ACL_VSTREAM_FLAG_PREREAD	(1 << 19)	/* Used by
							 * acl_vstream_can_read
							 * to set whether to
							 * enable preread */

#define ACL_VSTREAM_FLAG_MS		(1 << 20)	/**< Millisecond timeout unit */
#define ACL_VSTREAM_FLAG_US		(1 << 21)	/**< Microsecond timeout unit */
#define ACL_VSTREAM_FLAG_NS		(1 << 22)	/**< Nanosecond timeout unit */

#define ACL_VSTREAM_FLAG_BIND_IFACE_OK	(1 << 23)	/**< Binding to
							 *   network interface
							 *   succeeded */
#define ACL_VSTREAM_FLAG_BIND_IP_OK	(1 << 24)	/**< Binding to
							 *   local IP
							 *   succeeded */

/* Set millisecond timeout unit */
#define ACL_VSTREAM_SET_MS(x)	((x)->flag |= ACL_VSTREAM_FLAG_MS)
/* Set microsecond timeout unit */
#define ACL_VSTREAM_SET_US(x)	((x)->flag |= ACL_VSTREAM_FLAG_US)
/* Set nanosecond timeout unit */
#define ACL_VSTREAM_SET_NS(x)	((x)->flag |= ACL_VSTREAM_FLAG_NS)

/* Clear millisecond timeout unit */
#define ACL_VSTREAM_CLR_MS(x)	((x)->flag &= ~ACL_VSTREAM_FLAG_MS)
/* Clear microsecond timeout unit */
#define ACL_VSTREAM_CLR_US(x)	((x)->flag &= ~ACL_VSTREAM_FLAG_US)
/* Clear nanosecond timeout unit */
#define ACL_VSTREAM_CLR_NS(x)	((x)->flag &= ~ACL_VSTREAM_FLAG_NS)

/* Check if millisecond timeout unit is set */
#define ACL_VSTREAM_IS_MS(x)	(((x)->flag & ACL_VSTREAM_FLAG_MS) != 0)
/* Check if microsecond timeout unit is set */
#define ACL_VSTREAM_IS_US(x)	(((x)->flag & ACL_VSTREAM_FLAG_US) != 0)
/* Check if nanosecond timeout unit is set */
#define ACL_VSTREAM_IS_NS(x)	(((x)->flag & ACL_VSTREAM_FLAG_NS) != 0)

	int   errnum;                   /**< record the system errno here */
	int   rw_timeout;               /**< read/write timeout */
	char *addr_local;               /**< the local addr of the fp */
	char *addr_peer;                /**< the peer addr of the fp */
	struct sockaddr *sa_local;      /**< for IPV4/IPV6/UNIX */
	struct sockaddr *sa_peer;       /**< for IPV4/IPV6/UNIX */
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
#define ACL_VSTREAM_IN          (&acl_vstream_fstd[0]) /**< Standard input */
#define ACL_VSTREAM_OUT         (&acl_vstream_fstd[1]) /**< Standard output */
#define ACL_VSTREAM_ERR         (&acl_vstream_fstd[2]) /**< Standard
							 *   error output */

/*--------------------------------------------------------------------------*/
/**
 * Initialize ACL_VSTREAM-related functions.
 * For _WIN32, if you need to use standard input/output/error
 * streams, you need to call this function for initialization.
 */
ACL_API void acl_vstream_init(void);

/**
 * Set the default value used internally, indicating the write buffer size.
 * @poaram size {unsigned}
 */
ACL_API void acl_vstream_set_wbuf_size(unsigned size);

/**
 * Set the default value used internally, indicating the read buffer size.
 * @poaram size {unsigned}
 */
ACL_API void acl_vstream_set_rbuf_size(unsigned size);

/**
 * Function: Probe whether there is data in the socket, and copy
 * the data in the system buffer to the internal buffer.
 * @param fp {ACL_VSTREAM*} Stream pointer, must not be NULL
 * @return ret {int}, ret > 0 OK; ret <= 0 Error
 * Note: This function should be used for sockets.
 */
ACL_API int acl_vstream_peekfd(ACL_VSTREAM *fp);

/**
 * Clone an ACL_VSTREAM object, excluding ioctl_read_ctx, ioctl_write_ctx, fdp.
 * The cloned data are all static memory data, and the new object
 * internally allocates dynamic memory and copies the source data.
 * @param stream_src {ACL_VSTREAM*} Source stream pointer
 * @return {ACL_VSTREAM*} Target stream pointer
 */
ACL_API ACL_VSTREAM *acl_vstream_clone(const ACL_VSTREAM *stream_src);

/**
 * Set the stream's file descriptor type. This function will set
 * the corresponding read/write/close functions based on the
 * stream type.
 * @param fp {ACL_VSTREAM*} Stream pointer, must not be NULL
 * @param type {int} Stream file descriptor type, defined above:
 *  ACL_VSTREAM_TYPE_XXX
 * @return ret {int}, ret >= 0 OK; ret < 0 Error
 */
ACL_API int acl_vstream_set_fdtype(ACL_VSTREAM *fp, int type);

/**
 * Open a file handle and create a corresponding stream.
 * @param fh {ACL_FILE_HANDLE} File handle
 * @param oflags {unsigned int} Flag bits, We're assuming that O_RDONLY: 0x0000,
 *  O_WRONLY: 0x0001, O_RDWR: 0x0002, O_APPEND: 0x0008, O_CREAT: 0x0100,
 *  O_TRUNC: 0x0200, O_EXCL: 0x0400; just for win32, O_TEXT: 0x4000,
 *  O_BINARY: 0x8000, O_RAW: O_BINARY, O_SEQUENTIAL: 0x0020, O_RANDOM: 0x0010.
 *  Simultaneously set.
 * @return {ACL_VSTREAM*} Stream object
 */
ACL_API ACL_VSTREAM *acl_vstream_fhopen(ACL_FILE_HANDLE fh, unsigned int oflags);

/**
 * Open a file descriptor.
 * @param fd {ACL_SOCKET} File descriptor (can be a socket
 *  descriptor or a file descriptor)
 * @param oflags {unsigned int} Flag bits, We're assuming that O_RDONLY: 0x0000,
 *  O_WRONLY: 0x0001, O_RDWR: 0x0002, O_APPEND: 0x0008, O_CREAT: 0x0100,
 *  O_TRUNC: 0x0200, O_EXCL: 0x0400; just for win32, O_TEXT: 0x4000,
 *  O_BINARY: 0x8000, O_RAW: O_BINARY, O_SEQUENTIAL: 0x0020, O_RANDOM: 0x0010.
 * @param buflen {size_t} Buffer size to be set
 * @param rw_timeout {int} Read/write timeout time (default unit
 *  is seconds, when ACL_VSTREAM_IS_MS() unit is milliseconds),
 *  when value >= 0, enables read/write timeout checking, < 0
 *  disables checking.
 * @param fdtype {int} ACL_VSTREAM_TYPE_FILE, ACL_VSTREAM_TYPE_SOCK,
 *  ACL_VSTREAM_TYPE_LISTEN | ACL_VSTREAM_TYPE_LISTEN_INET |
 *  ACL_VSTREAM_TYPE_LISTEN_UNIX
 * @return ret {ACL_VSTREAM*}, ret == NULL: Error, ret != NULL: OK
 */
ACL_API ACL_VSTREAM *acl_vstream_fdopen(ACL_SOCKET fd, unsigned int oflags,
		size_t buflen, int rw_timeout, int fdtype);

/**
 * Open a file and create a stream.
 * @param path {const char*} File path
 * @param oflags {unsigned int} Flag bits, We're assuming that O_RDONLY: 0x0000,
 *  O_WRONLY: 0x0001, O_RDWR: 0x0002, O_APPEND: 0x0008, O_CREAT: 0x0100,
 *  O_TRUNC: 0x0200, O_EXCL: 0x0400; just for win32, O_TEXT: 0x4000,
 *  O_BINARY: 0x8000, O_RAW: O_BINARY, O_SEQUENTIAL: 0x0020, O_RANDOM: 0x0010.
 * @param mode {int} File creation mode when creating file (e.g.: 0600)
 * @param buflen {size_t} Buffer size to be set
 * @return ret {ACL_VSTREAM*}, ret== NULL: Error, ret != NULL: OK
 */
ACL_API ACL_VSTREAM *acl_vstream_fopen(const char *path, unsigned int oflags,
		int mode, size_t buflen);

/**
 * Load the entire file content into memory.
 * @param path {const char*} File path, e.g.: /opt/acl/conf/service/test.cf
 * @return {char*} Buffer containing the entire file content, the
 *  caller needs to call acl_myfree to free this memory.
 */
ACL_API char *acl_vstream_loadfile(const char *path);

/**
 * Load the entire file content into memory.
 * @param path {const char*} File path, e.g.: /opt/acl/conf/service/test.cf
 * @param size {ssize_t*} If not NULL, stores the returned buffer
 *  size. If file reading fails, this value will be set to -1
 * @return {char*} Buffer containing the entire file content, the
 *  caller needs to call acl_myfree to free this memory.
 */
ACL_API char *acl_vstream_loadfile2(const char *path, ssize_t *size);

/**
 * Control the stream's various parameters.
 * @param fp {ACL_VSTREAM*} Stream pointer
 * @param name {int} The first parameter in the parameter list to be set,
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
 * Reposition file pointer.
 * @param fp {ACL_VSTREAM*} Stream pointer
 * @param offset {acl_off_t} Offset value
 * @param whence {int} Offset direction, SEEK_SET, SEEK_CUR, SEEK_END
 * @return ret {acl_off_t}, ret >= 0: Correct, ret < 0: Error
 * Note: acl_vstream_fseek() is more efficient, it uses cached
 * buffer functionality, while acl_vstream_fseek2() calls the
 * lseek() system call once more.
 */
ACL_API acl_off_t acl_vstream_fseek(ACL_VSTREAM *fp, acl_off_t offset, int whence);

/**
 * Reposition file pointer.
 * @param fp {ACL_VSTREAM*} Stream pointer
 * @param offset {acl_off_t} Offset value
 * @param whence {int} Movement direction: SEEK_SET moves from
 *  the beginning of the file, SEEK_CUR moves from the current
 *  file pointer position, SEEK_END moves from the end of the
 *  file forward
 * @return ret {acl_off_t}, ret >= 0: Correct, ret < 0: Error
 * @DEPRECATED This function has lower efficiency.
 */
ACL_API acl_off_t acl_vstream_fseek2(ACL_VSTREAM *fp, acl_off_t offset, int whence);

/**
 * Get the current file pointer's absolute position.
 * @param fp {ACL_VSTREAM*} Stream pointer
 * @return {acl_off_t} Current file pointer's absolute position,
 *  -1 indicates error
 */
ACL_API acl_off_t acl_vstream_ftell(ACL_VSTREAM *fp);

/**
 * Truncate the source file to a specified length.
 * @param fp {ACL_VSTREAM*} Stream pointer
 * @param length {acl_off_t} Data length (>=0)
 * @return {int} 0: ok, -1: error
 */
ACL_API int acl_file_ftruncate(ACL_VSTREAM *fp, acl_off_t length);

/**
 * Truncate the source file to a specified length.
 * @param path {const char*} File path (can be absolute path or relative path)
 * @param length {acl_off_t} Data length (>=0)
 * @return {int} 0: ok, -1: error
 */
ACL_API int acl_file_truncate(const char *path, acl_off_t length);

/**
 * Check a file's status information.
 * @param fp {ACL_VSTREAM *} File stream
 * @param buf {acl_stat *} Address of structure to store status
 * @return {int} 0: ok; -1: error
 */
ACL_API int acl_vstream_fstat(ACL_VSTREAM *fp, struct acl_stat *buf);

/**
 * Check a file's size.
 * @param fp {ACL_VSTREAM *} File stream
 * @return {int} >= 0: ok;  -1: error
 */
ACL_API acl_int64 acl_vstream_fsize(ACL_VSTREAM *fp);

/**
 * Read a byte from the stream.
 * @param fp {ACL_VSTREAM*} Stream pointer
 * @return {int} ACL_VSTREAM_EOF (error) or the ASCII value of the read byte.
 *  If it is ACL_VSTREAM_EOF: the stream was closed by the
 *  peer, the application should close the stream.
 */
ACL_API int acl_vstream_getc(ACL_VSTREAM *fp);
#define	acl_vstream_get_char	acl_vstream_getc

/**
 * Read size bytes from a non-blocking stream.
 * @param fp {ACL_VSTREAM*} Stream pointer
 * @param buf {char*} User-provided memory buffer
 * @param size {int} Space size of buf buffer
 * @return {int} Number of bytes read n, if n == ACL_VSTREAM_EOF
 *  indicates error, otherwise
 *         n >= 0 is correct.
 */
ACL_API int acl_vstream_nonb_readn(ACL_VSTREAM *fp, char *buf, int size);

/**
 * Check whether a socket stream has been closed by the system.
 * When there is no data in the buffer, this function will try to
 * read one byte from the system's socket buffer to determine
 * whether the socket stream has been closed. If it successfully
 * reads one byte, it means the socket stream is still open, and
 * simultaneously puts the read data back into the buffer area.
 * If it returns ACL_VSTREAM_EOF, it is necessary to determine
 * whether the stream was closed.
 * @param fp {ACL_VSTREAM*} Stream pointer
 * @return {int}, 0 indicates the socket is open; -1 indicates
 *  the socket stream has been closed by the system
 */
ACL_API int acl_vstream_probe_status(ACL_VSTREAM *fp);

/**
 * Push a character back into the stream.
 * @param fp {ACL_VSTREAM*} Stream pointer
 * @param ch {int} Character's ASCII value 
 * @return {int} Character's ASCII value, this function should
 *  succeed, otherwise internal memory allocation failure will
 *  cause core dump.
 */
ACL_API int acl_vstream_ungetc(ACL_VSTREAM *fp, int ch);

/**
 * Push data of specified length back into the stream buffer.
 * @param fp {ACL_VSTREAM*} Stream pointer
 * @param ptr {const void *} Starting address of the data to be
 *  pushed back into the buffer
 * @param length {size_t} Length of the data to be pushed back into the buffer
 * @return {int} Length of data successfully pushed back into
 *  the buffer, should be equal to length, otherwise internal
 *  memory allocation failure will automatically cause core dump!
 */
ACL_API int acl_vstream_unread(ACL_VSTREAM *fp, const void *ptr, size_t length);

/**
 * Read a line from the stream, until encountering "\n" or end of
 * stream, the returned result includes "\n"
 * @param fp {ACL_VSTREAM*} Stream
 * @param vptr {void*} User-provided memory buffer pointer
 * @param maxlen {size_t} Size of vptr buffer
 * @return  ret {int}, ret == ACL_VSTREAM_EOF:  The stream was
 *  closed by the peer, should close the stream; n > 0:
 *  Successfully read n bytes of data, where n may be less than
 *  maxlen. The last character at position 0 is "\n" indicating
 *  a complete line was read, otherwise n indicates the data
 *  from the peer did not send "\n" and closed the stream;
 *  otherwise through checking (fp->flag &
 *  ACL_VSTREAM_FLAG_TAGYES) whether it is 0 to determine
 *  whether "\n" was found, if it is 0 it means "\n" was found.
 */
ACL_API int acl_vstream_gets(ACL_VSTREAM *fp, void *vptr, size_t maxlen);
#define	acl_vstream_readline	acl_vstream_gets
#define	acl_vstream_fgets	acl_vstream_gets

/**
 * Read a line from the stream, until encountering "\n" or end
 * of stream, the returned result does not include "\n"
 * @param fp {ACL_VSTREAM*} Stream 
 * @param vptr {void*} User-provided memory buffer pointer
 * @param maxlen {size_t} Size of vptr buffer
 * @return ret {int}, ret == ACL_VSTREAM_EOF:  The stream was closed by the peer,
 *  should close the stream, n == 0: An empty line was read, the read data ends with "\r\n",
 *  n > 0:  Successfully read n bytes of data.
 */
ACL_API int acl_vstream_gets_nonl(ACL_VSTREAM *fp, void *vptr, size_t maxlen);

/**
 * Read data from the stream with a string as a tag delimiter.
 * @param fp {ACL_VSTREAM*} Stream pointer
 * @param vptr {void*} Data storage buffer
 * @param maxlen {size_t} vptr buffer size
 * @param tag {const char*} String tag
 * @param taglen {size_t} Length of tag data
 * @return ret {int}, ret == ACL_VSTREAM_EOF:  The stream was
 *  closed by the peer, should close the stream; n > 0:
 *  Successfully read n bytes of data, if the required tag is
 *  found, then fp flag (fp->flag & ACL_VSTREAM_FLAG_TAGYES) is
 *  not 0.
 */
ACL_API int acl_vstream_readtags(ACL_VSTREAM *fp, void *vptr, size_t maxlen,
		const char *tag, size_t taglen);

/**
 * Loop to read maxlen bytes of data, until maxlen bytes are
 * read or an error occurs.
 * @param fp {ACL_VSTREAM*} Stream
 * @param vptr {void*} User-provided data buffer pointer
 * @param maxlen {size_t} vptr data buffer space size
 * @return ret {int}, ret == ACL_VSTREAM_EOF:  The stream was
 *  closed by the peer, should close the stream; n > 0:
 *  Successfully read maxlen bytes of data. If the actual bytes
 *  read are less than maxlen, an error will also be returned
 *  (ACL_VSTREAM_EOF)
 */
ACL_API int acl_vstream_readn(ACL_VSTREAM *fp, void *vptr, size_t maxlen);

/**
 * Copy the data in the stream buffer to vptr.
 * @param fp {ACL_VSTREAM*} Stream
 * @param vptr {void*} User-provided data buffer pointer
 * @param maxlen {size_t} vptr data buffer space size
 * @return ret {int}, ret == ACL_VSTREAM_EOF: indicates error,
 *  should close the stream, ret >= 0: Successfully read ret
 *  bytes of data from fp stream's buffer
 */
ACL_API int acl_vstream_bfcp_some(ACL_VSTREAM *fp, void *vptr, size_t maxlen);

/**
 * Read n bytes from the stream at once, where n may be less
 * than the user-requested maxlen
 * @param fp {ACL_VSTREAM*} Stream 
 * @param vptr {void*} User-provided data buffer pointer
 * @param maxlen {size_t} vptr data buffer space size
 * @return ret {int}, ret == ACL_VSTREAM_EOF: indicates error, should close the stream,
 *  ret > 0:  indicates successfully read ret bytes of data
 *  Note: If there is data in the buffer, directly copy the data
 *  in the buffer to the user's buffer and return directly; If
 *  there is no data in the buffer, need to call the system
 *  read() (may call the system read() multiple times), and
 *  then copy the read data to the user's buffer and return.
 *  This function cannot guarantee that the read bytes equal the
 *  requested bytes, if you need to read the exact number of
 *  bytes, please use vstream_loop_readn() function.
 */
ACL_API int acl_vstream_read(ACL_VSTREAM *fp, void *vptr, size_t maxlen);

/**
 * Read a line from ACL_VSTREAM stream system buffer at once,
 * including the newline character (handles differences in
 * newline characters between WINDOWS and UNIX platforms), if no
 * newline character is encountered, also copy the data to the
 * user's memory buffer.
 * @param fp {ACL_VSTREAM*} Stream 
 * @param buf {ACL_VSTRING*} Data buffer, when buf->maxlen > 0,
 *  limits the maximum length per line read. When the data
 *  length in buf reaches maxlen, even if a complete line has
 *  not been read, it will also return, and will set ready to
 *  1. Then check the fp->flag flag bit to see if
 *  ACL_VSTREAM_FLAG_TAGYES is set to determine whether a
 *  complete line was read
 * @param ready {int*} Flag pointer indicating whether a
 *  complete line was read, can be NULL
 * @return ret {int}, ret == ACL_VSTREAM_EOF when checking
 *  acl_last_error() system error is ACL_EWOULDBLOCK or
 *  ACL_EAGAIN, indicates no data available in the socket
 *  stream, otherwise indicates error, should close the stream.
 *  ret >= 0: Successfully read ret bytes of data from fp
 *  stream's buffer
 */
ACL_API int acl_vstream_gets_peek(ACL_VSTREAM *fp, ACL_VSTRING *buf, int *ready);

/**
 * Read a line from ACL_VSTREAM stream system buffer at once,
 * if no newline character is encountered, also copy the data
 * to the user's memory buffer. If a newline character is
 * encountered, the newline character will be automatically
 * removed, and the data before the newline character will be
 * copied to the user's memory.
 * @param fp {ACL_VSTREAM*} Stream 
 * @param buf {ACL_VSTRING*} Data buffer, when buf->maxlen > 0,
 *  limits the maximum length per line read. When the data
 *  length in buf reaches maxlen, even if a complete line has
 *  not been read, it will also return, and will set ready to
 *  1. Then check the fp->flag flag bit to see if
 *  ACL_VSTREAM_FLAG_TAGYES is set to determine whether a
 *  complete line was read
 * @param ready {int*} Flag pointer indicating whether a
 *  complete line was read, can be NULL
 * @return ret {int}, ret == ACL_VSTREAM_EOF when checking
 *  acl_last_error() system error is ACL_EWOULDBLOCK or
 *  ACL_EAGAIN, indicates no data available in the socket
 *  stream, otherwise indicates error, should close the stream.
 *  ret >= 0: Successfully read ret bytes of data from fp
 *  stream's buffer, if
 *  a complete line was read, then ret == 0.
 */
ACL_API int acl_vstream_gets_nonl_peek(ACL_VSTREAM *fp, ACL_VSTRING *buf, int *ready);

/**
 * Read fixed-length data from ACL_VSTREAM stream system buffer
 * at once, if the required data length is not available, also
 * copy the data to the user's memory buffer. If the required
 * data length is available, set the ready flag bit.
 * @param fp {ACL_VSTREAM*} Stream 
 * @param buf {ACL_VSTRING*} Data buffer
 * @param cnt {int} Required data length to read
 * @param ready {int*} Flag pointer indicating whether a
 *  complete line was read, can be NULL
 * @return ret {int}, ret == ACL_VSTREAM_EOF when checking
 *  acl_last_error() system error is ACL_EWOULDBLOCK or
 *  ACL_EAGAIN, indicates no data available in the socket
 *  stream, otherwise indicates error, should close the stream.
 *  ret >= 0: Successfully read ret bytes of data from fp
 *  stream's buffer, 
 *  (*ready) != 0: indicates data of the required length was read.
 */
ACL_API int acl_vstream_readn_peek(ACL_VSTREAM *fp, ACL_VSTRING *buf, int cnt, int *ready);

/**
 * Read non-fixed-length data from ACL_VSTREAM stream system buffer at once.
 * @param fp {ACL_VSTREAM*} Stream 
 * @param buf {ACL_VSTRING*} Data buffer
 * @return ret {int}, ret == ACL_VSTREAM_EOF when checking
 *  acl_last_error() system error is ACL_EWOULDBLOCK or
 *  ACL_EAGAIN, indicates no data available in the socket
 *  stream, otherwise indicates error, should close the stream.
 *  ret >= 0: Successfully read ret bytes of data from fp
 *  stream's buffer.
 */
ACL_API int acl_vstream_read_peek(ACL_VSTREAM *fp, ACL_VSTRING *buf);

/**
 * Read non-fixed-length data from ACL_VSTREAM stream system buffer at once.
 * @param fp {ACL_VSTREAM*} Stream 
 * @param buf {void*} Data buffer
 * @param size {size_t} buf size
 * @return ret {int}, ret == ACL_VSTREAM_EOF when checking
 *  acl_last_error() system error is ACL_EWOULDBLOCK or
 *  ACL_EAGAIN, indicates no data available in the socket
 *  stream, otherwise indicates error, should close the stream.
 *  ret >= 0: Successfully read ret bytes of data from fp
 *  stream's buffer.
 */
ACL_API int acl_vstream_read_peek3(ACL_VSTREAM *fp, void *buf, size_t size);

/**
 * Check if ACL_VSTREAM stream has readable data.
 * @param fp {ACL_VSTREAM*} Stream 
 * @return {int} 0: indicates no data readable; ACL_VSTREAM_EOF
 *  indicates error; > 0 indicates data readable
 */
ACL_API int acl_vstream_can_read(ACL_VSTREAM *fp);

/**
 * Check if ACL_VSTREAM stream is readable. This function first
 * calls acl_vstream_can_read() to check if it is readable. If
 * it is readable, it calls poll() again to check if fd is
 * readable.
 * @param fp {ACL_VSTREAM*} Stream 
 * @return {int} Return value 0 indicates current fd has no
 *  data readable, otherwise 1 indicates data is readable or
 *  the stream has been closed by the peer.
 */
ACL_API int acl_vstream_readable(ACL_VSTREAM *fp);

/**
 * Flush the system buffer and data in the file stream to disk synchronously.
 * @param fp {ACL_VSTREAM*} File stream pointer
 * @return {int} 0: ok; ACL_VSTREAM_EOF: error
 */
ACL_API int acl_vstream_fsync(ACL_VSTREAM *fp);

/**
 * Ensure buffer space is available for buffered write mode.
 * @param fp {ACL_VSTREAM*} Stream
 */
ACL_API void acl_vstream_buffed_space(ACL_VSTREAM *fp);

/**
 * Flush the write buffer data to the socket stream.
 * @param fp socket stream
 * @return Returns the number of bytes flushed to the write
 *  buffer, or ACL_VSTREAM_EOF on error
 */
ACL_API int acl_vstream_fflush(ACL_VSTREAM *fp);

/**
 * Buffered write mode.
 * @param fp {ACL_VSTREAM*} Stream
 * @param vptr {const void*} Data pointer starting position
 * @param dlen {size_t} Data length to write
 * @return {int} Number of bytes written successfully, or
 *  ACL_VSTREAM_EOF on error
 */
ACL_API int acl_vstream_buffed_writen(ACL_VSTREAM *fp, const void *vptr, size_t dlen);
#define	acl_vstream_buffed_fwrite	acl_vstream_buffed_writen

/**
 * Buffered format output function, similar to vfprintf()
 * @param fp {ACL_VSTREAM*} Stream 
 * @param fmt {const char*} Format string
 * @param ap {va_list}
 * @return ret {int}, ret == ACL_VSTREAM_EOF: indicates write
 *  error, should close the stream, ret > 0: indicates
 *  successfully wrote dlen bytes of data
 */
ACL_API int acl_vstream_buffed_vfprintf(ACL_VSTREAM *fp, const char *fmt, va_list ap);

/**
 * Buffered format output function, similar to fprintf()
 * @param fp {ACL_VSTREAM*} Stream 
 * @param fmt {const char*} Format string 
 * @param ... Variable arguments
 * @return ret {int}, ret == ACL_VSTREAM_EOF: indicates write
 *  error, should close the stream, ret > 0: indicates
 *  successfully wrote dlen bytes of data
 */
ACL_API int ACL_PRINTF(2, 3) acl_vstream_buffed_fprintf(ACL_VSTREAM *fp,
	const char *fmt, ...);

/**
 * Print message to standard output.
 * @param ... Variable arguments
 * @return {int}, ACL_VSTREAM_EOF: indicates write error, > 0:
 *  indicates successfully wrote dlen bytes of data
 */
ACL_API int acl_vstream_buffed_printf(const char*, ...);

/**
 * Write a string to the stream in buffered mode.
 * @param s {const char*} Source string
 * @param fp {ACL_VSTREAM*} Stream
 * @return {int} 0 success; ACL_VSTREAM_EOF failure
 */
ACL_API int acl_vstream_buffed_fputs(const char *s, ACL_VSTREAM *fp);

/**
 * Write a string to standard output in buffered mode.
 * @param s {const char*} Source string
 * @return {int} 0 success; ACL_VSTREAM_EOF failure
 */
ACL_API int acl_vstream_buffed_puts(const char *s);

/**
* Write data to the stream at once, returns the actual number of bytes written.
* @param fp {ACL_VSTREAM*} Stream 
* @param vptr {const void*} Data pointer address
* @param dlen {int} Data length to write
 * @return ret {int}, ret == ACL_VSTREAM_EOF: indicates write error,
 *  should close the stream, ret > 0: indicates successfully wrote
 *  ret bytes of data
*/
ACL_API int acl_vstream_write(ACL_VSTREAM *fp, const void *vptr, int dlen);

/**
 * Write data to the stream in writev mode at once, returns
 * the actual number of bytes written
 * @param fp {ACL_VSTREAM*}
 * @param vector {const struct iovec*}
 * @param count {int} Length of vector array
 * @return {int} Returns the number of bytes written successfully, or ACL_VSTREAM_EOF on error
 */
ACL_API int acl_vstream_writev(ACL_VSTREAM *fp, const struct iovec *vector, int count);

/**
 * Loop write in writev mode until all data is written or an error occurs.
 * @param fp {ACL_VSTREAM*}
 * @param vector {const struct iovec*}
 * @param count {int} Length of vector array
 * @return {int} Returns the number of bytes written successfully, or ACL_VSTREAM_EOF on error
 */
ACL_API int acl_vstream_writevn(ACL_VSTREAM *fp, const struct iovec *vector, int count);

/**
 * Format output function, similar to vfprintf()
 * @param fp {ACL_VSTREAM*} Stream 
 * @param fmt {const char*} Format string
 * @param ap {va_list}
 * @return ret {int}, ret == ACL_VSTREAM_EOF: indicates write
 *  error, should close the stream, ret > 0: indicates
 *  successfully wrote dlen bytes of data
 */
ACL_API int acl_vstream_vfprintf(ACL_VSTREAM *fp, const char *fmt, va_list ap);

/**
 * Format output function, similar to fprintf()
 * @param fp {ACL_VSTREAM*} Stream 
 * @param fmt {const char*} Format string 
 * @param ... Variable arguments
 * @return ret {int}, ret == ACL_VSTREAM_EOF: indicates write
 *  error, should close the stream, ret > 0: indicates
 *  successfully wrote dlen bytes of data
 */
ACL_API int ACL_PRINTF(2, 3) acl_vstream_fprintf(ACL_VSTREAM *fp,
	const char *fmt, ...);

/**
 * Print message to standard output.
 * @param ... Variable arguments
 * @return {int}, ACL_VSTREAM_EOF: indicates write error, > 0:
 *  indicates successfully wrote dlen bytes of data
 */
ACL_API int acl_vstream_printf(const char*, ...);

/**
 * Write a string to the stream.
 * @param s {const char*} Source string
 * @param fp {ACL_VSTREAM*} Stream
 * @return {int} 0 success; ACL_VSTREAM_EOF failure
 */
ACL_API int acl_vstream_fputs(const char *s, ACL_VSTREAM *fp);

/**
 * Write a string to standard output.
 * @param s {const char*} Source string
 * @return {int} 0 success; ACL_VSTREAM_EOF failure
 */
ACL_API int acl_vstream_puts(const char *s);

/**
 * Loop to write dlen bytes of data until all data is written
 * or an error occurs.
 * @param fp {ACL_VSTREAM*} Stream 
 * @param vptr {const char*} Data pointer address
 * @param dlen {size_t} Data length to write
 * @return ret {int}, ret == ACL_VSTREAM_EOF: indicates write
 *  error, should close the stream, ret > 0: indicates
 *  successfully wrote dlen bytes of data
 */
ACL_API int acl_vstream_writen(ACL_VSTREAM *fp, const void *vptr, size_t dlen);
#define	acl_vstream_fwrite	acl_vstream_writen

/**
 * Free a stream object's memory space, but do not close the socket descriptor.
 * @param fp {ACL_VSTREAM*} Stream
 */
ACL_API void acl_vstream_free(ACL_VSTREAM *fp);

/**
 * Free a stream object's memory space and close the socket
 * descriptor it carries.
 * @param fp {ACL_VSTREAM*} Stream
 */
ACL_API int acl_vstream_close(ACL_VSTREAM *fp);
#define	acl_vstream_fclose	acl_vstream_close

/**
 * Call all registered close handles for the stream and clear these handles.
 * @param fp {ACL_VSTREAM*} Stream
 */
ACL_API void acl_vstream_call_close_handles(ACL_VSTREAM *fp);

/**
 * Register a close handle.
 * @param fp {ACL_VSTREAM*} Stream
 * @param close_fn {void (*)(ACL_VSTREAM*, void*)} Close function pointer
 * @param context {void*} Parameter needed by close_fn
 */
ACL_API void acl_vstream_add_close_handle(ACL_VSTREAM *fp,
		void (*close_fn)(ACL_VSTREAM*, void*), void *context);

/**
 * Delete a close handle.
 * @param fp {ACL_VSTREAM*} Stream
 * @param close_fn {void (*)(ACL_VSTREAM*, void*)} Close function pointer
 * @param context {void*} Parameter needed by close_fn
 */
ACL_API void acl_vstream_delete_close_handle(ACL_VSTREAM *fp,
		void (*close_fn)(ACL_VSTREAM*, void*), void *context);
/**
 * Clear all close handles for a stream.
 * @param fp {ACL_VSTREAM*} Stream
 */
ACL_API void acl_vstream_clean_close_handle(ACL_VSTREAM *fp);

/**
 * Reset and restore the stream's internal pointer and flag values.
 * @param fp {ACL_VSTREAM*} Stream
 */
ACL_API void acl_vstream_reset(ACL_VSTREAM *fp);

/**
 * Get the current stream's error status.
 * @param fp {ACL_VSTREAM*} Stream
 * @return {const char*} Error message
 */
ACL_API const char *acl_vstream_strerror(ACL_VSTREAM *fp);

/*-------------  The following are commonly used macros ---------------------*/
/**
 * Macro implementation for reading a byte from the stream,
 * more efficient than acl_vstream_getc()/1.
 * @param stream_ptr {ACL_VSTREAM*} Stream pointer
 * @return {int} ACL_VSTREAM_EOF (error) or the ASCII value of the read byte.
 *  If it is ACL_VSTREAM_EOF: the stream was closed by the
 *  peer, the application should close the stream.
 */
#define ACL_VSTREAM_GETC(stream_ptr) (                        \
    (stream_ptr)->read_cnt > 0 ?                              \
        (stream_ptr)->read_cnt--,                             \
        (stream_ptr)->offset++,                               \
        *(stream_ptr)->read_ptr++:                            \
        (stream_ptr)->sys_getc((stream_ptr)))

/**
 * Macro implementation for writing a byte to the stream.
 * @param stream_ptr {ACL_VSTREAM*} Stream pointer
 * @return {int} ACL_VSTREAM_EOF (error) or the written byte's ASCII
 */
#define ACL_VSTREAM_PUTC(ch, stream_ptr) (                                   \
  (stream_ptr)->wbuf_size == 0 ?                                             \
    (acl_vstream_buffed_space((stream_ptr)),                                 \
        ((stream_ptr)->wbuf[(size_t) (stream_ptr)->wbuf_dlen++] = (ch)))     \
    : ((unsigned) (stream_ptr)->wbuf_dlen == stream_ptr->wbuf_size ?         \
        (acl_vstream_fflush((stream_ptr)) == ACL_VSTREAM_EOF ?               \
          ACL_VSTREAM_EOF                                                    \
          : ((stream_ptr)->wbuf[(size_t) (stream_ptr)->wbuf_dlen++] = (ch))) \
        : ((stream_ptr)->wbuf[(size_t) (stream_ptr)->wbuf_dlen++] = (ch))))

/**
 * Get the socket descriptor.
 * @param stream_ptr {ACL_VSTREAM*}
 */
#define ACL_VSTREAM_SOCK(stream_ptr) ((stream_ptr)->fd.sock)

/**
 * Get the file handle.
 * @param stream_ptr {ACL_VSTREAM*}
 */
#define ACL_VSTREAM_FILE(stream_ptr) ((stream_ptr)->fd.h_file)

/**
 * Get the file path for file streams.
 * @param stream_ptr {ACL_VSTREAM*}
 */
#define	ACL_VSTREAM_PATH(stream_ptr) ((stream_ptr)->path ? (stream_ptr)->path : "")

/**
 * When ACL_VSTREAM is a file stream, set the file's path.
 * @param fp {ACL_VSTREAM*} File stream
 * @param path {const char*} File path
 */
ACL_API void acl_vstream_set_path(ACL_VSTREAM *fp, const char *path);

/**
 * When ACL_VSTREAM is a socket stream, use this macro to get
 * the peer's address.
 */
#define	ACL_VSTREAM_PEER(stream_ptr) ((stream_ptr)->addr_peer ? (stream_ptr)->addr_peer : "")

/**
 * When ACL_VSTREAM is a socket stream, use this function to
 * set the remote connection address.
 * @param fp {ACL_VSTREAM*} Socket stream, must not be NULL
 * @param addr {const char*} Remote connection address, must not be NULL
 */
ACL_API void acl_vstream_set_peer(ACL_VSTREAM *fp, const char *addr);

/**
 * When ACL_VSTREAM is a socket stream, use this function to
 * set the remote connection address.
 * @param fp {ACL_VSTREAM*} Socket stream, must not be NULL
 * @param sa {const struct sockaddr *} Remote connection address, must not be NULL
 * @return {int} Return value == 0 indicates success, < 0 indicates failure
 */
ACL_API int acl_vstream_set_peer_addr(ACL_VSTREAM *fp, const struct sockaddr *sa);

/**
 * When ACL_VSTREAM is a socket stream, use this macro to get the local address.
 */
#define	ACL_VSTREAM_LOCAL(stream_ptr) ((stream_ptr)->addr_local ? (stream_ptr)->addr_local : "")

/**
 * When ACL_VSTREAM is a socket stream, use this function to set the local address.
 * @param fp {ACL_VSTREAM*} Socket stream, must not be NULL
 * @param addr {const char*} Local address, must not be NULL
 */
ACL_API void acl_vstream_set_local(ACL_VSTREAM *fp, const char *addr);

/**
 * When ACL_VSTREAM is a socket stream, use this function to set the local address.
 * @param fp {ACL_VSTREAM*} Socket stream, must not be NULL
 * @param sa {const sockaddr*} Local address, must not be NULL
 * @return {int} Return value == 0 indicates success, < 0 indicates failure
 */
ACL_API int acl_vstream_set_local_addr(ACL_VSTREAM *fp, const struct sockaddr *sa);

ACL_API int acl_vstream_add_object(ACL_VSTREAM *fp, const char *key, void *obj);
ACL_API int acl_vstream_del_object(ACL_VSTREAM *fp, const char *key);
ACL_API void *acl_vstream_get_object(ACL_VSTREAM *fp, const char *key);

ACL_API void acl_socket_read_hook(ACL_VSTREAM_RD_FN read_fn);
ACL_API void acl_socket_write_hook(ACL_VSTREAM_WR_FN write_fn);
ACL_API void acl_socket_writev_hook(ACL_VSTREAM_WV_FN writev_fn);
ACL_API void acl_socket_close_hook(int (*close_fn)(ACL_SOCKET));

/**
 * Set the read/write socket descriptor.
 * @param stream_ptr {ACL_VSTREAM*}
 * @param _fd {ACL_SOCKET} Socket descriptor
 */
#define	ACL_VSTREAM_SET_SOCK(stream_ptr, _fd) do {            \
        ACL_VSTREAM *__stream_ptr = stream_ptr;               \
        __stream_ptr->fd.sock   = _fd;                        \
} while (0)

/**
 * Set/get the file handle.
 * @param stream_ptr {ACL_VSTREAM*}
 * @param _fh {ACL_FILE_HANDLE}
 */
#define	ACL_VSTREAM_SET_FILE(stream_ptr, _fh) do {            \
        ACL_VSTREAM *__stream_ptr = stream_ptr;               \
        __stream_ptr->fd.h_file = _fh;                        \
} while (0)

/* Some relatively fast macro patterns */

/**
 * Get the remaining data length in the read buffer.
 * @param stream_ptr {ACL_VSTREAM*) Stream pointer
 * @return -1: indicates error, >= 0 value is the remaining data length in the read buffer
 */
#define	ACL_VSTREAM_BFRD_CNT(stream_ptr)                      \
	((stream_ptr) == NULL ? -1 : (stream_ptr)->read_cnt)

/**
 * Set the stream's read/write timeout value.
 * @param stream_ptr {ACL_VSTREAM*) Stream pointer
 * @param _rw_timeo {int} Timeout value (default unit is seconds)
 */
#define	ACL_VSTREAM_SET_RWTIMO(stream_ptr, _rw_timeo) do {    \
        ACL_VSTREAM *__stream_ptr  = stream_ptr;              \
        __stream_ptr->rw_timeout = _rw_timeo;                 \
} while (0)

/**
 * Set the stream to EOF state.
 * @param stream_ptr {ACL_VSTREAM*) Stream pointer
 */
#define	ACL_VSTREAM_SET_EOF(stream_ptr) do {                  \
        ACL_VSTREAM *__stream_ptr = stream_ptr;               \
        __stream_ptr->flag |= ACL_VSTREAM_FLAG_EOF;           \
} while (0)

/**
 * Check whether the stream has an error.
 * @param stream_ptr: ACL_VSTREAM Stream pointer
 * @return 0 indicates no error, non-0 indicates error
 */
#define ACL_IF_VSTREAM_ERR(stream_ptr)                        \
	((stream_ptr)->flag & ACL_VSTREAM_FLAG_BAD)

#ifdef  __cplusplus
}
#endif

/**
 * Get the system error number related to read/write operations on the stream.
 * @param stream_ptr {ACL_VSTREAM*) Stream pointer
 * @return err {int} If there is an error number, the caller can use
 *  strerror(err) to view the error content
 */
#define	ACL_VSTREAM_ERRNO(stream_ptr) ((stream_ptr)->errnum)

/**
 * Check whether a stream has timed out.
 * @param stream_ptr {ACL_VSTREAM*) Stream pointer
 * @return {int} 0: No; != 0: Yes
 */
#define	acl_vstream_ftimeout(stream_ptr) \
        ((stream_ptr)->flag & ACL_VSTREAM_FLAG_TIMEOUT)

#endif
