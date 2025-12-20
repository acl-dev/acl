#ifndef	ACL_MYSTRING_INCLUDE_H
#define	ACL_MYSTRING_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "acl_define.h"

#include <string.h>

/**
 * Safe string copy macro that always terminates the destination with '\0'.
 * @param _dst {char*} Destination buffer
 * @param _src {const char*} Source string
 * @param _size {int} Size of the destination buffer
 */
#ifndef ACL_SAFE_STRNCPY
#define ACL_SAFE_STRNCPY(_dst, _src, _size) do {                              \
    if ((_size) > 0) {                                                        \
        size_t _n = strlen((_src));                                           \
        _n = _n > (size_t ) (_size) - 1? (size_t) (_size) - 1 : _n;           \
        memcpy(_dst, _src, _n);                                               \
        (_dst)[_n] = 0;                                                       \
    }                                                                         \
} while (0)
#endif

#ifndef ACL_SAFE_STRCPY
#define ACL_SAFE_STRCPY(_dst, _src) do {                                      \
	size_t _n = strlen((_src));                                           \
	_n = _n > sizeof((_dst)) - 1 ? sizeof((_dst)) -1 : _n;                \
	memcpy((_dst), (_src), _n);                                           \
	(_dst)[_n] = '\0';                                                    \
} while (0)
#endif

/**
 * Convert a string to lower case in place.
 * @param s {char *} Input string
 * @return {char*} Pointer to the converted string on success, or NULL on error
 */
ACL_API char *acl_lowercase(char *s);

/**
 * Convert the first n bytes of a string to lower case in place.
 * @param s {char *} Input string
 * @param n {int} Number of bytes to convert
 * @return {char*} Pointer to the converted string on success, or NULL on error
 */
ACL_API char *acl_lowercase2(char *s, size_t n);

/**
 * Convert a string to lower case and store the result in a separate buffer.
 * @param s {const char*} Source string
 * @param buf {char*} Destination buffer for the converted string
 * @param size {size_t} Size of the destination buffer
 * @return {char*} Pointer to the converted string on success, or NULL on error
 */
ACL_API char *acl_lowercase3(const char *s, char *buf, size_t size);

/**
 * Convert a string to upper case in place.
 * @param s {char *} Input string
 * @return {char*} Pointer to the converted string on success, or NULL on error
 */
ACL_API char *acl_uppercase(char *s);

/**
 * Convert a string to upper case in place, but only up to the specified length.
 * @param s {char *} Input string
 * @param n {int} Number of bytes to convert
 * @return {char*} Pointer to the converted string on success, or NULL on error
 */
ACL_API char *acl_uppercase2(char *s, size_t n);

/**
 * Convert the first n bytes of a string to upper case and store in a buffer.
 * @param s {char *} Source string
 * @param buf {char*} Destination buffer for the converted string
 * @param size {size_t} Size of the destination buffer (in bytes)
 * @return {char*} Pointer to the converted string on success, or NULL on error
 */
ACL_API char *acl_uppercase3(const char *s, char *buf, size_t size);

/**
 * Split a string by a multi-character separator.
 * @param src {char**} Pointer to the current position in the source string;
 *  must not be NULL. When the string has been fully consumed, this function
 *  will set *src to NULL.
 * @param sep {const char*} Non-empty separator string
 * @return {char*} Pointer to the current token, and *src is advanced to the start
 *         of the next token.
 *  1) When the return value is NULL, splitting is finished and *src points to '\0'.
 *  2) When the return value is non-NULL, *src still points to a non-empty string.
 *     On the next call, if no more separators are found, NULL will be returned;
 *     otherwise another non-NULL token pointer will be returned.
 *  Example: For source string "abcd=|efg=|hijk" and separator "=|", after the
 *  first split, *src points to "efg" and the return value points to "abcd".
 */
ACL_API char *acl_strtok(char **src, const char *sep);
#define acl_mystrtok	acl_strtok

