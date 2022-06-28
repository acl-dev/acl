#include "lib_acl.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#define	FMT_ERR		-1
#define	FMT_UNIX	0
#define	FMT_DOS		1
#define	FMT_MAC		2

static int __total_c_line = 0;
static int __total_h_line = 0;
static int __total_cpp_line = 0;
static int __total_hpp_line = 0;

static void fmt_change(const char *filepath, const char *fmt)
{
	const char *myname = "fmt_change";
	ACL_VSTREAM *fp;
	char *buf, *ptr;
	const char *tmp;
	int   n, ret;

	buf = acl_vstream_loadfile(filepath);
	if (buf == NULL)
		acl_msg_fatal("%s: loadfile(%s, %s)",
			myname, filepath, strerror(errno));

	fp = acl_vstream_fopen(filepath, O_WRONLY | O_TRUNC, 0600, 4096);
	if (fp == NULL)
		acl_msg_fatal("%s: open file(%s, %s)",
			myname, filepath, strerror(errno));

	ptr = buf;

	while (*ptr) {
		n = 0;
		if (*ptr == '\r') {
			n++;
			ptr++;
			if (*ptr == '\n') {
				n++;
				ptr++;
				if (strstr(filepath, "samples") == NULL
				    && strstr(filepath, "unit_test") == NULL)
				{
					if (strrncasecmp(filepath, ".c", 2) == 0)
						__total_c_line++;
					else if (strrncasecmp(filepath, ".cpp", 4) == 0)
						__total_cpp_line++;
					else if (strstr(filepath, "mysql") == NULL
						&& strstr(filepath, "openssl") == NULL
						&& strstr(filepath, "dist") == NULL
						&& strstr(filepath, "bdb") == NULL
						&& strstr(filepath, "tc") == NULL
						&& strstr(filepath, "google") == NULL
						&& strstr(filepath, "iconv") == NULL
						&& strstr(filepath, "polarssl") == NULL
						&& strstr(filepath, "sqlite") == NULL
						&& strstr(filepath, "zlib") == NULL
						&& strstr(filepath, "cdb") == NULL)
					{
						if (strrncasecmp(filepath, ".h", 2) == 0)
							__total_h_line++;
						else if (strrncasecmp(filepath, ".hpp", 4) == 0)
							__total_hpp_line++;
					}
				}
			}
		} else if (*ptr == '\n') {
			n++;
			ptr++;
			if (strstr(filepath, "samples") == NULL
					&& strstr(filepath, "unit_test") == NULL)
			{
				if (strrncasecmp(filepath, ".c", 2) == 0)
					__total_c_line++;
				else if (strrncasecmp(filepath, ".cpp", 4) == 0)
					__total_cpp_line++;
				else if (strstr(filepath, "mysql") == NULL
					&& strstr(filepath, "openssl") == NULL
					&& strstr(filepath, "dist") == NULL
					&& strstr(filepath, "bdb") == NULL
					&& strstr(filepath, "tc") == NULL
					&& strstr(filepath, "google") == NULL
					&& strstr(filepath, "iconv") == NULL
					&& strstr(filepath, "polarssl") == NULL
					&& strstr(filepath, "sqlite") == NULL
					&& strstr(filepath, "zlib") == NULL
					&& strstr(filepath, "cdb") == NULL)
				{
					if (strrncasecmp(filepath, ".h", 2) == 0)
						__total_h_line++;
					else if (strrncasecmp(filepath, ".hpp", 4) == 0)
						__total_hpp_line++;
				}
			}
		}
		if (n) {
			tmp = fmt;
			n = strlen(tmp);
		} else {
			tmp = ptr++;
			n = 1;
		}
		ret = acl_vstream_buffed_writen(fp, tmp, n);
		if (ret == ACL_VSTREAM_EOF)
			acl_msg_fatal("%s: write to %s error(%s)",
				myname, filepath, strerror(errno));
	}

	if (acl_vstream_fflush(fp) == ACL_VSTREAM_EOF)
		acl_msg_fatal("%s: fflush to %s error(%s)",
			myname, filepath, strerror(errno));
	acl_vstream_close(fp);
	acl_myfree(buf);
}

static void scan_dir(const char *src_path, int to_fmt)
{
	ACL_SCAN_DIR *scan;
	const char *filename, *path, *ptr;
	char  filepath[1024], fmt_buf[32], info[32];
	int   n;
	
#define	CP	ACL_SAFE_STRNCPY

	switch (to_fmt) {
	case FMT_UNIX:
		CP(info, "UNIX", sizeof(info));
		CP(fmt_buf, "\n", sizeof(fmt_buf));
		break;
	case FMT_DOS:
		CP(info, "DOS", sizeof(info));
		CP(fmt_buf, "\r\n", sizeof(fmt_buf));
		break;
	case FMT_MAC:
		CP(info, "MAC", sizeof(info));
		CP(fmt_buf, "\r", sizeof(fmt_buf));
		break;
	default:
		acl_msg_fatal("unknown fmt(%d)", to_fmt);
	}
	
	scan = acl_scan_dir_open(src_path, 1);
	if (scan == NULL)
		acl_msg_fatal("open path(%s, %s)", src_path, strerror(errno));

	n = 0;
	while (1) {
		filename = acl_scan_dir_next_file(scan);
		if (filename == NULL)
			break;
		if (strlen(filename) < 3)
			continue;
		ptr = strchr(filename, '.');
		if (ptr == NULL)
			continue;
		ptr++;
		if (strcasecmp(ptr, "c") != 0 && strcasecmp(ptr, "h") != 0
		    && strcasecmp(ptr, "cpp") != 0 && strcasecmp(ptr, "hpp") != 0)
			continue;

		path = acl_scan_dir_path(scan);
		if (path == NULL)
			acl_msg_fatal("file(%s) no path", filename);
		snprintf(filepath, sizeof(filepath), "%s/%s",
			path, filename);
		printf("filepath:%s\r\n", filepath);
		fmt_change(filepath, fmt_buf);
		n++;
	}
	
	acl_scan_dir_close(scan);

	printf(">>> At last, %d files were changed to %s format\r\n"
		">>> total_c_line: %d lines, total_h_line: %d, "
		"total_cpp_line: %d, total_hpp_line: %d\r\n",
		n, info, __total_c_line, __total_h_line,
		__total_cpp_line, __total_hpp_line);
}

static void usage(const char *progname)
{
	printf("usage: %s -h help\r\n"
			"-f file_format[unix|dos|mac]\r\n"
			"-d dir_path\r\n", progname);
}

int main(int argc, char *argv[])
{
	char  ch;
	char *src_path = NULL, *ptr;
	int   to_fmt = FMT_UNIX;


	while ((ch = getopt(argc, argv, "hf:d:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			exit (0);
		case 'f':
			if (strcasecmp(optarg, "unix") == 0)
				to_fmt = FMT_UNIX;
			else if (strcasecmp(optarg, "dos") == 0)
				to_fmt = FMT_DOS;
			else if (strcasecmp(optarg, "mac") == 0)
				to_fmt = FMT_MAC;
			else
				to_fmt = FMT_ERR;
			break;
		case 'd':
			src_path = acl_mystrdup(optarg);
			break;
		default:
			usage(argv[0]);
			exit (0);
		}
	}
	
	if (to_fmt == FMT_ERR || src_path == NULL || *src_path == 0) {
		usage(argv[0]);
		exit (0);
	}

	ptr = src_path + strlen(src_path) - 1;
	while (*ptr == '/') {
		*ptr-- = 0;
	}

	scan_dir(src_path, to_fmt);

	return (0);
}
