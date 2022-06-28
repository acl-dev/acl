#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>

#include "test_stdlib.h"

int test_fatal_malloc(AUT_LINE *test_line acl_unused, void *arg acl_unused)
{
	char  *ptr;

	acl_memory_debug_start();
	ptr = acl_mymalloc(-1);
	assert(ptr);
	acl_memory_debug_stop();

	/*not reached */
	return (0);
}

int test_fatal_free(AUT_LINE *test_line acl_unused, void *arg acl_unused)
{
	char *ptr = NULL;

	acl_memory_debug_start();
	acl_myfree(ptr);
	acl_memory_debug_stop();

	/*not reached */
	return (0);
}
