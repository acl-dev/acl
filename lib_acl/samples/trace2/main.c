#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

static void test_trace(void)
{
	const char *s = "hello hello hello hello hello hello";
	ACL_ARGV *argv = acl_argv_split(s, " ");
	ACL_HTABLE *htable;
	acl_argv_free(argv);

	printf("-------------------------------------------------------\r\n");

	acl_default_set_memlimit(100);
	htable = acl_htable_create(100, 0);
	acl_htable_enter(htable, "hello", acl_mystrdup(s));
	acl_htable_free(htable, acl_myfree_fn);

	printf("-------------------------------------------------------\r\n");
	printf("addr: %p\r\n",  __builtin_return_address (0));
	printf("addr: %p\r\n",  __builtin_return_address (1));
}

int main(void)
{
	char *ptr = (char*) malloc(100);
	size_t n = malloc_usable_size(ptr);

	printf("n: %d\r\n", (int) n);
	acl_msg_stdout_enable(1);
	test_trace();
	return (0);
}
