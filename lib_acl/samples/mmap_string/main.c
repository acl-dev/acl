#include "lib_acl.h"

#if defined(_WIN32) || defined(_WIN64)
#define snprintf _snprintf
#endif

static void incr_string(ACL_VSTRING *vp, int len, const char* s, int debug)
{
	int   i;

	printf("max: %ld, len: %ld, cnt: %ld\r\n", (long) vp->maxlen,
		vp->vbuf.len, vp->vbuf.cnt);

	for (i = 0; i < len; i++)
		ACL_VSTRING_ADDCH(vp, 'x');

	if (s && *s)
		acl_vstring_sprintf_append(vp, "%s", s);
	else
		ACL_VSTRING_TERMINATE(vp);

	if (debug)
		printf("[%s]\r\n", acl_vstring_str(vp));

	printf("strlen: %ld, ACL_VSTRING_LEN: %ld, max: %ld\r\n",
		(long) strlen(acl_vstring_str(vp)),
		(long) ACL_VSTRING_LEN(vp), (long) vp->maxlen);

	printf("Enter any key to continue ...\r\n\r\n");
	getchar();

	ACL_VSTRING_RESET(vp);
	ACL_VSTRING_TERMINATE(vp);
}

static void test_string(ACL_FILE_HANDLE fd, ssize_t max, int debug)
{
	ACL_VSTRING *vp = acl_vstring_mmap_alloc(fd, 1, max);
	const char *s = "hello world!";

	printf("-------------------------------------------------------\r\n");
	incr_string(vp, max - 1, NULL, debug);

	printf("-------------------------------------------------------\r\n");
	incr_string(vp, max - 1 - strlen(s), s, debug);

	printf("-------------------------------------------------------\r\n");
	incr_string(vp, max, NULL, debug);

	printf("-------------------------------------------------------\r\n");
	incr_string(vp, max + 2, NULL, debug);

	printf("-------------------------------------------------------\r\n");
	acl_vstring_strcat(vp, s);
	incr_string(vp, max - strlen(s) - 1, NULL, debug);

	printf("-------------------------------------------------------\r\n");
	acl_vstring_strcat(vp, s);
	incr_string(vp, max, NULL, debug);

	printf("-------------------------------------------------------\r\n");
	acl_vstring_strcat(vp, s);
	incr_string(vp, max + 10, NULL, debug);

	printf(">>>[%s]\r\n", acl_vstring_str(vp));
	printf(">>>len: %ld, %ld, %p, %p\r\n",
		(long) strlen(acl_vstring_str(vp)),
		(long) ACL_VSTRING_LEN(vp),
		acl_vstring_str(vp), acl_vstring_end(vp));
	acl_vstring_free(vp);
}

static void usage(const char* procname)
{
	printf("usage: %s -h[help] -f filename -n max_size -d [debug]\r\n", procname);
}

int main(int argc, char *argv[])
{
	char  filename[256];
	int   ch, debug = 0;
	ssize_t max = 8192;
	ACL_FILE_HANDLE fd;

	snprintf(filename, sizeof(filename), "mmap.local");

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
		case 'd':
			debug = 1;
			break;
		default:
			break;
		}
	}

	acl_msg_stdout_enable(1);

	printf("filename: %s, max size: %ld\r\n", filename, (long) max);

	fd = acl_file_open(filename, O_RDWR | O_CREAT | O_TRUNC, 0600);

	if (fd  == ACL_FILE_INVALID) {
		printf("open %s error %s\r\n", filename, acl_last_serror());
		return 1;
	}

	test_string(fd, max, debug);

	acl_file_close(fd);
	return 0;
}
