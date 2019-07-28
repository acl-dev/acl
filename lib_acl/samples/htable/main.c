#include "lib_acl.h"
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

static void free_fn(void *arg)
{
	acl_myfree(arg);
}

static void usage(const char *procname)
{
	printf("usage: %s -h[help] -n count -m[use ACL_HTABLE_FLAG_MSLOOK]\n", procname);
}

int main(int argc, char *argv[])
{
	ACL_HTABLE *htable;
	char  key[128], *value;
	int   i, n = 50, ch, flag = 0;

	while ((ch = getopt(argc, argv, "hn:m")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return (0);
		case 'n':
			n = atoi(optarg);
			break;
		case 'm':
			flag |= ACL_HTABLE_FLAG_MSLOOK;
			break;
		default:
			break;
		}
	}

	htable = acl_htable_create(1, flag);

	for (i = 0; i < n; i++) {
		value = (char*) acl_mymalloc(256);
		snprintf(value, 256, "value:%d", i);
		snprintf(key, sizeof(key), "key:%d", i);
		acl_htable_enter(htable, key, value);
	}
	acl_htable_stat(htable);

	printf("------------------------------------------------------------\n");
	for (i = n - 1; i >= 0; i--) {
		snprintf(key, sizeof(key), "key:%d", i);
		(void) acl_htable_find(htable, key);
	}
	acl_htable_stat(htable);

	acl_htable_free(htable, free_fn);
	return (0);
}

