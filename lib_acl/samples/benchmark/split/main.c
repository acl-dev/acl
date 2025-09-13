#include "lib_acl.h"
#include "../stamp.h"

static void test1(long long max, const char *str, const char *sep) {
	long long i;

	for (i = 0; i < max; i++) {
		ACL_ARGV *argv = acl_argv_split(str, sep);
		acl_argv_free(argv);
	}
}

static void argv_extend(ACL_ARGV *argv) {
	argv->len = argv->len * 2;
	argv->argv = (char**) acl_myrealloc((char*) argv->argv,
		(argv->len + 1) * sizeof(char*));
}

#define SPACE_LEFT(a) ((a)->len - (a)->argc - 1)

static ACL_ARGV *split2(const char *str, const char *sep) {
#if 0
	size_t len = strlen(str), i, start = 0;
	ACL_ARGV *argv = acl_argv_alloc(5);

	for (i = 0; i < len; i++) {
		if (strchr(sep, str[i]) != NULL) {
			if (start < i) {
				size_t n = i - start;
				char *buf = (char*) acl_mymalloc(n + 1);

				if (SPACE_LEFT(argv) <= 0) {
					argv_extend(argv);
				}
				memcpy(buf, str + start, n);
				buf[n] = 0;
				argv->argv[argv->argc++] = buf;
			}
			start = i + 1;
		}
	}

	if (start < len) {
		size_t n = len - start;
		char *buf = (char*) acl_mymalloc(n + 1);

		if (SPACE_LEFT(argv) <= 0) {
			argv_extend(argv);
		}
		memcpy(buf, str + start, n);
		buf[n] = 0;
		argv->argv[argv->argc++] = buf;
	}
#else
	const char *ptr = str, *start = str;
	ACL_ARGV *argv = acl_argv_alloc(5);

	while (*ptr != 0) {
		if (strchr(sep, (int) (*ptr)) != NULL) {
			if (start < ptr) {
				size_t n = ptr - start;
				char *buf = (char*) acl_mymalloc(n + 1);

				if (SPACE_LEFT(argv) <= 0) {
					argv_extend(argv);
				}
				memcpy(buf, start, n);
				buf[n] = 0;
				argv->argv[argv->argc++] = buf;
			}
			start = ptr + 1;
		}
		ptr++;
	}

	if (*start) {
		char *buf = (char*) acl_mystrdup(start);

		if (SPACE_LEFT(argv) <= 0) {
			argv_extend(argv);
		}
		argv->argv[argv->argc++] = buf;
	}
#endif

	return argv;
}

static void test2(long long max, const char *str, const char *sep) {
	long long i;

	for (i = 0; i < max; i++) {
		ACL_ARGV *argv = split2(str, sep);
		acl_argv_free(argv);
	}
}

static ACL_ARGV *split3(char *str, const char *sep) {
	char *ptr = str, *start = str;
	ACL_ARGV *argv = acl_argv_alloc(5);

	while (*ptr != 0) {
		if (strchr(sep, (int) (*ptr)) != NULL) {
			if (start < ptr) {
				if (SPACE_LEFT(argv) <= 0) {
					argv_extend(argv);
				}
				*ptr = 0;
				argv->argv[argv->argc++] = start;
			}
			start = ptr + 1;
		}
		ptr++;
	}

	if (*start) {
		if (SPACE_LEFT(argv) <= 0) {
			argv_extend(argv);
		}
		argv->argv[argv->argc++] = start;
	}

	return argv;
}

static void test3(long long max, const char *str, const char *sep) {
	long long i;

	for (i = 0; i < max; i++) {
		char *buf = strdup(str);
		ACL_ARGV *argv = split3(buf, sep);
		acl_myfree(argv->argv);
		acl_myfree(argv);
		free(buf);
	}
}

static ACL_ARGV *split4(char *str, const char *sep) {
	size_t len = strlen(str), i, start = 0;
	ACL_ARGV *argv = acl_argv_alloc(5);

	for (i = 0; i < len; i++) {
		if (strchr(sep, str[i]) != NULL) {
			if (start < i) {
				if (SPACE_LEFT(argv) <= 0) {
					argv_extend(argv);
				}
				str[i] = 0;
				argv->argv[argv->argc++] = str + start;
			}
			start = i + 1;
		}
	}

	if (start < len) {
		if (SPACE_LEFT(argv) <= 0) {
			argv_extend(argv);
		}

		argv->argv[argv->argc++] = str + start;
	}

	return argv;
}

static void test4(long long max, const char *str, const char *sep) {
	long long i;

	for (i = 0; i < max; i++) {
		char *buf = strdup(str);
		ACL_ARGV *argv = split4(buf, sep);
		acl_myfree(argv->argv);
		acl_myfree(argv);
		free(buf);
	}
}

static void test5(long long max, const char *str, const char *sep) {
	long long i;

	for (i = 0; i < max; i++) {
		ACL_ARGV_VIEW *view = acl_argv_view_split(str, sep);
		acl_argv_view_free(view);
	}
}

static void test6(const char *str, const char *sep) {
	ACL_ITER iter;
	ACL_ARGV *argv = split2(str, sep);

	acl_foreach(iter, argv) {
		const char *ptr = (const char*) iter.data;
		printf("%s\r\n", ptr);
	}

	acl_argv_free(argv);
}

static int argv_cmp(const ACL_ARGV *argv1, const ACL_ARGV *argv2) {
	assert(argv1->argc == argv2->argc);
	int i;

	for (i = 0; i < argv1->argc; i++) {
		if (strcmp(argv1->argv[i], argv2->argv[i]) != 0) {
			printf("different, i=%d, %s, %s\r\n", i,
				argv1->argv[i], argv2->argv[i]);
			return -1;
		}
	}

	return 0;
}

