#include "lib_acl.h"

#if defined(_WIN32) || defined(_WIN64)
#define snprintf _snprintf
#endif

static void test_file(ACL_FILE_HANDLE fd, ssize_t max)
{
	ssize_t i;
	char buf[1];
	buf[0] = 'X';

	for (i = 0; i < max; i++) {
		if (acl_file_write(fd, buf, 1, 0, NULL, NULL) != 1) {
			printf("write error: %s\r\n", acl_last_serror());
			return;
		}
	}
}

static void usage(const char* procname)
{
	printf("usage: %s -h[help] -f filename -n max_size\r\n", procname);
}

int main(int argc, char *argv[])
{
	char  filename[256];
	int   ch;
	ssize_t max = 8192;
	ACL_FILE_HANDLE fd;

	snprintf(filename, sizeof(filename), "test.txt");

	while ((ch = getopt(argc, argv, "hf:n:d")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'f':
			snprintf(filename, sizeof(filename), "%s", optarg);
			break;
		case 'n':
			max = atol(optarg);
			break;
		default:
			break;
		}
	}

	acl_msg_stdout_enable(1);

	printf("filename: %s, max size: %ld\r\n", filename, (long) max);

	fd = acl_file_open(filename, O_RDWR | O_CREAT | O_APPEND, 0600);

	if (fd  == ACL_FILE_INVALID) {
		printf("open %s error %s\r\n", filename, acl_last_serror());
		return 1;
	}

	test_file(fd, max);

	acl_file_close(fd);
	return 0;
}
