#include "lib_acl.h"
#include "../stamp.h"

static void test1(long long max, const char *str, const char *sep)
{
	long long i;

	for (i = 0; i < max; i++) {
		ACL_ARGV *argv = acl_argv_split(str, sep);
		acl_argv_free(argv);
	}
}

static void argv_extend(ACL_ARGV *argv)
{
	argv->len = argv->len * 2;
	argv->argv = (char**) acl_myrealloc((char*) argv->argv,
		(argv->len + 1) * sizeof(char*));
}

#define SPACE_LEFT(a) ((a)->len - (a)->argc - 1)

static ACL_ARGV *split(const char *str, const char *sep)
{
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

static void test2(long long max, const char *str, const char *sep)
{
	long long i;

	for (i = 0; i < max; i++) {
		ACL_ARGV *argv = split(str, sep);
		acl_argv_free(argv);
	}
}

static void test3(const char *str, const char *sep)
{
	ACL_ITER iter;
	ACL_ARGV *argv = split(str, sep);

	acl_foreach(iter, argv) {
		const char *ptr = (const char*) iter.data;
		printf("%s\r\n", ptr);
	}

	acl_argv_free(argv);
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		" -n max_loop[default: 10000]\r\n"
		, procname);
}

int main(int argc, char *argv[])
{
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
	printf("loop=%lld, spent=%.2f ms, speed=%.2f\r\n", max, spent, speed);

	/*------------------------------------------------------------------*/

	printf("Enter any key to continue...");
	getchar();

	gettimeofday(&begin, NULL);
	test2(max, str, sep);
	gettimeofday(&end, NULL);

	spent = stamp_sub(&end, &begin);
	speed = (max * 1000) / (spent >= 1.0 ? spent : 1.0);
	printf("loop=%lld, spent=%.2f ms, speed=%.2f\r\n", max, spent, speed);

	test3(str, sep);

	printf("-------------------------------------------------------\r\n");
	test3("hello world ,;?!\t\r\n", ",;?! \t\r\n");

	return 0;
}