/**
 * Extract one logical line: if a line ends with the escape character '\\', the
 * next physical line will be merged into the same logical line. Any trailing
 * line terminators ("\r\n" or "\n") are stripped.
 * @param src {char**} Pointer to the current position in the source string
 * @return {char*} Pointer to the next logical line, or NULL if no more lines exist
 */
ACL_API char *acl_strline(char **src);
#define acl_mystrline	acl_strline

/**
 * Trim leading and trailing spaces and tabs from a string.
 * @param str {char*} Source string
 * @return {char*} The same pointer as the source string
 */
ACL_API char *acl_strtrim(char *str);
#define acl_mystr_trim	acl_strtrim

/**
 * Remove all occurrences of a substring from the source string.
 * @param haystack {const char*} Source string
 * @param needle {const char*} Substring to be removed from the source string
 * @param buf {char*} Destination buffer for the result
 * @param bsize {int} Size of the destination buffer
 * @return {int} Length of the resulting string stored in buf
 */
ACL_API int acl_strstrip(const char *haystack, const char *needle,
		char *buf, int bsize);
#define acl_mystr_strip	acl_strstrip

/**
 * Locate the end of the first line in the string and truncate there,
 * removing any trailing carriage return or newline characters.
 * @param str {char*} Source string
 * @return {int} 0 on success; -1 on failure (for example, if no line terminator was found)
 */
ACL_API int acl_strtrunc_byln(char *str);
#define acl_mystr_truncate_byln	acl_strtrunc_byln

/**
 * Compare two strings from the end with case ignored, within a limited length.
 * @param s1 {const char*} First string
 * @param s2 {const char*} Second string
 * @param n {size_t} Maximum number of characters to compare
 * @return {int} Comparison result: 0 if equal; > 0 if s1 > s2; < 0 if s1 < s2
 */
ACL_API int acl_strrncasecmp(const char *s1, const char *s2, size_t n);

/**
 * Compare two strings from the end with case sensitivity, within a limited length.
 * @param s1 {const char*} First string
 * @param s2 {const char*} Second string
 * @param n {size_t} Maximum number of characters to compare
 * @return {int} Comparison result: 0 if equal; > 0 if s1 > s2; < 0 if s1 < s2
 */
ACL_API int acl_strrncmp(const char *s1, const char *s2, size_t n);

/**
 * Search for the last occurrence of a substring in a string (case sensitive).
 * @param haystack {char *} Source string
 * @param needle {const char *} Substring to search for
 * @return {char *} Non-NULL if found (pointer to the match), or NULL if not found
 */
ACL_API char *acl_rstrstr(const char *haystack, const char *needle);

/**
 * Search forward for a substring in a string, ignoring case.
 * @param haystack {const char *} Source string
 * @param needle {const char *} Substring to search for
 * @return {char *} Non-NULL if found (pointer to the match), or NULL if not found
 */
ACL_API char *acl_strcasestr(const char *haystack, const char *needle);

/**
 * Search backward for a substring in a string, ignoring case.
 * @param haystack {char *} Source string
 * @param needle {const char *} Substring to search for
 * @return {char *} Non-NULL if found (pointer to the match), or NULL if not found
 */
ACL_API char *acl_rstrcasestr(const char *haystack, const char *needle);

/**
 * Return the length of a string, but check at most count characters.
 * This is safer than strlen: if the string is not terminated by '\0' within
 * the given range, the function will not read past count bytes.
 * @param s {const char*} String
 * @param count {size_t} Maximum number of characters to examine
 * @return {size_t} Actual length of s within the given bound
 */
ACL_API size_t acl_strnlen(const char * s, size_t count);

/**
 * Compare two strings for equality, ignoring case.
 * @param s1 {const char*}
 * @param s2 {cosnt char*}
 * @return {int} 0: equal; < 0: s1 < s2; > 0: s1 > s2
 */
ACL_API int acl_strcasecmp(const char *s1, const char *s2);

