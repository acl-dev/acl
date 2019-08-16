#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>	/* for offsetof */
#include <string.h>
#include <errno.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#ifdef	_USE_GLIB
#include <glib.h>
#endif

#include "stdlib/acl_msg.h"
#include "stdlib/acl_malloc.h"
#ifdef	ACL_UNIX
#include "stdlib/unix/acl_trace.h"
#endif

#endif

static char __FILENAME_UNKNOWN[] = "unknown file";
static size_t  __malloc_limit = 100000000;

/*
  * Structure of an annotated memory block. In order to detect spurious
  * free() calls we prepend a signature to memory given to the application.
  * In order to detect access to free()d blocks, overwrite each block as soon
  * as it is passed to myfree(). With the code below, the user data has
  * integer alignment or better.
  */
typedef struct MBLOCK {
	size_t signature;		/* set when block is active */
	size_t length;			/* user requested length */
	union {
		ALIGN_TYPE align;
		char  payload[1];	/* actually a bunch of bytes */
	} u;
} MBLOCK;

#define SIGNATURE	0xdead
#define FILLER		0x0

#define CHECK_PTR(_ptr_, _real_ptr_, _len_, _fname_, _line_) { \
  if (_ptr_ == 0) \
    acl_msg_fatal("%s(%d): null pointer input", _fname_, _line_); \
  _real_ptr_ = (MBLOCK *) (((char*)_ptr_) - offsetof(MBLOCK, u.payload[0])); \
  if (_real_ptr_->signature != SIGNATURE) \
    acl_msg_fatal("%s(%d): corrupt or unallocated block(%d, 0x%x, 0x%x)", \
      _fname_, _line_, (int) _real_ptr_->length, \
      (int) _real_ptr_->signature, SIGNATURE); \
  if ((_len_ = _real_ptr_->length) < 1) \
    acl_msg_fatal("%s(%d): corrupt memory block length", _fname_, _line_); \
}

#define CHECK_IN_PTR(_ptr_, _real_ptr_, _len_, _fname_, _line_) { \
  if (_ptr_ == 0) \
    acl_msg_fatal("%s(%d): null pointer input", _fname_, _line_); \
  _real_ptr_ = (MBLOCK *) (((char*)_ptr_) - offsetof(MBLOCK, u.payload[0])); \
  if (_real_ptr_->signature != SIGNATURE) \
    acl_msg_fatal("%s(%d): corrupt or unallocated block(%d, 0x%x, 0x%x)", \
      _fname_, _line_, (int) _real_ptr_->length, \
      (int) _real_ptr_->signature, SIGNATURE); \
  _real_ptr_->signature = 0; \
  if ((_len_ = _real_ptr_->length) < 1) \
    acl_msg_fatal("%s(%d): corrupt memory block length", _fname_, _line_); \
}

#define CHECK_OUT_PTR(_ptr_, _real_ptr_, _len_) { \
  _real_ptr_->signature = SIGNATURE; \
  _real_ptr_->length = _len_; \
  _ptr_ = _real_ptr_->u.payload; \
}

#define SPACE_FOR(len)	(offsetof(MBLOCK, u.payload[0]) + len)

/*
  * Optimization for short strings. We share one copy with multiple callers.
  * This differs from normal heap memory in two ways, because the memory is
  * shared:
  * 
  * -  It must be read-only to avoid horrible bugs. This is OK because there is
  * no legitimate reason to modify the null terminator.
  * 
  * - myfree() cannot overwrite the memory with a filler pattern like it can do
  * with heap memory. Therefore, some dangling pointer bugs will be masked.
  */

/*
#define	NO_SHARED_EMPTY_STRINGS
*/

#ifndef NO_SHARED_EMPTY_STRINGS
static char empty_string[] = "";
#endif

/* acl_malloc - allocate memory or bust */

#ifdef	ACL_WINDOWS

# define SET_FILE(_ptr_, _filename_) do {  \
	_ptr_ = strrchr(_filename_, '/');  \
	if (_ptr_ == NULL) {  \
		_ptr_ = strrchr(_filename_, '\\');  \
	}  \
	if (_ptr_ == NULL)  \
		_ptr_ = filename;  \
	else  \
		_ptr_++;  \
} while (0)

#else

# define SET_FILE(_ptr_, _filename_) do {  \
	_ptr_ = strrchr(_filename_, '/');  \
	if (_ptr_ == NULL)  \
		_ptr_ = filename;  \
	else  \
		_ptr_++;  \
} while (0)
#endif  /* ACL_WINDOWS */

