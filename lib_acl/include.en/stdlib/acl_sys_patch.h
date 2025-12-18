#ifndef	ACL_SYS_PATCH_INCLUDE_H
#define	ACL_SYS_PATCH_INCLUDE_H

# ifdef	__cplusplus
extern "C" {
# endif

#include "acl_define.h"
#include "acl_vstream.h"

#if defined(_WIN32) || defined(_WIN64)
struct iovec {
	void *iov_base;   /**< Starting address */
	size_t iov_len;   /**< Number of bytes */
};

#ifdef	HAVE_NO_TIMEVAL
struct timeval {
	long tv_sec;      /**< seconds */
	long tv_usec;     /**< microseconds */
};
#endif

struct timezone {
	int tz_minuteswest; /**< minutes W of Greenwich */
	int tz_dsttime;     /**< type of dst correction */
};

/**
 * Sleep function.
 * @param sec {int} Sleep seconds
 */
ACL_API void sleep(int sec);

/**
 * Get current time.
 * @param tv {struct timeval*} Storage for current time structure
 * @param tz {struct timezone*} Timezone
 */
ACL_API int gettimeofday(struct timeval *tv, struct timezone *tz);
ACL_API int gettimeofday1(struct timeval *tv, struct timezone *tz);
ACL_API int gettimeofday2(struct timeval *tv, struct timezone *tz);
ACL_API int gettimeofday3(struct timeval *tv, struct timezone *tz);
ACL_API int gettimeofday4(struct timeval *tv, struct timezone *tz);

#endif  /* _WIN32 */
#ifdef	ACL_UNIX
# include <sys/uio.h>
#endif

/**
 * Socket initialization function. On _WIN32 platform, need to call WSAStartup to initialize SOCKET.
 * On UNIX platform, need to ignore SIGPIPE signal via signal(SIGPIPE, SIG_IGN).
 * @return {int} 0: OK; -1: error
 */
ACL_API int acl_socket_init(void);

/**
 * Before program exits, call this function to free all socket resources. Only effective on _WIN32.
 * @return {int} 0: OK; -1: error
 */
ACL_API int acl_socket_end(void);

/**
 * Close socket handle.
 * @param fd {ACL_SOCKET} Socket handle
 * @return {int} 0: OK; -1: error
 */
ACL_API int acl_socket_close(ACL_SOCKET fd);

/**
 * Shutdown socket's read/write operations.
 * @param fd {ACL_SOCKET} Socket handle
 * @param how {int}
 * @return {int} Return 0 indicates operation succeeded, otherwise indicates error
 */
#if defined(_WIN32) || defined(_WIN64)
# ifndef SHUT_RD
#  define SHUT_RD SD_RECEIVE
# endif
# ifndef SHUT_WR
#  define SHUT_WR SD_SEND
# endif
# ifndef SHUT_RDWR
#  define SHUT_RDWR SD_BOTH
# endif
#endif
ACL_API int acl_socket_shutdown(ACL_SOCKET fd, int how);

/**
 * Read data from socket handle.
 * @param fd {ACL_SOCKET} Connected socket handle
 * @param buf {void*} Memory buffer address
 * @param size {size_t} buf buffer's size
 * @param timeout {int} Read/write timeout time (default unit is seconds, when ACL_VSTREAM_IS_MS()
 *  unit is milliseconds), when value >= 0, will check read/write timeout, < 0 will not check.
 * @param fp {ACL_VSTREAM*} Stream object, can be NULL
 * @param arg {void*} User-provided parameter, used in callback mode
 * @return {int} 0: OK; -1: error
 */
ACL_API int acl_socket_read(ACL_SOCKET fd, void *buf, size_t size,
	int timeout, ACL_VSTREAM *fp, void *arg);

/**
 * Write data to socket handle.
 * @param fd {ACL_SOCKET} Connected socket handle
 * @param buf {void*} Data address
 * @param size {size_t} buf data size
 * @param timeout {int} Read/write timeout time (default unit is seconds, when ACL_VSTREAM_IS_MS()
 *  unit is milliseconds), when value >= 0, will check read/write timeout, < 0 will not check.
 * @param fp {ACL_VSTREAM*} Stream object, can be NULL
 * @param arg {void*} User-provided parameter, used in callback mode
 * @return {int} 0: OK; -1: error
 */
ACL_API int acl_socket_write(ACL_SOCKET fd, const void *buf,
	size_t size, int timeout, ACL_VSTREAM *fp, void *arg);

/**
 * Write data to socket handle.
 * @param fd {ACL_SOCKET} Connected socket handle
 * @param vec {const struct iovec*} Data buffer array address
 * @param count {int} vec array length
 * @param timeout {int} Read/write timeout time (default unit is seconds, when ACL_VSTREAM_IS_MS()
 *  unit is milliseconds), when value >= 0, will check read/write timeout, < 0 will not check.
 * @param fp {ACL_VSTREAM*} Stream object, can be NULL
 * @param arg {void*} User-provided parameter, used in callback mode
 * @return {int} 0: OK; -1: error
 */
ACL_API int acl_socket_writev(ACL_SOCKET fd, const struct iovec *vec,
	int count, int timeout, ACL_VSTREAM *fp, void *arg);

/**
 * Check whether socket handle is alive.
 * @param fd {ACL_SOCKET}
 * @return {int} Return value 1 indicates socket is alive, 0 indicates exception
 */
ACL_API	int acl_socket_alive(ACL_SOCKET fd);
ACL_API	int acl_socket_alive2(ACL_SOCKET fd, double *tc1, double *tc2);

/**
 * Open file handle.
 * @param filepath {cosnt char*} File path
 * @param flags {int} Open flag bits, O_RDONLY | O_WRONLY | O_RDWR, 
 *  O_CREAT | O_EXCL | O_TRUNC, O_APPEND(for UNIX)
 * @param mode {int} Permission bits, only effective on UNIX, e.g., 0700, 0755
 * @return {ACL_FILE_HANDLE} Opened file handle, returns ACL_FILE_INVALID on failure
 */
ACL_API ACL_FILE_HANDLE acl_file_open(const char *filepath, int flags, int mode);

/**
 * Close opened file handle.
 * @param fh {ACL_FILE_HANDLE} File handle
 * @return {int} 0: ok; -1: error
 */
ACL_API int acl_file_close(ACL_FILE_HANDLE fh);

/**
 * Seek file position.
 * @param fh {ACL_FILE_HANDLE} File handle
 * @param offset {acl_off_t} Offset position
 * @param whence {int} Position flag bits: SEEK_CUR, SEEK_SET, SEEK_END
 * @return {acl_off_t} Current file offset position
 */
ACL_API acl_off_t acl_lseek(ACL_FILE_HANDLE fh, acl_off_t offset, int whence);

/**
 * Read data from file handle.
 * @param fh {ACL_FILE_HANDLE} File handle
 * @param buf {void*} Storage buffer
 * @param size {size_t} buf buffer's size
 * @param timeout {int} This parameter is not used
 * @param fp {ACL_VSTREAM*} Corresponding file stream object, can be NULL
 * @param arg {void*} User-provided parameter, this parameter is effective when used in callback mode
 * @return {int} Actually read bytes, on error returns ACL_VSTREAM_EOF to indicate read error
 */
ACL_API int acl_file_read(ACL_FILE_HANDLE fh, void *buf, size_t size,
	int timeout, ACL_VSTREAM *fp, void *arg);

/**
 * Write data to file handle.
 * @param fh {ACL_FILE_HANDLE} File handle
 * @param buf {void*} Data storage buffer
 * @param size {size_t} buf buffer's data length size
 * @param timeout {int} This parameter is not used
 * @param fp {ACL_VSTREAM*} Corresponding file stream object, can be NULL
 * @param arg {void*} User-provided parameter, this parameter is effective when used in callback mode
 * @return {int} Successfully written bytes, on error returns ACL_VSTREAM_EOF to indicate write error
 */
ACL_API int acl_file_write(ACL_FILE_HANDLE fh, const void *buf, size_t size,
	int timeout, ACL_VSTREAM *fp, void *arg);

/**
 * Write a buffer array to file handle.
 * @param fh {ACL_FILE_HANDLE} File handle
 * @param vec {const struct iovec*} Data storage array
 * @param count {int} vec array's element count
 * @param timeout {int} This parameter is not used
 * @param fp {ACL_VSTREAM*} Corresponding file stream object, can be NULL
 * @param arg {void*} User-provided parameter, this parameter is effective when used in callback mode
 * @return {int} Successfully written bytes, on error returns ACL_VSTREAM_EOF to indicate write error
 */
ACL_API int acl_file_writev(ACL_FILE_HANDLE fh, const struct iovec *vec,
	int count, int timeout, ACL_VSTREAM *fp, void *arg);

/**
 * Flush all data in file handle's buffer to disk.
 * @param fh {ACL_FILE_HANDLE} File handle
 * @param fp {ACL_VSTREAM*} Corresponding file stream object, can be NULL
 * @param arg {void*} User-provided parameter, this parameter is effective when used in callback mode
 * @return {int} 0: ok; -1: error
 */
ACL_API int acl_file_fflush(ACL_FILE_HANDLE fh, ACL_VSTREAM *fp, void *arg);

/**
 * Get file's size by file name.
 * @param filename {const char*} File name
 * @return {acl_int64} >= 0: ok;  -1: error
 */
ACL_API acl_int64 acl_file_size(const char *filename);

/**
 * Get file's size by file handle.
 * @param fh {ACL_FILE_HANDLE} File handle
 * @param fp {ACL_VSTREAM*} Corresponding file stream object, can be NULL
 * @param arg {void*} User-provided parameter, this parameter is effective when used in callback mode
 * @return {acl_int64} >= 0: ok;  -1: error
 */
ACL_API acl_int64 acl_file_fsize(ACL_FILE_HANDLE fh, ACL_VSTREAM *fp, void *arg);

/**
 * Create SOCKET pair.
 * @param domain {int}
 * @param type {int}
 * @param protocol {int}
 * @param result {ACL_SOCKET [2]} Storage buffer
 * @return {int} Success returns 0, failure returns -1
 */
ACL_API int acl_sane_socketpair(int domain, int type, int protocol,
		ACL_SOCKET result[2]);

/* in acl_sys_socket.c */

#if defined(_WIN32) || defined(_WIN64)
typedef int (WINAPI *acl_close_socket_fn)(ACL_SOCKET);
typedef int (WINAPI *acl_recv_fn)(ACL_SOCKET, char *, int, int);
typedef int (WINAPI *acl_send_fn)(ACL_SOCKET, const char *, int, int);
#else
typedef int (*acl_close_socket_fn)(ACL_SOCKET);
typedef ssize_t  (*acl_read_fn)(ACL_SOCKET, void *, size_t);
typedef ssize_t  (*acl_recv_fn)(ACL_SOCKET, void *, size_t, int);
typedef ssize_t  (*acl_write_fn)(ACL_SOCKET, const void *, size_t);
typedef ssize_t  (*acl_writev_fn)(ACL_SOCKET, const struct iovec *, int);
typedef ssize_t  (*acl_send_fn)(ACL_SOCKET, const void *, size_t, int);
#endif

#if !defined(_WIN32) && !defined(_WIN64)
ACL_API void acl_set_read(acl_read_fn fn);
ACL_API void acl_set_write(acl_write_fn fn);
ACL_API void acl_set_writev(acl_writev_fn fn);
#endif

ACL_API void acl_set_close_socket(acl_close_socket_fn fn);
ACL_API void acl_set_recv(acl_recv_fn fn);
ACL_API void acl_set_send(acl_send_fn fn);

# ifdef	__cplusplus
}
# endif

#endif
