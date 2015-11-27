#include <sys/mman.h>
#include <sys/stat.h>
#include "lib_acl.h"

static int build_xml(ACL_VSTREAM *fp, int max_size)
{
	char  buf[8192];
	int   n;

	memset(buf, 'X', sizeof(buf));

	if (acl_vstream_fprintf(fp, "%s", "<root>\r\n") == ACL_VSTREAM_EOF) {
		printf("%s(%d): write error\r\n", __FUNCTION__, __LINE__);
		return -1;
	}

	while (max_size > 0) {
		n = (int) sizeof(buf) > max_size ? max_size : (int) sizeof(buf);
		if (acl_vstream_writen(fp, buf, n) == ACL_VSTREAM_EOF) {
			printf("%s(%d): write error\r\n", __FUNCTION__, __LINE__);
			return -1;
		}
		max_size -= n;
	}

	if (acl_vstream_fprintf(fp, "%s", "</root>\r\n") == ACL_VSTREAM_EOF) {
		printf("%s(%d): write error\r\n", __FUNCTION__, __LINE__);
		return -1;
	}

	return 0;
}

static void usage(const char *procname)
{
	printf("usage: %s -h[help] -m max_size -f to_file\r\n", procname);
}

int main(int argc, char *argv[])
{
	char  filepath[256];
	int   ch, max_size = 1024000;
	ACL_VSTREAM *fp;

	snprintf(filepath, sizeof(filepath), "./test.xml");

	while ((ch = getopt(argc, argv, "hm:f:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'm':
			max_size = atoi(optarg);
			break;
		case 'f':
			snprintf(filepath, sizeof(filepath), "%s", optarg);
			break;
		default:
			break;
		}
	}

	fp = acl_vstream_fopen(filepath, O_CREAT | O_TRUNC | O_WRONLY, 0600, 8192);
	if (fp == NULL) {
		printf("open %s error %s\r\n", filepath, acl_last_serror());
		return 1;
	}

	if (build_xml(fp, max_size) == 0)
		printf("build_xml to %s ok\r\n", filepath);
	else
		printf("build_xml to %s error\r\n", filepath);

	acl_vstream_close(fp);
	return 0;
}
