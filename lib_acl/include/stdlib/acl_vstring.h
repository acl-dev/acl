#ifndef ACL_VSTRING_INCLUDE_H
#define ACL_VSTRING_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include <stdarg.h>
#include "acl_vbuf.h"

/**
 * Wrapper for ACL_VBUF, ACL_VSTRING structure type definition.
 */
typedef struct ACL_VSTRING {
	ACL_VBUF vbuf;
	ssize_t  maxlen;
} ACL_VSTRING;

/**
 * Initialize ACL_VSTRING structure pointer with default buffer size.
 * User can use their own function.
 * When using ACL_VSTRING str format (static allocation), need to initialize
 * with this function, additionally, need to call acl_vstring_free_buf format
 * to free internal buffer allocated by this function.
 * @param vp {ACL_VSTRING*} String buffer, must not be NULL
 * @param len {size_t} Initial buffer size
 */
ACL_API void acl_vstring_init(ACL_VSTRING *vp, size_t len);

/**
 * When ACL_VSTRING object is initialized via acl_vstring_init, need to call
 * this function to free buffer memory.
 * @param vp {ACL_VSTRING*} String buffer, must not be NULL
 */
ACL_API void acl_vstring_free_buf(ACL_VSTRING *vp);

/**
 * Dynamically allocate an ACL_VSTRING object, specify internal buffer's initial size.
 * @param len {size_t} Initial buffer size
 * @return {ACL_VSTRING*} Newly allocated ACL_VSTRING object
 */
ACL_API ACL_VSTRING *acl_vstring_alloc(size_t len);

/**
 * Dynamically allocate an ACL_VSTRING object, specify internal buffer's initial size.
 * Simultaneously specify memory pool allocator or memory pool.
 * @param slice {ACL_SLICE_POOL*} Slice memory pool object pointer
 * @param len {size_t} Initial buffer size
 * @return {ACL_VSTRING*} Newly allocated ACL_VSTRING object
 */
ACL_API ACL_VSTRING *acl_vstring_slice_alloc(ACL_SLICE_POOL *slice, size_t len);

/**
 * Dynamically allocate an ACL_VSTRING object, specify internal buffer's initial size.
 * Simultaneously specify memory pool allocator or memory pool.
 * @param len {size_t} Initial buffer size
 * @return {ACL_VSTRING*} Newly allocated ACL_VSTRING object
 */
ACL_API ACL_VSTRING *acl_vstring_dbuf_alloc(ACL_DBUF_POOL *dbuf, size_t len);

/**
 * When using memory-mapped file format to allocate memory, use this function to
 * create ACL_VSTRING dynamic object.
 * @param fd {ACL_FILE_HANDLE} Valid file handle
 * @param max_len {size_t} Maximum mapped memory size
 * @param init_len {size_t} Initial memory mapped size
 * @return {ACL_VSTRING*} Newly created ACL_VSTRING object
 */
ACL_API ACL_VSTRING *acl_vstring_mmap_alloc(ACL_FILE_HANDLE fd,
	size_t max_len, size_t init_len);

/**
 * When using memory-mapped file format to allocate memory, use this function to
 * create ACL_VSTRING dynamic object.
 * @param fd {ACL_FILE_HANDLE} Valid file handle
 * @param max_len {size_t} Maximum mapped memory size
 * @param init_len {size_t} Initial memory mapped size
 * @param offset {ssize_t} Start position in file for mapping
 * @return {ACL_VSTRING*} Newly created ACL_VSTRING object
 */
ACL_API ACL_VSTRING *acl_vstring_mmap_alloc2(ACL_FILE_HANDLE fd,
	size_t max_len, size_t init_len, size_t offset);

/**
 * Control ACL_VSTRING object's parameters, currently this function's
 * functionality is limited.
 * @param vp {ACL_VSTRING*}
 * @param ... Control parameters represented by ACL_VSTRING_CTL_XXX, end flag is
 *  ACL_VSTRING_CTL_END
 */
ACL_API void acl_vstring_ctl(ACL_VSTRING *vp,...);

#define ACL_VSTRING_CTL_MAXLEN      1
#define ACL_VSTRING_CTL_END         0

/**
 * Truncate data in buffer to specified length, simultaneously ensure buffer
 * ends with '\0'.
 * @param vp {ACL_VSTRING*}
 * @param len {size_t} Length after truncation
 * @return {ACL_VSTRING*} Same as vp
 */
ACL_API ACL_VSTRING *acl_vstring_truncate(ACL_VSTRING *vp, size_t len);

/**
 * Free ACL_VSTRING object dynamically allocated via acl_vstring_alloc.
 * @param vp {ACL_VSTRING*}
 */
ACL_API void acl_vstring_free(ACL_VSTRING *vp);