/**
 * Compare two strings for equality, ignoring case, with a maximum length.
 * @param s1 {const char*}
 * @param s2 {cosnt char*}
 * @param n {size_t} Maximum number of characters to compare
 * @return {int} 0: equal; < 0: s1 < s2; > 0: s1 > s2
 */
ACL_API int acl_strncasecmp(const char *s1, const char *s2, size_t n);
/**
 * On Windows, some string comparison functions are not provided by the CRT,
 * so we map them to the ACL implementations here.
 */
#if defined(_WIN32) || defined(_WIN64)
# ifndef strcasestr
#  define strcasestr	acl_strcasestr
# endif
# ifndef strcasecmp
#  define strcasecmp	acl_strcasecmp
# endif
# ifndef strncasecmp
#  define strncasecmp	acl_strncasecmp
# endif
#endif

#ifndef strrncasecmp
# define strrncasecmp	acl_strrncasecmp
#endif
#ifndef strrncmp
# define strrncmp	acl_strrncmp
#endif

/*----------------------------------------------------------------------------
 * Normalize a file path, ensuring it is in the form:
 * /home/avwall/test.txt
 * @param psrc_file_path {const char*} Source path string
 * @param pbuf {char*} Destination buffer for the normalized path
 * @param sizeb {int} Size of the destination buffer
 * @return {int} 0 on success; -1 on failure
 */
ACL_API int acl_file_path_correct(const char *psrc_file_path,
		char *pbuf, int sizeb);

/*----------------------------------------------------------------------------
 * Normalize a directory path so that the result is of the form:
 * Source examples:
 *   /home/avwall/, /home//////avwall/, /home/avwall, /////home/avwall///
 *   /home/avwall////, /home///avwall///, ///home///avwall///
 * Result:
 *   /home/avwall/
 * @param psrc_dir {const char*} Source directory string
 * @param pbuf {char*} Destination buffer for the normalized directory
 * @param sizeb {int} Size of the destination buffer
 * @return {int} 0 on success; -1 on failure
 */
ACL_API int acl_dir_correct(const char *psrc_dir, char *pbuf, int sizeb);

/*----------------------------------------------------------------------------
 * Given a full file path such as: /home/avwall/log.txt
 * extract its directory part: /home/avwall/
 * @param pathname {const char*} Source path string
 * @param pbuf {char*} Destination buffer for the directory part
 * @param bsize {int} Size of the destination buffer
 * @return {int} 0 on success; -1 on failure
 */
ACL_API int acl_dir_getpath(const char *pathname, char *pbuf, int bsize);

/**
 * Safely convert a string to an integer; when conversion fails, a default
 * value is returned so that the caller can avoid dealing with errno.
 * Internally this uses strtol/strtoll but wraps them to provide a safer API.
 *
 * Convert a string to a 32-bit integer.
 * @param s {const char*} String to convert
 * @param def {int} Default value to return on failure
 * @return {int}
 */
ACL_API int acl_safe_atoi(const char *s, int def);

/**
 * Convert a string to a long integer.
 * @param s {const char*} String to convert
 * @param def {long} Default value to return on failure
 * @return {long}
 */
ACL_API long acl_safe_atol(const char *s, long def);

/**
 * Convert a string to a 64-bit signed integer.
 * @param s {const char*} String to convert
 * @param def {long long} Default value to return on failure
 * @return {long long}
 */
ACL_API long long acl_safe_atoll(const char *s, long long def);

/**
 * Convert a string to a 64-bit signed integer (no default value).
 * @param s {const char*} String to convert
 * @return {long long} Converted signed integer value
 */
ACL_API long long acl_atoll(const char *s);

/**
 * Convert a string to a 64-bit unsigned integer.
 * @param str {const char*} String to convert
 * @return {acl_uint64} Converted unsigned integer value
 */
ACL_API acl_uint64 acl_atoui64(const char *str);

/**
 * Convert a string to a 64-bit signed integer.
 * @param str {const char*} String to convert
 * @return {acl_int64} Converted signed integer value
*/
ACL_API acl_int64 acl_atoi64(const char *str);