/* #define DEBUG_MEM */
#ifdef	DEBUG_MEM
static __thread unsigned long long __nmalloc   = 0;
static __thread unsigned long long __ncalloc   = 0;
static __thread unsigned long long __nrealloc  = 0;
static __thread unsigned long long __nfree     = 0;
static __thread unsigned long long __nstrdup   = 0;
static __thread unsigned long long __nstrndup  = 0;
static __thread unsigned long long __nmemdup   = 0;
static __thread ssize_t __nsize = 0;
#endif

void acl_default_memstat(const char *filename, int line,
	void *ptr, size_t *len, size_t *real_len)
{
	MBLOCK *real_ptr;
	const char *pname = NULL;
	size_t old_len;

	if (filename && *filename)
		SET_FILE(pname, filename);
	else
		pname = __FILENAME_UNKNOWN;

	CHECK_PTR(ptr, real_ptr, old_len, pname, line);
	if (len)
		*len = real_ptr->length;
	if (real_len)
		*real_len = SPACE_FOR(*len);
}
 
void acl_default_meminfo(void)
{
#ifdef DEBUG_MEM
	printf("%s(%d): __nmalloc: %llu, __ncalloc: %llu, __nrealloc: %llu, "
		"__nfree: %llu, diff: %llu, __nsize: %lu\r\n",
		__FUNCTION__, __LINE__, __nmalloc, __ncalloc, __nrealloc,
		__nfree, __nmalloc - __nfree,
		(unsigned long) __nsize);
#endif
}

void acl_default_set_memlimit(size_t len)
{
	acl_assert(len > 0);
	__malloc_limit = len;
}

size_t acl_default_get_memlimit(void)
{
	return __malloc_limit;
}

void *acl_default_malloc(const char *filename, int line, size_t len)
{
	const char *myname = "acl_default_malloc";
	size_t new_len;
	char *ptr;
	MBLOCK *real_ptr;
	const char *pname = NULL;

#if 0
	printf("%s:%d, len: %d\r\n", filename, line, (int) len);
	acl_trace_info();
#endif

	if (filename && *filename)
		SET_FILE(pname, filename);
	else
		pname = __FILENAME_UNKNOWN;

	if (len < 1) {
		acl_msg_warn("%s(%d), %s: malloc: length %ld invalid",
			pname, line, myname, (long) len);
		acl_trace_info();
		len = 128;
	}

	new_len = SPACE_FOR(len);
	if (new_len <= 0)
		acl_msg_fatal("%s(%d): new_len(%d) <= 0",
			myname, __LINE__, (int) new_len);
	else if (new_len >= __malloc_limit) {
		acl_msg_warn("%s(%d): new_len(%d) too large",
			myname, __LINE__, (int) new_len);
		acl_trace_info();
	}

#ifdef 	DEBUG_MEM
	__nmalloc++;
	__nsize += len;
	//printf("malloc: %llu, filename=%s, line=%d\r\n", __nmalloc, filename, line);
#endif

#ifdef	_USE_GLIB
	if ((real_ptr = (MBLOCK *) g_malloc(new_len)) == 0) {
		acl_msg_error("%s(%d)->%s: new_len: %d, g_malloc error(%s)",
			pname, line, myname, (int) new_len, strerror(errno));
		return 0;
	}
#else
	if ((real_ptr = (MBLOCK *) malloc(new_len)) == 0) {
		acl_msg_error("%s(%d)->%s: malloc: insufficient memory: %s, "
			"new_len: %d", pname, line, myname,
			strerror(errno), (int) new_len);
		return 0;
	}
#endif

	CHECK_OUT_PTR(ptr, real_ptr, len);
	return ptr;
}

void *acl_default_calloc(const char *filename, int line,
	size_t nmemb, size_t size)
{
	void *ptr;
	int   n;

#ifdef 	DEBUG_MEM
	__ncalloc++;
	//printf("calloc: %llu, file=%s, line=%d\r\n", __ncalloc, filename, line);
#endif
	n = (int) (nmemb * size);
	ptr = acl_default_malloc(filename, line, n);
	memset(ptr, FILLER, n);
	return ptr;
}

/* acl_default_realloc - reallocate memory or bust */