/**
 * Copy string.
 * @param vp {ACL_VSTRING*}
 * @param src {const char*} Source string
 * @return {ACL_VSTRING*} Same as vp
 */
ACL_API ACL_VSTRING *acl_vstring_strcpy(ACL_VSTRING *vp, const char *src);

/**
 * Copy string, with maximum length limit.
 * @param vp {ACL_VSTRING*}
 * @param src {const char*} Source string
 * @param len {size_t} Maximum length limit
 * @return {ACL_VSTRING*} Same as vp
 */
ACL_API ACL_VSTRING *acl_vstring_strncpy(ACL_VSTRING *vp,
		const char *src, size_t len);

/**
 * Append string.
 * @param vp {ACL_VSTRING*}
 * @param src {const char*} Source string
 * @return {ACL_VSTRING*} Same as vp
 */
ACL_API ACL_VSTRING *acl_vstring_strcat(ACL_VSTRING *vp, const char *src);

/**
 * Append string, with maximum length limit.
 * @param vp {ACL_VSTRING*}
 * @param src {const char*} Source string
 * @param len {size_t} Maximum length limit
 * @return {ACL_VSTRING*} Same as vp
 */
ACL_API ACL_VSTRING *acl_vstring_strncat(ACL_VSTRING *vp,
		const char *src, size_t len);

/**
 * Copy memory data, simultaneously ensure target buffer ends with '\0'.
 * @param vp {ACL_VSTRING*}
 * @param src {const char*} Source data address
 * @param len {size_t} Source data length
 * @return {ACL_VSTRING*} Same as vp
 */
ACL_API ACL_VSTRING *acl_vstring_memcpy(ACL_VSTRING *vp,
		const char *src, size_t len);

/**
 * Move memory data, source and target addresses can be in the same memory area
 * or different memory areas.
 * memory areas. This function ensures target address ends with '\0'.
 * @param vp {ACL_VSTRING*}
 * @param src {const char*} Source data address
 * @param len {size_t} Source data length
 * @return {ACL_VSTRING*} Same as vp
 */
ACL_API ACL_VSTRING *acl_vstring_memmove(ACL_VSTRING *vp,
		const char *src, size_t len);

/**
 * Append memory data, this function ensures target buffer ends with '\0'.
 * @param vp {ACL_VSTRING*}
 * @param src {const char*} Source data address
 * @param len {size_t} Source data length
 * @return {ACL_VSTRING*} Same as vp
 */
ACL_API ACL_VSTRING *acl_vstring_memcat(ACL_VSTRING *vp,
		const char *src, size_t len);

/**
 * Search for a certain character.
 * @param vp {ACL_VSTRING*}
 * @param ch {int} Character to search for
 * @return {char*} Address of target character's position, if not found returns NULL,
 *  Note: The returned address cannot be modified or freed, as it is managed by
 *  ACL_VSTRING object
 */
ACL_API char *acl_vstring_memchr(ACL_VSTRING *vp, int ch);

/**
 * Search for a certain string, string comparison is case-sensitive.
 * @param vp {ACL_VSTRING*}
 * @param needle {const char*} String to search for
 * @return {char*} Address of target string's position, if not found returns NULL,
 *  Note: The returned address cannot be modified or freed, as it is managed by
 *  ACL_VSTRING object
 */
ACL_API char *acl_vstring_strstr(ACL_VSTRING *vp, const char *needle);

/**
 * Search for a certain string, string comparison is case-insensitive.
 * @param vp {ACL_VSTRING*}
 * @param needle {const char*} String to search for
 * @return {char*} Address of target string's position, if not found returns NULL,
 *  Note: The returned address cannot be modified or freed, as it is managed by
 *  ACL_VSTRING object
 */
ACL_API char *acl_vstring_strcasestr(ACL_VSTRING *vp, const char *needle);

/**
 * Search string from back to front, string comparison is case-sensitive.
 * @param vp {ACL_VSTRING*}
 * @param needle {const char*} String to search for
 * @return {char*} Address of target string's position, if not found returns NULL,
 *  Note: The returned address cannot be modified or freed, as it is managed by
 *  ACL_VSTRING object
 */
ACL_API char *acl_vstring_rstrstr(ACL_VSTRING *vp, const char *needle);

/**
 * Search string from back to front, string comparison is case-insensitive.
 * @param vp {ACL_VSTRING*}
 * @param needle {const char*} String to search for
 * @return {char*} Address of target string's position, if not found returns NULL,
 *  Note: The returned address cannot be modified or freed, as it is managed by
 *  ACL_VSTRING object
 */
ACL_API char *acl_vstring_rstrcasestr(ACL_VSTRING *vp, const char *needle);

