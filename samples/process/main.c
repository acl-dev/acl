#include "lib_acl.h"
#include <stdio.h>

int main(int argc acl_unused, char *argv[] acl_unused)
{
	const char *ptr = acl_process_path();
	const char *ptr1 = acl_getcwd();

	printf("current process file's path: %s, getcwd: %s\n",
		ptr ? ptr : "unknown", ptr1 ? ptr1 : "unknown");

#ifdef	ACL_MS_WINDOWS
	getchar();
#endif
	return (0);
}