void *acl_default_realloc(const char *filename, int line,
	void *ptr, size_t len)
{
	const char *myname = "acl_default_realloc";
	MBLOCK *real_ptr;
	size_t old_len, new_len;
	const char *pname = NULL;

	if (filename && *filename)
		SET_FILE(pname, filename);
	else
		pname = __FILENAME_UNKNOWN;

#ifndef NO_SHARED_EMPTY_STRINGS
	if (ptr == empty_string)
		return acl_default_malloc(pname, line, len);
#endif

	if (len < 1) {
		acl_msg_warn("%s(%d)->%s: realloc: requested length %ld",
			pname, line, myname, (long) len);
		acl_trace_info();
		len = 128;
	}

	if (ptr == NULL)
		return acl_default_malloc(pname, line, len);

	CHECK_IN_PTR(ptr, real_ptr, old_len, pname, line);

	new_len = SPACE_FOR(len);
	if (new_len <= 0)
		acl_msg_fatal("%s(%d): new_len(%d) <= 0",
			myname, __LINE__, (int) new_len);
	else if (new_len >= __malloc_limit) {
		acl_msg_warn("%s(%d): new_len(%d) too large",
			myname, __LINE__, (int) new_len);
		acl_trace_info();
	}

#ifdef 	DEBUG_MEM
	if (ptr)
		__nrealloc++;
	else
		__nmalloc++;
	__nsize += len;
	__nsize -= old_len;
#endif

#ifdef	_USE_GLIB
	if ((real_ptr = (MBLOCK *) g_realloc((char *) real_ptr, new_len)) == 0)
		acl_msg_fatal("%s(%d)->%s: realloc: insufficient memory: %s",
			pname, line, myname, strerror(errno));
#else
	if ((real_ptr = (MBLOCK *) realloc((char *) real_ptr, new_len)) == 0)
		acl_msg_fatal("%s(%d)->%s: realloc: insufficient memory: %s",
			pname, line, myname, strerror(errno));
#endif
	CHECK_OUT_PTR(ptr, real_ptr, len);
#if 0
	if (len > old_len)
		memset((char *) ptr + old_len, FILLER, len - old_len);
#endif

	return ptr;
}

/* acl_default_free - release memory */

void acl_default_free(const char *filename, int line, void *ptr)
{
	const char *myname = "acl_default_free";
	MBLOCK *real_ptr;
	size_t len;
	const char *pname = NULL;

	if (filename && *filename)
		SET_FILE(pname, filename);
	else
		pname = __FILENAME_UNKNOWN;

	if (ptr == NULL) {
		acl_msg_error("%s(%d)->%s: ptr null", pname, line, myname);
		return;
	}

# ifndef NO_SHARED_EMPTY_STRINGS
	if (ptr != empty_string) {
# endif
		CHECK_IN_PTR(ptr, real_ptr, len, pname, line);
/*
		memset((char *) real_ptr, FILLER, SPACE_FOR(len));
*/

#ifdef 	DEBUG_MEM
		__nfree++;
		__nsize -= len;
		//printf("free: %llu, filename=%s, line=%d\n", __nfree, filename, line);
#endif

#ifdef	_USE_GLIB
		g_free(real_ptr);
#else
		free((char *) real_ptr);
#endif
# ifndef NO_SHARED_EMPTY_STRINGS
	} 
# endif
}

/* acl_default_strdup - save string to heap */

char *acl_default_strdup(const char *filename, int line, const char *str)
{
	const char *myname = "acl_default_strdup";
	const char *pname = NULL;

	if (filename && *filename)
		SET_FILE(pname, filename);
	else
		pname = __FILENAME_UNKNOWN;

	if (str == 0)
		acl_msg_fatal("%s(%d)->%s: null pointer argument",
			pname, line, myname);

#ifndef NO_SHARED_EMPTY_STRINGS
	if (*str == 0)
		return (char *) empty_string;
#endif

#ifdef 	DEBUG_MEM
	__nstrdup++;
#endif

	return strcpy(acl_default_malloc(pname, line, strlen(str) + 1), str);
}

/* acl_default_strndup - save substring to heap */

char *acl_default_strndup(const char *filename, int line,
	const char *str, size_t len)
{
	const char *myname = "acl_default_strndup";
	char *result;
	char *cp;
	const char *pname = NULL;

	if (filename && *filename)
		SET_FILE(pname, filename);
	else
		pname = __FILENAME_UNKNOWN;

	if (str == 0)
		acl_msg_fatal("%s(%d)->%s: null pointer argument",
			pname, line, myname);

#ifndef NO_SHARED_EMPTY_STRINGS
	if (*str == 0)
		return (char *) empty_string;
#endif

	if ((cp = memchr(str, 0, len)) != 0)
		len = cp - str;

#ifdef 	DEBUG_MEM
	__nstrndup++;
#endif

	result = memcpy(acl_default_malloc(pname, line, len + 1), str, len);
	result[len] = 0;
	return result;
}

/* acl_default_memdup - copy memory */

void *acl_default_memdup(const char *filename, int line,
	const void *ptr, size_t len)
{
	const char *myname = "acl_default_memdup";
	const char *pname = NULL;

	if (filename && *filename)
		SET_FILE(pname, filename);
	else
		pname = __FILENAME_UNKNOWN;

	if (ptr == 0)
		acl_msg_fatal("%s(%d)->%s: null pointer argument",
			pname, line, myname);

#ifdef 	DEBUG_MEM
	__nmemdup++;
#endif

	return memcpy(acl_default_malloc(pname, line, len), ptr, len);
}