/**
 * Insert data at a certain specified position in buffer, simultaneously ensure
 * target buffer ends with '\0'.
 * @param vp {ACL_VSTRING*}
 * @param start {size_t} Specified position
 * @param buf {const char*} Data address
 * @param len {size_t} Data length
 * @return {ACL_VSTRING*} Same as vp
 */
ACL_API ACL_VSTRING *acl_vstring_insert(ACL_VSTRING *vp, size_t start,
		const char *buf, size_t len);

/**
 * Prepend data at buffer head, simultaneously ensure target buffer ends with '\0'.
 * @param vp {ACL_VSTRING*}
 * @param buf {const char*} Data address
 * @param len {size_t} Data length
 * @return {ACL_VSTRING*} Same as vp
 */
ACL_API ACL_VSTRING *acl_vstring_prepend(ACL_VSTRING *vp,
		const char *buf, size_t len);

/**
 * Append formatted data to buffer.
 * @param vp {ACL_VSTRING*}
 * @param format {const char*} Format string
 * @param ... Variable arguments
 * @return {ACL_VSTRING*} Same as vp
 */
ACL_API ACL_VSTRING *ACL_PRINTF(2, 3) acl_vstring_sprintf(ACL_VSTRING *vp,
		const char *format,...);

/**
 * Append formatted data to buffer in append mode.
 * @param vp {ACL_VSTRING*}
 * @param format {const char*} Format string
 * @param ... Variable arguments
 * @return {ACL_VSTRING*} Same as vp
 */
ACL_API ACL_VSTRING *ACL_PRINTF(2, 3) acl_vstring_sprintf_append(
		ACL_VSTRING *vp, const char *format,...);

/**
 * Export data in buffer and simultaneously free ACL_VSTRING object. User needs to use
 * acl_myfree to free returned data memory.
 * @param vp {ACL_VSTRING*}
 * @return {char*} Data string address, when return value is not NULL, user needs
 *  to use acl_myfree to free this address, otherwise memory leak will occur
 *  acl_myfree to free this address, otherwise memory leak will occur
 */
ACL_API char *acl_vstring_export(ACL_VSTRING *vp);

/**
 * Import user's dynamically allocated storage string memory and create new ACL_VSTRING object.
 * @param str {char*} External dynamically allocated storage string memory address
 * @return {ACL_VSTRING*} Newly allocated ACL_VSTRING object
 */
ACL_API ACL_VSTRING *acl_vstring_import(char *str);

/**
 * Glue static memory to ACL_VSTRING.
 * Note: vp must not be allocated via acl_vstring_alloc, and must not be
 *  initialized via acl_vstring_init, for initialization, vp must be allocated
 *  via acl_mymalloc or a stack variable
 * Example: ACL_VSTRING v; acl_vstring_glue(&v, "hello", 5);
 * @param vp {ACL_VSTRING*} Allocated via acl_mymalloc or a stack variable, when
 *  allocated via acl_mymalloc format, should be freed via acl_myfree
 * @param buf {void*} User-provided memory address, must not be stack variable
 * @param len {size_t} buf memory address length
 */
ACL_API void acl_vstring_glue(ACL_VSTRING *vp, void *buf, size_t len);

/**
 * Get character at a certain position.
 * @param vp {ACL_VSTRING*}
 * @param len {size_t} Position, if value exceeds bounds, internally calls fatal
 * @return {char} Found character
 */
ACL_API char acl_vstring_charat(ACL_VSTRING *vp, size_t len);

/**
 * Append data in specified format.
 * @param vp {ACL_VSTRING*}
 * @param format {const char*}
 * @param ap {va_list}
 * @return {ACL_VSTRING*} Same as vp
 * @see acl_vstring_sprintf
 */
ACL_API ACL_VSTRING *acl_vstring_vsprintf(ACL_VSTRING *vp,
		const char *format, va_list ap);

/**
 * Append data in specified format at end.
 * @param vp {ACL_VSTRING*}
 * @param format {const char*}
 * @param ap {va_list}
 * @return {ACL_VSTRING*} Same as vp
 */
ACL_API ACL_VSTRING *acl_vstring_vsprintf_append(ACL_VSTRING *vp,
		const char *format, va_list ap);

/**
 * Prepend data in specified format at head.
 * @param vp {ACL_VSTRING*}
 * @param format {const char*}
 * @param ... Variable arguments
 * @return {ACL_VSTRING*} Same as vp
 */
ACL_API ACL_VSTRING *ACL_PRINTF(2, 3) acl_vstring_sprintf_prepend(
		ACL_VSTRING *vp, const char *format, ...);

