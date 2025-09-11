#include "stdafx.h"
#include "util.h"

static int max = 1000;
const char *str = "hello world, you're welcome here! what's day is today? what's your name? How old are you? Can I help you?\r\n";
const char *sep = ",;'?! \t\r\n";

static void test1() {
	for (int i = 0; i < max; i++) {
		acl::string buf(str);
		std::list<acl::string>& tokens = buf.split(sep);
		if (i > 0) {
			continue;
		}

		printf("Use acl::string::split:\r\n");
		for (std::list<acl::string>::const_iterator cit = tokens.begin();
			cit != tokens.end(); ++cit) {
			printf("%s ", (*cit).c_str());
		}
		printf("\r\n");
	}
}

static void test2() {
	for (int i = 0; i < max; i++) {
		std::list<std::string> tokens;
		acl::split(str, sep, tokens);
		if (i > 0) {
			continue;
		}

		printf("Use acl::split(list):\r\n");
		for (std::list<std::string>::const_iterator cit = tokens.begin();
			cit != tokens.end(); ++cit) {
			printf("%s ", (*cit).c_str());
		}
		printf("\r\n");
	}
}

static void test3() {
	for (int i = 0; i < max; i++) {
		std::vector<std::string> tokens;
		acl::split(str, sep, tokens);
		if (i > 0) {
			continue;
		}

		printf("Use acl::split(vector):\r\n");
		for (std::vector<std::string>::const_iterator cit = tokens.begin();
			cit != tokens.end(); ++cit) {
			printf("%s ", (*cit).c_str());
		}
		printf("\r\n");
	}
}

static void test4() {
	for (int i = 0; i < max; i++) {
		ACL_ARGV *argv = acl_argv_split(str, sep);
		if (i > 0) {
			acl_argv_free(argv);
			continue;
		}

		printf("Use acl_argv_split:\r\n");
		ACL_ITER iter;
		acl_foreach(iter, argv) {
			printf("%s ", (char*) iter.data);
		}
		printf("\r\n");

		acl_argv_free(argv);

	}
}

static void test5() {
	for (int i = 0; i < max; i++) {
		ACL_ARGV_VIEW *view = acl_argv_view_split(str, sep);
		if (i > 0) {
			acl_argv_view_free(view);
			continue;
		}

		printf("Use acl_argv_view_split:\r\n");
		ACL_ITER iter;
		acl_foreach(iter, view) {
			printf("%s ", (char*) iter.data);
		}
		printf("\r\n");

		acl_argv_view_free(view);
	}
}

static void usage(const char* procname) {
	printf("usage: %s -h [help]\r\n"
		"\t-n max_loop[10]\r\n"
		, procname);
}

int main(int argc, char* argv[]) {
	int   ch;

	while ((ch = getopt(argc, argv, "hn:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			max = atoi(optarg);
			if (max < 100) {
				max = 100;
			}
			break;
		default:
			usage(argv[0]);
			return 0;
		}
	}

	//////////////////////////////////////////////////////////////////////

	struct timeval begin, end;

	//////////////////////////////////////////////////////////////////////

	gettimeofday(&begin, NULL);
	test1();
	gettimeofday(&end, NULL);

	double tc = util::stamp_sub(&end, &begin);
	printf("Test1 time cost: %.2f ms\r\n", tc);
	printf("--------------------------------------------------------\r\n");

	//////////////////////////////////////////////////////////////////////

	gettimeofday(&begin, NULL);
	test2();
	gettimeofday(&end, NULL);

	tc = util::stamp_sub(&end, &begin);
	printf("Test2 time cost: %.2f ms\r\n", tc);
	printf("--------------------------------------------------------\r\n");

	//////////////////////////////////////////////////////////////////////

	gettimeofday(&begin, NULL);
	test3();
	gettimeofday(&end, NULL);

	tc = util::stamp_sub(&end, &begin);
	printf("Test3 time cost: %.2f ms\r\n", tc);
	printf("--------------------------------------------------------\r\n");

	//////////////////////////////////////////////////////////////////////

	gettimeofday(&begin, NULL);
	test4();
	gettimeofday(&end, NULL);

	tc = util::stamp_sub(&end, &begin);
	printf("Test4 time cost: %.2f ms\r\n", tc);
	printf("--------------------------------------------------------\r\n");

	//////////////////////////////////////////////////////////////////////

	gettimeofday(&begin, NULL);
	test5();
	gettimeofday(&end, NULL);

	tc = util::stamp_sub(&end, &begin);
	printf("Test5 time cost: %.2f ms\r\n", tc);

	//////////////////////////////////////////////////////////////////////

	acl::string buf;
	const std::vector<acl::string>& tokens = buf.split2(";");
	printf("tokens' size: %zd\r\n", tokens.size());

	return 0;
}
