#include "lib_acl.h"

int main(int argc, char* argv[])
{
	char  path[256];
	int   i, n = 1024;

	if (argc != 2)
	{
		printf("usage: %s path\r\n", argv[1]);
		return 1;
	}

	for (i = 0; i < n; i++)
	{
		snprintf(path, sizeof(path), "%s/%d", argv[1], i);
		if (acl_make_dirs(path, 0700) == -1)
			printf("mkdir %s failed: %s\r\n", path, acl_last_serror());
		else
			printf("mkdir %s ok\r\n", path);
	}
	return 0;
}
