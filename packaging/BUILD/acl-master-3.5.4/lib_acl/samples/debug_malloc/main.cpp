#include "lib_acl.h"

int main(int argc acl_unused, char *argv[] acl_unused)
{
	char *ptr2;
	char *ptr3 = acl_mystrdup("hello world!");
	int   i;

	acl_debug_malloc_init(NULL, "log.txt");

	for (i = 0; i < 100; i++) {
		(void) acl_mymalloc(100);
		ptr2 = (char*) acl_mymalloc(100);
		acl_myfree(ptr2);
	}

	acl_myfree(ptr2);
	acl_myfree(ptr3);
	return 0;
}
