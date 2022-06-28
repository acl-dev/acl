#include "lib_acl.h"

extern char **_environ;

int main(void)
{
	const char **pptr = (const char**) _environ;

	printf("Content-Type: text/html\r\n\r\n");
	while (*pptr) {
		printf("%s<br>\r\n", *pptr);
		pptr++;
	}

	return 0;
}
