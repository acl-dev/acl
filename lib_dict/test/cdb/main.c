#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "lib_acl.h"
#include "dict.h"

static void test_set(const char *filename, int count)
{
	int i;
	int open_flags = O_RDWR | O_CREAT | O_TRUNC;
	int dict_flags = DICT_FLAG_DUP_IGNORE;
//	int dict_flags = DICT_FLAG_DUP_REPLACE;
//	int dict_flags = DICT_FLAG_NONE;
	DICT *dict = dict_open3("cdb", filename, open_flags, dict_flags);
	char  key[128], value[256];

	if (dict == NULL) {
		printf("open %s error %s\r\n", filename, acl_last_serror());
		return;
	}

	for (i = 0; i < count; i++) {
		snprintf(key, sizeof(key), "key-%d", i);
		snprintf(value, sizeof(value), "value-%d==========================---", i);
		dict->update(dict, key, strlen(key), value, strlen(value) + 1);
		if (i % 10000 == 0) {
			char info[256];
			snprintf(info, sizeof(info), "i=%d", i);
			ACL_METER_TIME(info);
		}
	}
	DICT_CLOSE(dict);
}

static char *test_get_one(const char *filename, char *key)
{
	int open_flags = O_RDONLY;
	int dict_flags = DICT_FLAG_NONE;
	DICT *dict = dict_open3("cdb", filename, open_flags, dict_flags);
	char *value;
	const char *ptr;
	size_t key_len, val_len;

	if (dict == NULL) {
		printf("open %s error %s\r\n", filename, acl_last_serror());
		return NULL;
	}

	key_len = strlen(key);
	ptr = dict->lookup(dict, key, key_len, &value, &val_len);
	if (ptr == NULL) {
		printf("not exist key=%s\r\n", key);
		DICT_CLOSE(dict);
		return NULL;
	}

	DICT_CLOSE(dict);
	return value;
}

static void test_get_once(const char *filename, int count)
{
	char *value, key[128];
	int i;

	for (i = 0; i < count; i++) {
		snprintf(key, sizeof(key), "key-%d", i);
		value = test_get_one(filename, key);
		if (value == NULL) {
			break;
		}
		if (i < 100) {
			printf("key=%s, value=%s\r\n", key, value);
		}
		acl_myfree(value);
		if (i % 10000 == 0) {
			char info[256];
			snprintf(info, sizeof(info), "i=%d", i);
			ACL_METER_TIME(info);
		}
	}
}

static void test_get(const char *filename, int count)
{
	int open_flags = O_RDONLY;
	int dict_flags = DICT_FLAG_NONE;
	DICT *dict = dict_open3("cdb", filename, open_flags, dict_flags);
	char *value, key[128];
	const char *ptr;
	size_t key_len, val_len;
	int i;

	if (dict == NULL) {
		printf("open %s error %s\r\n", filename, acl_last_serror());
		return;
	}

	for (i = 0; i < count; i++) {
		snprintf(key, sizeof(key), "key-%d", i);
		key_len = strlen(key);
		ptr = dict->lookup(dict, key, key_len, &value, &val_len);
		if (ptr == NULL) {
			printf("not exist, key=%s\r\n", key);
			break;
		}
		if (i < 100) {
			printf("key=%s, value=%s\r\n", key, value);
		}
		acl_myfree(value);
		if (i % 10000 == 0) {
			char info[256];
			snprintf(info, sizeof(info), "i=%d", i);
			ACL_METER_TIME(info);
		}
	}

	DICT_CLOSE(dict);
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]"
		"    -f filename\r\n"
		"    -a actoin[set|get|once]\r\n"
		"    -n count\r\n"
		, procname);
}

int main(int argc, char* argv[])
{
	char filename[256], action[64];
	int  ch, count = 100;

	action[0]   = 0;

	snprintf(filename, sizeof(filename), "test");

	while ((ch = getopt(argc, argv, "hf:a:n:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'f':
			snprintf(filename, sizeof(filename), "%s", optarg);
			break;
		case 'a':
			snprintf(action, sizeof(action), "%s", optarg);
			break;
		case 'n':
			count = atoi(optarg);
			break;
		default:
			break;
		}
	}

	if (filename[0] == 0 || action[0] == 0) {
		usage(argv[0]);
		return 1;
	}

	acl_msg_stdout_enable(1);
	dict_open_init();

	if (strcasecmp(action, "set") == 0) {
		test_set(filename, count);
	} else if (strcasecmp(action, "get") == 0) {
		test_get(filename, count);
	} else if (strcasecmp(action, "once") == 0) {
		test_get_once(filename, count);
	} else {
		usage(argv[0]);
		return 1;
	}

	return 0;
}
