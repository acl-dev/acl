#ifndef	ACL_FILE_INCLUDE_H
#define	ACL_FILE_INCLUDE_H

#include "acl_define.h"
#include "acl_vstream.h"
#include <stdarg.h>

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * File handle type definition.
 */
typedef struct ACL_FILE {
	ACL_VSTREAM *fp;	/**< Stream pointer */
	unsigned int status;	/**< File handle status */
#define	ACL_FILE_EOF		(1 << 0)
	int   errnum;		/**< File handle's error number */
} ACL_FILE;

#define	ACL_FPATH(fp)	ACL_VSTREAM_PATH((fp)->fp)
#define	ACL_FSTREAM(fp)	((fp)->fp)

/**
 * Open or create a file handle for read/write operations.
 * @param filename {const char*} File name
 * @param mode {const char*} Open flag.
 *  r or rb: read-only mode, file must exist
 *  w or wb: write-only mode, existing file will be truncated, or create new file in write-only mode
 *  a or ab: append at end, write-only mode, existing file or create new file
 *  r+ or rb+: read-write mode, file must exist
 *  w+ or wb+: read-write mode, existing file will be truncated, or create new file
 *  a+ or ab+: append at end, read-write mode, existing file or create new file
 */
ACL_API ACL_FILE *acl_fopen(const char *filename, const char *mode);

/**
 * Close a file handle.
 * @param fp {ACL_FILE*} File handle
 */
ACL_API int acl_fclose(ACL_FILE *fp);

/**
 * Clear file handle's error flag.
 * @param fp {ACL_FILE*} File handle
 */
ACL_API void acl_clearerr(ACL_FILE *fp);

/**
 * Check whether end of file is reached.
 * @param fp {ACL_FILE*} File handle
 * @return {int} 0: no; !0: yes
 */
ACL_API int acl_feof(ACL_FILE *fp);

/**
 * Read some fixed-length data blocks from file handle.
 * @param buf {void*} Memory buffer address
 * @param size {size_t} Length of each data block
 * @param nitems {size_t} Number of data blocks
 * @param fp {ACL_FILE*} File handle
 * @return {size_t} Number of data blocks read, on error returns EOF
 */
ACL_API size_t acl_fread(void *buf, size_t size, size_t nitems, ACL_FILE *fp);

/**
 * Read a line from file handle.
 * @param buf {char*} Buffer address
 * @param size {int} buf space size
 * @param fp {ACL_FILE*} File handle
 * @return {char*} NULL: no complete line read; !NULL: complete line read
 */
ACL_API char *acl_fgets(char *buf, int size, ACL_FILE *fp);

/**
 * Read a line from file handle, returned line end does not include "\r\n".
 * @param buf {char*} Buffer address
 * @param size {int} buf space size
 * @param fp {ACL_FILE*} File handle
 * @return {char*} NULL: no complete line read; !NULL: complete line read
 */
ACL_API char *acl_fgets_nonl(char *buf, int size, ACL_FILE *fp);

/**
 * Read a character from file handle.
 * @param fp {ACL_FILE*} File handle
 * @return {int} EOF: reached end of file or error; !EOF: successfully read one character's ASCII value
 */
ACL_API int acl_fgetc(ACL_FILE *fp);
#define	acl_getc	acl_fgetc

/**
 * Read a line from standard input.
 * @param buf {char*} Buffer address
 * @param size {int} buf space size
 * @return {char*} NULL: read error; !NULL: should be same address as buf
 */
ACL_API char *acl_gets(char *buf, size_t size);

/**
 * Read a line from standard input, line end does not include "\r\n".
 * @param buf {char*} Buffer address
 * @param size {int} buf space size
 * @return {char*} NULL: read error; !NULL: should be same address as buf
 */
ACL_API char *acl_gets_nonl(char *buf, size_t size);

/**
 * Read a character from standard input.
 * @return {int} EOF: reached end of file or error; !EOF: successfully
 *  read one character's ASCII value
 */
ACL_API int acl_getchar(void);

/**
 * Write formatted data to file handle.
 * @param fp {ACL_FILE*} File handle pointer
 * @param fmt {const char*} Format string
 * @param ... Arguments
 * @return {size_t} Data length, on error returns EOF
 */
ACL_API int ACL_PRINTF(2, 3) acl_fprintf(ACL_FILE *fp, const char *fmt, ...);

/**
 * Write formatted data to file handle.
 * @param fp {ACL_FILE*} File handle pointer
 * @param fmt {const char*} Format string
 * @param ap {va_list} Argument list
 * @return {size_t} Data length, on error returns EOF
 */
ACL_API int acl_vfprintf(ACL_FILE *fp, const char *fmt, va_list ap);

/**
 * Write some fixed-length data blocks to file handle.
 * @param ptr {const void*} Data address
 * @param size {size_t} Length of each data block
 * @param nitems {size_t} Number of data blocks
 * @param fp {ACL_FILE*} File handle pointer
 * @return {size_t} Number of data blocks written, on error returns EOF
 */
ACL_API size_t acl_fwrite(const void *ptr, size_t size, size_t nitems, ACL_FILE *fp);

/**
 * Write data to file handle and automatically append "\r\n" at end.
 * @param s {const char*} String address
 * @param fp {ACL_FILE*} File handle pointer
 * @return {int} Number of bytes written (including "\r\n"), on error returns EOF
 */
ACL_API int acl_fputs(const char *s, ACL_FILE *fp);

/**
 * Write formatted data to standard output.
 * @param fmt {const char*} Format string
 * @param ... Arguments
 * @return {size_t} Data length, on error returns EOF
 */
ACL_API int ACL_PRINTF(1, 2) acl_printf(const char *fmt, ...);

/**
 * Write formatted data to standard output.
 * @param fmt {const char*} Format string
 * @param ap {va_list} Argument list
 * @return {size_t} Data length, on error returns EOF
 */
ACL_API int acl_vprintf(const char *fmt, va_list ap);

/**
 * Write a byte to file handle.
 * @param c {int} One byte's ASCII value
 * @param fp {ACL_FILE*} File handle pointer
 * @return {int} Number of bytes written, on error returns EOF
 */
ACL_API int acl_putc(int c, ACL_FILE *fp);
#define	acl_fputc	acl_putc

/**
 * Write data to standard output and automatically append "\r\n" at end.
 * @param s {const char*} String address
 * @return {int} Number of bytes written (including "\r\n"), on error returns EOF
 */
ACL_API int acl_puts(const char *s);

/**
 * Write a byte to file handle.
 * @param c {int} One byte's ASCII value
 * @return {int} Number of bytes written, on error returns EOF
 */
ACL_API int acl_putchar(int c);

/**
 * Seek file position.
 * @param fp {ACL_FILE*} File handle
 * @param offset {acl_off_t} Offset position
 * @param whence {int} Offset direction, SEEK_SET, SEEK_CUR, SEEK_END
 * @return ret {acl_off_t}, ret >= 0: correct, ret < 0: error
 */
ACL_API acl_off_t acl_fseek(ACL_FILE *fp, acl_off_t offset, int whence);

/**
 * Get current file pointer's position in file.
 * @param fp {ACL_FILE*} File handle pointer
 * @return {acl_off_t} Return value -1 indicates error
 */
ACL_API acl_off_t acl_ftell(ACL_FILE *fp);

#ifdef	__cplusplus
}
#endif

#endif