/**
 * Extract one line (excluding "\r\n" or "\n") from source buffer and
 * simultaneously store remaining data in buffer. If no complete line is found,
 * only stores source data.
 * @param vp {ACL_VSTRING*} String data buffer
 * @param src {const char**} Source string pointer address, after return, pointer
 *  moves to next position
 * @param dlen {size_t} Source string data length
 * @return {const ACL_VSTRING*} NULL, indicates "\r\n" or "\n" not found, will
 *  store remaining data in buffer, application should check via ACL_VSTRING_LEN
 *  whether buffer has data; !NULL indicates found complete line
 *  Note: After extracting line, should call ACL_VSTRING_RESET(vp) to reset buffer
 */
ACL_API const ACL_VSTRING *acl_buffer_gets_nonl(ACL_VSTRING *vp,
		const char **src, size_t dlen);

/**
 * Extract one line (including "\r\n" or "\n") from source buffer and simultaneously
 * store remaining data in buffer, if no complete line is found, only stores source data.
 * @param vp {ACL_VSTRING*} String data buffer
 * @param src {const char**} Source string pointer address, after return, pointer
 *  moves to next position
 * @param dlen {size_t} Source string data length
 * @return {const ACL_VSTRING*} NULL, indicates "\r\n" or "\n" not found, will store
 *  remaining data in buffer, application should check via ACL_VSTRING_LEN whether
 *  buffer has data; !NULL indicates found complete line
 *  Note: After extracting line, should call ACL_VSTRING_RESET(vp) to reset buffer
 */
ACL_API const ACL_VSTRING *acl_buffer_gets(ACL_VSTRING *vp,
		const char **src, size_t dlen);

 /*
  * Macros. Unsafe macros have UPPERCASE names.
  */
#define ACL_VSTRING_SPACE(vp, len) ACL_VBUF_SPACE(&(vp)->vbuf, len)

/**
 * Get current ACL_VSTRING data storage address.
 * @param vp {ACL_VSTRING*}
 * @return {char*}
 */
#define acl_vstring_str(vp) ((char *) (vp)->vbuf.data)

/**
 * Get current ACL_VSTRING stored data length.
 * @param vp {ACL_VSTRING*}
 * @return {int}
 */
#define ACL_VSTRING_LEN(vp) (size_t) ((vp)->vbuf.ptr - (vp)->vbuf.data)

/**
 * Get current ACL_VSTRING internal buffer's total size.
 * @param vp {ACL_VSTRING*}
 * @return {int}
 */
#define	ACL_VSTRING_SIZE(vp) ((vp)->vbuf.len)

/**
 * Get current ACL_VSTRING data offset pointer position.
 * @param vp {ACL_VSTRING*}
 * @return {char*}
 */
#define acl_vstring_end(vp) ((char *) (vp)->vbuf.ptr)

/**
 * Set ACL_VSTRING data offset pointer position to 0.
 * @param vp {ACL_VSTRING*}
 */
#define ACL_VSTRING_TERMINATE(vp) ACL_VBUF_TERM(&(vp)->vbuf)

/**
 * Reset ACL_VSTRING internal buffer to specified string start position,
 * this will not append 0 at end. Application can use macro
 * call ACL_VSTRING_TERMINATE to append 0 at end.
 * @param vp {ACL_VSTRING*}
 */
#define ACL_VSTRING_RESET(vp) {	\
	(vp)->vbuf.ptr = (vp)->vbuf.data; \
	acl_vbuf_clearerr(&(vp)->vbuf); \
}

/**
 * Append a character to ACL_VSTRING buffer.
 * @param vp {ACL_VSTRING*}
 * @param ch {int} Character
 */
#define	ACL_VSTRING_ADDCH(vp, ch) ACL_VBUF_PUT(&(vp)->vbuf, ch)

/**
 * Move data offset pointer to internal buffer end.
 * @param vp {ACL_VSTRING*}
 */
#define ACL_VSTRING_SKIP(vp) { \
	while ((vp)->vbuf.ptr < (vp)->vbuf.data + (vp)->vbuf.len && *(vp)->vbuf.ptr) { \
		(vp)->vbuf.ptr++; \
	} \
}

/**
 * Check how much data space is available in current ACL_VSTRING buffer.
 * @param vp {ACL_VSTRING*}
 */
#define acl_vstring_avail(vp) ((vp)->vbuf.len - ((vp)->vbuf.ptr - (vp)->vbuf.data))

 /**
  * The following macro is not part of the public interface, because it can
  * really screw up a buffer by positioning past allocated memory.
  */
#define ACL_VSTRING_AT_OFFSET(vp, offset) { \
	(vp)->vbuf.ptr = (vp)->vbuf.data + (offset); \
}

ACL_API int acl_vstring_space(ACL_VBUF *bp, ssize_t len);
ACL_API int acl_vstring_put_ready(ACL_VBUF *bp);

#ifdef  __cplusplus
}
#endif

#endif
