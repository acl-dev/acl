#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fiber/lib_fiber.h"

static void fiber_main(ACL_FIBER *fiber acl_unused, void *ctx)
{
	const char *path = (const char*) ctx;
	
	if (acl_make_dirs(path, 0755) < 0)
		printf("acl_make_dirs error=%s, path=%s\r\n",
			acl_last_serror(), path);
	else
		printf("acl_make_dirs %s ok\r\n", path);
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -d path\r\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch;
	char  path[256];

	snprintf(path, sizeof(path), "./path/d1/d2");

	while ((ch = getopt(argc, argv, "hd:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'd':
			snprintf(path, sizeof(path), "%s", optarg);
			break;
		default:
			break;
		}
	}

	acl_fiber_create(fiber_main, path, 128000);

	acl_fiber_schedule();
	return 0;
}
