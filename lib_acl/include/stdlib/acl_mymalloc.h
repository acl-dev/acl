/*
 * @file	mymalloc.h
 * @author	zsx
 * @date	2003-12-15
 * @version	1.0
 * @brief	This file is a high-level memory management interface for using
 *  ACL library. Users should use this interface for memory allocation and
 *  deallocation. Users can call functions in acl_mem_hook.h to register their
 *  own memory allocation and deallocation interfaces, so that when calling
 *  acl_myxxx functions, they will automatically switch to user's own memory
 *  management interfaces.
 */

#ifndef	ACL_MYMALLOC_INCLUDE_H
#define	ACL_MYMALLOC_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include "acl_malloc.h"

/**
 * Dynamic memory allocation macro definition, allocates uninitialized memory space.
 * @param size {size_t} Allocation length
 * @return {void *}
 */
#define acl_mymalloc(size) acl_malloc_glue(__FILE__, __LINE__, size)

/**
 * Dynamic memory allocation macro definition, allocates memory space initialized to zero.
 * @param nmemb {size_t} Number of elements
 * @param size {size_t} Length of each element
 * @return {void *}
 */
#define acl_mycalloc(nmemb, size) acl_calloc_glue(__FILE__, __LINE__, nmemb, size)

/**
 * Reallocate dynamic memory macro definition.
 * @param ptr {void*} Original memory address
 * @param size {size_t} Length requested when reallocating memory
 * @return {void *}
 */
#define acl_myrealloc(ptr, size) acl_realloc_glue(__FILE__, __LINE__, (ptr), size)

/**
 * Dynamic string duplication macro definition.
 * @param str {const char*} Source string
 * @return {char*} New string, must be freed with acl_myfree
 */
#define acl_mystrdup(str) acl_strdup_glue(__FILE__, __LINE__, (str))

/**
 * Dynamic string duplication macro definition, with bounded memory space size.
 * @param str {const char*} Source string
 * @param len {size_t} Maximum memory space size for string duplication
 * @return {char*} New string, must be freed with acl_myfree
 */
#define acl_mystrndup(str, len) acl_strndup_glue(__FILE__, __LINE__, (str), len)

/**
 * Dynamic memory duplication macro definition.
 * @param ptr {const void*} Source memory address
 * @param len {size_t} Source memory size
 * @return {void*} New address, must be freed with acl_myfree
 */
#define acl_mymemdup(ptr, len) acl_memdup_glue(__FILE__, __LINE__, (ptr), len)

/**
 * Free dynamically allocated memory space.
 * @param _ptr_ {void*} Dynamic memory address
 */
#define acl_myfree(_ptr_) do {  \
	if (_ptr_) {  \
		acl_free_glue(__FILE__, __LINE__, (_ptr_));  \
		(_ptr_) = NULL;  \
	}  \
} while (0)

/**
 * XXX: Because this function is used in callback functions, macro conversion
 * cannot be performed, so this function name is used directly.
 */
#define	acl_myfree_fn acl_free_fn_glue

#ifdef  __cplusplus
}
#endif

#endif
