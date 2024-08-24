#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include "lib_acl.h"

static void test_json(const char *data, ACL_ARGV *names)
{
	ACL_JSON *json = acl_json_alloc();
	ACL_VSTRING *buf = acl_vstring_alloc(1024);
	ACL_ITER iter, iter2;

	acl_json_update(json, data);

	if (!acl_json_finish(json)) {
		printf("Invalid json!\r\n");
		return;
	}

#define STR	acl_vstring_str
#define LEN	ACL_VSTRING_LEN
#define EQ	!strcmp

	acl_foreach(iter, json) {
		ACL_JSON_NODE* node = (ACL_JSON_NODE*) iter.data;
		if (LEN(node->ltag) == 0) {
			continue;
		}

		acl_foreach(iter2, names) {
			const char *name = (const char*) iter2.data;
			if (EQ(STR(node->ltag), name)) {
				acl_json_node_disable(node, 1);
			}
		}
	}

	acl_json_build(json, buf);
	printf("%s\r\n", STR(buf));
	acl_json_free(json);
	acl_vstring_free(buf);
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -f filename -n tag_names\r\n", procname);
}

int main(int argc, char *argv[])
{
	ACL_ARGV *names;
	char name[512], filename[256], *data;
	int ch;

	snprintf(name, sizeof(name), "name");
	snprintf(filename, sizeof(filename), "json.txt");

	while ((ch = getopt(argc, argv, "hf:n:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'f':
			snprintf(filename, sizeof(filename), "%s", optarg);
			break;
		case 'n':
			snprintf(name, sizeof(name), "%s", optarg);
			break;
		default:
			break;
		}
	}

	if (name[0] == 0 || filename[0] == 0) {
		usage(argv[0]);
		return 0;
	}

	data = acl_vstream_loadfile(filename);
	if (data == NULL || *data == 0) {
		printf("load %s error %s\r\n", filename, acl_last_serror());
		return 1;
	}

	names = acl_argv_split(name, ",; \t");
	test_json(data, names);
	acl_argv_free(names);
	acl_myfree(data);

#ifdef	WIN32
	getchar();
#endif

	return (0);
}