static void test7(const char *str, const char *sep) {
	ACL_ARGV *argv1 = acl_argv_split(str, sep);
	ACL_ARGV *argv2 = split2(str, sep);

	char *buf1 = strdup(str);
	ACL_ARGV *argv3 = split3(buf1, sep);

	char *buf2 = strdup(str);
	ACL_ARGV *argv4 = split4(buf2, sep);

	ACL_ARGV_VIEW *view = acl_argv_view_split(str, sep);

	if (argv_cmp(argv1, argv2) == 0) {
		printf("ok, argv1 == argv2\r\n");
	} else {
		printf("error, argv1 != argv2\r\n");
		exit(1);
	}


	if (argv_cmp(argv2, argv3) == 0) {
		printf("ok, argv2 == argv3\r\n");
	} else {
		printf("error, argv2 != argv3\r\n");
		exit(1);
	}

	if (argv_cmp(argv3, argv4) == 0) {
		printf("ok, argv3 == argv4\r\n");
	} else {
		printf("error, argv3 != argv4\r\n");
		exit(1);
	}

	if (argv_cmp(argv4, &view->argv) == 0) {
		printf("ok, argv4 == view\r\n");
	} else {
		printf("error, argv4 != view\r\n");
		exit(1);
	}

	ACL_ITER iter;
	acl_foreach(iter, argv3) {
		printf("ARGV->%s\r\n", (char*) iter.data);
	}

	acl_foreach(iter, view) {
		printf("View->%s\r\n", (char*) iter.data);
	}
	acl_argv_free(argv1);
	acl_argv_free(argv2);

	acl_myfree(argv3->argv);
	acl_myfree(argv3);
	free(buf1);

	acl_myfree(argv4->argv);
	acl_myfree(argv4);
	free(buf2);

	acl_argv_view_free(view);
}

static void usage(const char *procname) {
	printf("usage: %s -h [help]\r\n"
		" -n max_loop[default: 10000]\r\n"
		, procname);
}

int main(int argc, char *argv[]) {
	struct timeval begin, end;
	double spent, speed;
	int ch;
	long long max = 100000;
	const char *str = "hello world, you're welcome here! what's day is today? what's your name? How old are you? Can I help you?\r\n";
	const char *sep = ",;'?! \t\r\n";

	while ((ch = getopt(argc, argv, "hn:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			max = atoll(optarg);
			break;
		default:
			usage(argv[0]);
			return 0;
		}
	}

	gettimeofday(&begin, NULL);
	test1(max, str, sep);
	gettimeofday(&end, NULL);

	spent = stamp_sub(&end, &begin);
	speed = (max * 1000) / (spent >= 1.0 ? spent : 1.0);
	printf("test1 bench: loop=%lld, spent=%.2f ms, speed=%.2f\r\n", max, spent, speed);

	/*------------------------------------------------------------------*/

	printf("Enter any key to continue...");
	getchar();

	gettimeofday(&begin, NULL);
	test2(max, str, sep);
	gettimeofday(&end, NULL);

	spent = stamp_sub(&end, &begin);
	speed = (max * 1000) / (spent >= 1.0 ? spent : 1.0);
	printf("test2 bench: loop=%lld, spent=%.2f ms, speed=%.2f\r\n", max, spent, speed);

	printf("-------------------------------------------------------\r\n");

	printf("Enter any key to continue...");
	getchar();

	gettimeofday(&begin, NULL);
	test3(max, str, sep);
	gettimeofday(&end, NULL);

	spent = stamp_sub(&end, &begin);
	speed = (max * 1000) / (spent >= 1.0 ? spent : 1.0);
	printf("test3 bench: loop=%lld, spent=%.2f ms, speed=%.2f\r\n", max, spent, speed);

	printf("-------------------------------------------------------\r\n");

	printf("Enter any key to continue...");
	getchar();

	gettimeofday(&begin, NULL);
	test4(max, str, sep);
	gettimeofday(&end, NULL);

	spent = stamp_sub(&end, &begin);
	speed = (max * 1000) / (spent >= 1.0 ? spent : 1.0);
	printf("test4 bench: loop=%lld, spent=%.2f ms, speed=%.2f\r\n", max, spent, speed);

	printf("-------------------------------------------------------\r\n");

	printf("Enter any key to continue...");
	getchar();

	gettimeofday(&begin, NULL);
	test5(max, str, sep);
	gettimeofday(&end, NULL);

	spent = stamp_sub(&end, &begin);
	speed = (max * 1000) / (spent >= 1.0 ? spent : 1.0);
	printf("test5 bench: loop=%lld, spent=%.2f ms, speed=%.2f\r\n", max, spent, speed);

	printf("-------------------------------------------------------\r\n");

	printf("Enter any key to continue...");
	getchar();
	test6(str, sep);

	printf("-------------------------------------------------------\r\n");

	test7(str, sep);

	printf("-------------------------------------------------------\r\n");
	test7("hello world ,;?!\t\r\n", ",;?! \t\r\n");

	printf("-------------------------------------------------------\r\n");

	ACL_ARGV_VIEW *view = acl_argv_view_split("", ";");
	ACL_ITER iter;
	printf("argc=%d\r\n", view->argv.argc);

	acl_foreach(iter, view) {
		printf(">>size: %d, i: %d, %s\n", iter.size, iter.i, (const char*) iter.data);
	}
	acl_argv_view_free(view);
	return 0;
}
