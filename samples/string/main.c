#include "lib_acl.h"

int main(void)
{
	char *src = acl_mystrdup("hello \tworld! you're  welcome to China!");
	char *ptr, *src_saved;
	ACL_VSTRING* buf;
	const char* s = "hello";
	unsigned int   n1 = (unsigned int) -1;
	unsigned long  n2 = (unsigned long) -1;
	unsigned long long n3 = (unsigned long long) -1;

	src_saved = src;
	printf("src: %s\r\n", src);
	while ((ptr = acl_mystrtok(&src, " \t!'")) != NULL)
	{
		printf("--------------------------------------\r\n");
		printf("ptr: |%s|\r\n", ptr);
		printf("src: |%s|\r\n", src);
		printf("src_saved: |%s|\r\n", src_saved);
	}

	acl_myfree(src_saved);

	printf("----------------------------------------------\r\n");

	buf = acl_vstring_alloc(1);
	acl_vstring_sprintf(buf, "%*lu, s: %s, n1: %20u, n2: %20lu, n3: %20llu\n",
		(int) sizeof(unsigned long) * 4, (unsigned long) getpid(),
		s, n1, n2, n3);
	printf("buf: %s\r\n", acl_vstring_str(buf));
	acl_vstring_free(buf);

	return 0;
}
