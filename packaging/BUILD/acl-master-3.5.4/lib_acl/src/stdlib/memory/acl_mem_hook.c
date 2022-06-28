#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdlib.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_msg.h"
#include "stdlib/acl_malloc.h"
#include "stdlib/acl_mem_hook.h"

#endif

#include "malloc_vars.h"

void acl_mem_hook(void *(*malloc_hook)(const char*, int, size_t),
		void *(*calloc_hook)(const char*, int, size_t, size_t),
		void *(*realloc_hook)(const char*, int, void*, size_t),
		char *(*strdup_hook)(const char*, int, const char*),
		char *(*strndup_hook)(const char*, int, const char*, size_t),
		void *(*memdup_hook)(const char*, int, const void*, size_t),
		void  (*free_hook)(const char*, int, void*))
{
	const char *myname = "acl_mem_hook";

	if (malloc_hook == NULL) {
		acl_msg_error("%s(%d): malloc_hook null", myname, __LINE__);
	} else if (calloc_hook == NULL) {
		acl_msg_error("%s(%d): calloc_hook null", myname, __LINE__);
	} else if (realloc_hook == NULL) {
		acl_msg_error("%s(%d): realloc_hook null", myname, __LINE__);
	} else if (strdup_hook == NULL) {
		acl_msg_error("%s(%d): strdup_hook null", myname, __LINE__);
	} else if (strndup_hook == NULL) {
		acl_msg_error("%s(%d): strncup_hook null", myname, __LINE__);
	} else if (memdup_hook == NULL) {
		acl_msg_error("%s(%d): memdup_hook null", myname, __LINE__);
	} else if (free_hook == NULL) {
		acl_msg_error("%s(%d): free_hook null", myname, __LINE__);
	} else {
		__malloc_fn = malloc_hook;
		__calloc_fn = calloc_hook;
		__realloc_fn = realloc_hook;
		__strdup_fn = strdup_hook;
		__strndup_fn = strndup_hook;
		__memdup_fn = memdup_hook;
		__free_fn = free_hook;
	}
}

void acl_mem_unhook(void)
{
	__malloc_fn = acl_default_malloc;
	__calloc_fn = acl_default_calloc;
	__realloc_fn = acl_default_realloc;
	__strdup_fn = acl_default_strdup;
	__strndup_fn = acl_default_strndup;
	__memdup_fn = acl_default_memdup;
	__free_fn = acl_default_free;
}
