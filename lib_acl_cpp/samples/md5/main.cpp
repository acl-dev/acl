// md5.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <assert.h>
#include "md5_c.h"

static void base_test(void)
{
	const char* s = "中国人民银行！";
	const char* key = "zsxxsz";
	char  buf1[33], buf2[33];

	acl::md5::md5_string(s, strlen(s), key, strlen(key),
		buf1, sizeof(buf1));
	printf("first md5: %s\r\n", buf1);

	md5_string(s, key, strlen(key), buf2, sizeof(buf2));
	printf("second md5: %s\r\n", buf2);

	assert(strcmp(buf1, buf2) == 0);

	/////////////////////////////////////////////////////////////////////

	acl::md5 md5;

	s = "ABCDEFGHIGKLMNOPQRSTUVWXYZ";
	md5.update(s, strlen(s));
	md5.finish();
	acl::safe_snprintf(buf1, sizeof(buf1), "%s", md5.get_string());

	md5.reset();

	char ch;
	size_t len = strlen(s);
	for (size_t i = 0; i < len; i++) {
		ch = s[i];
		md5.update(&ch, 1);
	}
	md5.finish();
	acl::safe_snprintf(buf2, sizeof(buf2), "%s", md5.get_string());

	if (strcmp(buf1, buf2) == 0)
		printf("OK: %s\r\n", buf1);
	else
		printf("error, buf1: %s, buf2: %s\r\n", buf1, buf2);

	/////////////////////////////////////////////////////////////////////

	md5.reset();
	len = 1024000;
	char* buf = (char*) malloc(len);

	for (size_t i = 0; i < len; i++) {
		ch = i % 255;
		buf[i] = ch;
		md5.update(&ch, 1);
	}
	md5.finish();
	acl::safe_snprintf(buf1, sizeof(buf1), "%s", md5.get_string());

	md5.reset();
	md5.update(buf, len);
	md5.finish();
	acl::safe_snprintf(buf2, sizeof(buf2), "%s", md5.get_string());

	if (strcmp(buf1, buf2) == 0)
		printf("OK2: %s\r\n", buf1);
	else
		printf("error, buf1: %s, buf2: %s\r\n", buf1, buf2);
}

static void check_file(const char* filepath)
{
	char buf[33];
	if (acl::md5::md5_file(filepath, NULL, 0, buf, sizeof(buf)) == -1) {
		printf("md5 check %s error\r\n", filepath);
	} else {
		printf("md5 ok, file=%s, result=%s\r\n", filepath, buf);
	}
}

static void usage(const char* procname)
{
	printf("usage: %s -h[help] -f filepath\r\n", procname);
#if defined(_WIN32) || defined(_WIN64)
	printf("Enter any key to continue...");
	fflush(stdout);
	getchar();

#endif
}
int main(int argc, char* argv[])
{
	int ch;
	acl::string filepath;

	while ((ch = getopt(argc, argv, "hf:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'f':
			filepath = optarg;
			break;
		default:
			break;
		}
	}
	base_test();
	if (!filepath.empty()) {
		check_file(filepath);
	}

#if defined(_WIN32) || defined(_WIN64)
	getchar();
#endif
	return 0;
}
