#include <sys/mman.h>
#include <sys/stat.h>
#include "lib_acl.h"

static int build_xml(ACL_VSTREAM *fp, int nested, int nattrs, int max_size)
{
	char  buf[8192];
	int   n;

	memset(buf, 'X', sizeof(buf));

	if (acl_vstream_buffed_fprintf(fp, "%s", "<?xml version=\"1.0\" ?>\r\n") == ACL_VSTREAM_EOF) {
		printf("%s(%d): write error\r\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if (acl_vstream_buffed_fprintf(fp, "%s", "<root>\r\n") == ACL_VSTREAM_EOF) {
		printf("%s(%d): write error\r\n", __FUNCTION__, __LINE__);
		return -1;
	}

	for (n = 0; n < nested; n++) {
		int   i;
		for (i = 0; i <= n; i++) {
			if (acl_vstream_buffed_writen(fp, "  ", 2) == ACL_VSTREAM_EOF) {
				printf("%s(%d): write space error\r\n",
					__FUNCTION__, __LINE__);
				return -1;
			}
		}

		if (acl_vstream_buffed_fprintf(fp, "<node-%d", n) == ACL_VSTREAM_EOF) {
			printf("%s(%d): write node-%d error",
				__FUNCTION__, __LINE__, n);
			return -1;
		}

		for (i = 0; i < nattrs; i++) {
			if (acl_vstream_buffed_fprintf(fp, " name-%d=\"value-%d\"",
				i, i) == ACL_VSTREAM_EOF) {
				printf("write attr error\r\n");
				return -1;
			}
		}

		if (acl_vstream_buffed_fprintf(fp, ">\r\n") == ACL_VSTREAM_EOF) {
			printf("%s(%d): write node-%d error",
				__FUNCTION__, __LINE__, n);
			return -1;
		}
	}

	while (max_size > 0) {
		n = (int) sizeof(buf) > max_size ? max_size : (int) sizeof(buf);
		if (acl_vstream_buffed_writen(fp, buf, n) == ACL_VSTREAM_EOF) {
			printf("%s(%d): write error\r\n", __FUNCTION__, __LINE__);
			return -1;
		}
		max_size -= n;
	}

	if (acl_vstream_buffed_fputs("", fp) == ACL_VSTREAM_EOF) {
		printf("write line error\r\n");
		return -1;
	}

	for (n = nested - 1; n >= 0; n--) {
		int   i;
		for (i = 0; i <= n; i++) {
			if (acl_vstream_buffed_writen(fp, "  ", 2) == ACL_VSTREAM_EOF) {
				printf("%s(%d): write space error\r\n",
					__FUNCTION__, __LINE__);
				return -1;
			}
		}

		if (acl_vstream_buffed_fprintf(fp, "</node-%d>\r\n", n) == ACL_VSTREAM_EOF) {
			printf("%s(%d): write node-%d error",
				__FUNCTION__, __LINE__, n);
			return -1;
		}
	}

	if (acl_vstream_buffed_fprintf(fp, "%s", "</root>\r\n") == ACL_VSTREAM_EOF) {
		printf("%s(%d): write error\r\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if (acl_vstream_buffed_writen(fp, "\0", 1) == ACL_VSTREAM_EOF) {
		printf("%s(%d): write 0 error\r\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if (acl_vstream_fflush(fp) == ACL_VSTREAM_EOF) {
		printf("%s(%d): fflush error\r\n", __FUNCTION__, __LINE__);
		return -1;
	}

	return 0;
}

static void usage(const char *procname)
{
	printf("usage: %s -h[help] -N nested -A nattrs -m max_size -f to_file\r\n",
		procname);
}

int main(int argc, char *argv[])
{
	char  filepath[256];
	int   ch, max_size = 1024000, nested = 10, nattrs = 10;
	ACL_VSTREAM *fp;

	snprintf(filepath, sizeof(filepath), "./test.xml");

	while ((ch = getopt(argc, argv, "hm:f:N:A:")) > 0) {
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
		case 'N':
			nested = atoi(optarg);
			break;
		case 'A':
			nattrs = atoi(optarg);
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

	if (build_xml(fp, nested, nattrs, max_size) == 0)
		printf("build_xml to %s ok\r\n", filepath);
	else
		printf("build_xml to %s error\r\n", filepath);

	acl_vstream_close(fp);
	return 0;
}