/**
 * Convert a 64-bit unsigned integer to a decimal string.
 * @param value {acl_uint64} 64-bit unsigned integer
 * @param buf {char*} Buffer to receive the converted string
 * @param size {sizt_t} Size of the buffer; should be at least 21 bytes
 * @return {const char*} Pointer to the converted string on success; NULL on failure
 */
ACL_API const char *acl_ui64toa(acl_uint64 value, char *buf, size_t size);

/**
 * Convert a 64-bit signed integer to a decimal string.
 * @param value {acl_int64} 64-bit signed integer
 * @param buf {char*} Buffer to receive the converted string
 * @param size {sizt_t} Size of the buffer; should be at least 21 bytes
 * @return {const char*} Pointer to the converted string on success; NULL on failure
 */
ACL_API const char *acl_i64toa(acl_int64 value, char *buf, size_t size);

/**
 * Convert a 64-bit signed integer to a string in the given radix.
 * @param value {acl_int64} 64-bit signed integer
 * @param buf {char*} Buffer to receive the converted string
 * @param size {sizt_t} Size of the buffer; should be at least 21 bytes
 * @param radix {int} Radix, e.g. 8 for octal, 10 for decimal, 16 for hexadecimal
 * @return {const char*} Pointer to the converted string on success; NULL on failure
 */
ACL_API const char *acl_i64toa_radix(acl_int64 value, char *buf,
		size_t size, int radix);

/**
 * Convert a 64-bit unsigned integer to a string in the given radix.
 * @param value {acl_int64} 64-bit unsigned integer
 * @param buf {char*} Buffer to receive the converted string
 * @param size {sizt_t} Size of the buffer; should be at least 21 bytes
 * @param radix {int} Radix, e.g. 8 for octal, 10 for decimal, 16 for hexadecimal
 * @return {const char*} Pointer to the converted string on success; NULL on failure
 */
ACL_API const char *acl_ui64toa_radix(acl_uint64 value, char *buf,
		size_t size, int radix);

/*--------------------------------------------------------------------------*/

typedef struct ACL_LINE_STATE {
	int   offset;		/* Current offset in the inspected buffer */
	char  finish;		/* Whether a complete line has been found */
	char  last_ch;		/* The last character inspected */
	char  last_lf;		/* Whether the last character was a line feed (LF) */
} ACL_LINE_STATE;

/**
 * Allocate an ACL_LINE_STATE object used by acl_find_blank_line.
 * @return {ACL_LINE_STATE*} Always returns a non-NULL pointer on success
 */
ACL_API ACL_LINE_STATE *acl_line_state_alloc(void);

/**
 * Free an ACL_LINE_STATE object allocated by acl_line_state_alloc.
 * @param state {ACL_LINE_STATE*} Non-NULL ACL_LINE_STATE object
 */
ACL_API void acl_line_state_free(ACL_LINE_STATE *state);

/**
 * Reset the state of an ACL_LINE_STATE object.
 * @param state {ACL_LINE_STATE*} Object allocated by acl_line_state_alloc.
 *        Do not pass objects allocated on the stack or by raw malloc/free,
 *        as only objects from acl_line_state_alloc may be freed by acl_line_state_free.
 * @param offset {int} Initial value for the offset field
 * @return {ACL_LINE_STATE*} The same pointer as state
 */
ACL_API ACL_LINE_STATE *acl_line_state_reset(ACL_LINE_STATE *state, int offset);

/**
 * Scan a data buffer to find a blank line (line terminators can be "\r\n" or "\n").
 * This function supports incremental parsing by being called repeatedly.
 * @param s {const char*} Data buffer to scan
 * @param n {int} Length of the data buffer
 * @param state {ACL_LINE_STATE*} State object allocated by acl_line_state_alloc
 * @return {int} Number of remaining bytes after the blank line
 */
ACL_API int acl_find_blank_line(const char *s, int n, ACL_LINE_STATE *state);

/*--------------------------------------------------------------------------*/

#ifdef  __cplusplus
}
#endif

#endif
