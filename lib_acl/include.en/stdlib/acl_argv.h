#ifndef ACL_ARGV_INCLUDE_H
#define ACL_ARGV_INCLUDE_H

# ifdef	__cplusplus
extern "C" {
# endif
#include "acl_define.h"
//#include <stdarg.h>
#include "acl_dbuf_pool.h"
#include "acl_iterator.h"

#define ACL_ARGV_END	((char *) 0)

/**
 * External interface.
 */
typedef struct ACL_ARGV ACL_ARGV;

struct ACL_ARGV {
	int     len;			/**< number of array elements */
	int     argc;			/**< array elements in use */
	char  **argv;			/**< string array */

	/* Extended operations */

	/* Append a string at the end (internally duplicates the
	 * string) */
	void  (*push_back)(ACL_ARGV*, const char*);
	/* Insert a string at the beginning (internally duplicates
	 * the string) */
	void  (*push_front)(ACL_ARGV*, const char*);
	/* Remove and return the string at the end (caller must
	 * free with acl_myfree) */
	char *(*pop_back)(ACL_ARGV*);
	/* Remove and return the string at the beginning (caller
	 * must free with acl_myfree) */
	char *(*pop_front)(ACL_ARGV*);

	/* for acl_iterator */

	/* Get iterator at the head of the container */
	void *(*iter_head)(ACL_ITER*, const ACL_ARGV*);
	/* Get the next iterator position */
	void *(*iter_next)(ACL_ITER*, const ACL_ARGV*);
	/* Get iterator at the tail of the container */
	void *(*iter_tail)(ACL_ITER*, const ACL_ARGV*);
	/* Get the previous iterator position */
	void *(*iter_prev)(ACL_ITER*, const ACL_ARGV*);

	/* private */
	ACL_DBUF_POOL *dbuf;
};

/* in acl_argv.c */
/**
 * Allocate a dynamic string array.
 * @param size {int} Initial size of the dynamic array
 * @return {ACL_ARGV*}
 */
ACL_API ACL_ARGV *acl_argv_alloc(int size);
ACL_API ACL_ARGV *acl_argv_alloc2(int size, ACL_DBUF_POOL *dbuf);

/**
 * Initialize the iterator functions of an ACL_ARGV object.
 * @param argvp {ACL_ARGV*}
 */
ACL_API void acl_argv_iter_init(ACL_ARGV *argvp);

ACL_API void *acl_argv_iter_head(ACL_ITER *iter, const ACL_ARGV *argv);
ACL_API void *acl_argv_iter_next(ACL_ITER *iter, const ACL_ARGV *argv);
ACL_API void *acl_argv_iter_tail(ACL_ITER *iter, const ACL_ARGV *argv);
ACL_API void *acl_argv_iter_prev(ACL_ITER *iter, const ACL_ARGV *argv);

/**
 * Append multiple strings to the dynamic string array; terminated by a NULL string.
 * @param argvp {ACL_ARGV*} Pointer to the dynamic string array
 * @param ... List of strings terminated by NULL, e.g.: {s1}, {s2}, ..., NULL
 */
ACL_API void acl_argv_add(ACL_ARGV *argvp,...);

/**
 * Set the string value at a specified index and free the original string.
 * @param argvp {ACL_ARGV *} Dynamic string array
 * @param idx {int} Index position; must be within bounds
 * @param value {const char *} Non-NULL string
 * @return {int} Returns -1 if index is out of range or value is NULL; 0 on success
 */
ACL_API int acl_argv_set(const ACL_ARGV *argvp, int idx, const char *value);

/**
 * Append a variable argument list of strings to the dynamic string array.
 * @param argvp {ACL_ARGV*} Pointer to the dynamic string array
 * @param ap {va_list} Variable argument list composed of strings
 */
ACL_API void acl_argv_addv(ACL_ARGV *argvp, va_list ap);

/**
 * Append a list of strings with explicit lengths to the dynamic string array.
 * @param argvp {ACL_ARGV*} Pointer to the dynamic string array
 * @param ... A list of pairs (string, length), for example:
 *            {s1}, {len1}, {s2}, {len2}, ... NULL
 */
ACL_API void acl_argv_addn(ACL_ARGV *argvp,...);

/**
 * Append a list of strings with explicit lengths to the dynamic string array.
 * @param argvp {ACL_ARGV*} Pointer to the dynamic string array
 * @param ap {va_list} Variable argument list composed of (string, length) pairs
 */
ACL_API void acl_argv_addnv(ACL_ARGV *argvp, va_list ap);

/**
 * Mark the end position of the dynamic string array with a terminating NULL.
 * @param argvp {ACL_ARGV*} Pointer to the dynamic string array
 */
ACL_API void acl_argv_terminate(const ACL_ARGV *argvp);

/**
 * Free the dynamic string array and its internal resources.
 * @param argvp {ACL_ARGV*} Pointer to the dynamic string array
 */
ACL_API void acl_argv_free(ACL_ARGV *argvp);

/**
 * Return the string pointer at the given index.
 * @param argvp {ACL_ARGV*} Pointer to the dynamic string array
 * @param idx {int} Index position
 * @return {char*} NULL if index is out of range; otherwise pointer to the string
 */
ACL_API char *acl_argv_index(const ACL_ARGV *argvp, int idx);

/**
 * Return the number of strings currently stored in the dynamic string array.
 * @param argvp {ACL_ARGV*} Pointer to the dynamic string array
 * @return {int}
 */
ACL_API int acl_argv_size(const ACL_ARGV *argvp);

/* in acl_argv_split.c */
/**
 * Split a source string into a dynamic string array using the given delimiter string.
 * @param str {const char*} Source string
 * @param delim {const char*} Delimiter string
 * @return {ACL_ARGV*}
 */
ACL_API ACL_ARGV *acl_argv_split(const char *str, const char *delim);

/**
 * Split a source string into a dynamic string array using the given delimiter,
 * while allocating all internal memory from the specified memory pool.
 * The pool can be NULL, in which case the default allocation strategy is used.
 * @param str {const char*} Source string
 * @param delim {const char*} Delimiter string
 * @param dbuf {ACL_DBUF_POOL*} Memory pool object; can be NULL
 * @return {ACL_ARGV*}
 */
ACL_API ACL_ARGV *acl_argv_split3(const char *str, const char *delim,
	ACL_DBUF_POOL *dbuf);

/**
 * Split a source string into a dynamic string array using the given delimiter,
 * limiting the maximum number of splits.
 * @param str {const char*} Source string
 * @param delim {const char*} Delimiter string
 * @param n {size_t} Maximum number of splits
 * @return {ACL_ARGV*}
 */
ACL_API ACL_ARGV *acl_argv_splitn(const char *str, const char *delim, size_t n);

/**
 * Split a source string into a dynamic string array using the given delimiter,
 * limiting the maximum number of splits, and allocate memory from the given pool.
 * @param str {const char*} Source string
 * @param delim {const char*} Delimiter string
 * @param n {size_t} Maximum number of splits
 * @param dbuf {ACL_DBUF_POOL*} Memory pool object; can be NULL
 * @return {ACL_ARGV*}
 */
ACL_API ACL_ARGV *acl_argv_splitn4(const char *str, const char *delim,
	size_t n, ACL_DBUF_POOL *dbuf);

/**
 * Split the source string using the given delimiter and append the result
 * to an existing dynamic string array.
 * @param argvp {ACL_ARGV*} Pointer to the dynamic string array
 * @param str {const char*} Source string
 * @param delim {const char*} Delimiter string
 * @return {ACL_ARGV*}
 */
ACL_API ACL_ARGV *acl_argv_split_append(ACL_ARGV *argvp, const char *str,
	const char *delim);

/**
 * Split the source string using the given delimiter and append the result
 * to an existing dynamic string array, limiting the maximum number of splits.
 * @param argvp {ACL_ARGV*} Pointer to the dynamic string array
 * @param str {const char*} Source string
 * @param delim {const char*} Delimiter string
 * @param n {size_t} Maximum number of splits
 * @return {ACL_ARGV*}
 */
ACL_API ACL_ARGV *acl_argv_splitn_append(ACL_ARGV *argvp, const char *str,
	const char *delim, size_t n);

/**
 * Split a source string into a dynamic string array using the given delimiter,
 * but treat substrings enclosed in \"\" or '' as a single token.
 * @param str {const char*} Source string
 * @param delim {const char*} Delimiter string
 * @return {ACL_ARGV*}
 */
ACL_API ACL_ARGV *acl_argv_quote_split(const char *str, const char *delim);

/**
 * Split a source string into a dynamic string array using the given delimiter,
 * treating substrings enclosed in \"\" or '' as a single token, and allocate
 * memory from the given pool.
 * @param str {const char*} Source string
 * @param delim {const char*} Delimiter string
 * @param dbuf {ACL_DBUF_POOL*} Memory pool object; can be NULL; when NULL,
 *        the default allocation strategy is used
 * @return {ACL_ARGV*}
 */
ACL_API	ACL_ARGV *acl_argv_quote_split4(const char *str, const char *delim,
	ACL_DBUF_POOL *dbuf);

///////////////////////////////////////////////////////////////////////////////
// Efficient string splitting algorithm

typedef struct ACL_ARGV_VIEW ACL_ARGV_VIEW;

struct ACL_ARGV_VIEW {
	ACL_ARGV argv;

	/* Get iterator at the head of the view container */
	void *(*iter_head)(ACL_ITER*, const ACL_ARGV_VIEW*);
	/* Get the next iterator position */
	void *(*iter_next)(ACL_ITER*, const ACL_ARGV_VIEW*);
	/* Get iterator at the tail of the view container */
	void *(*iter_tail)(ACL_ITER*, const ACL_ARGV_VIEW*);
	/* Get the previous iterator position */
	void *(*iter_prev)(ACL_ITER*, const ACL_ARGV_VIEW*);
};

/**
 * Split the source string in-place using the delimiter characters, where
 * every character in the delimiter string is treated as an individual separator.
 * The returned object only provides a read-only view and must not be modified.
 * @param str {const char*} Source string
 * @param delim {const char*} Each character appearing in this string is treated
 *        as a separator when splitting the source
 * @return {ACL_ARGV_VIEW*} A non-NULL view object on success; the view is
 *         read-only and should be traversed with ACL_ITER
 */
ACL_API ACL_ARGV_VIEW *acl_argv_view_split(const char *str, const char *delim);

/**
 * Free the view object returned by acl_argv_view_split.
 * @param view {ACL_ARGV_VIEW*} View returned by acl_argv_view_split
 */
ACL_API void acl_argv_view_free(ACL_ARGV_VIEW *view);

# ifdef	__cplusplus
}
# endif

#endif
