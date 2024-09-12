#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "lib_acl.h"

static void b64_encode(const char *ptr)
{
	ACL_VSTRING *str = acl_vstring_alloc(1);

	acl_vstring_base64_encode(str, ptr, strlen(ptr));

	printf(">>>encode result:%s\n", acl_vstring_str(str));

	acl_vstring_free(str);
}

static void b64_decode(const char *ptr, int verbose)
{
	ACL_VSTRING *str = acl_vstring_alloc(1);
	const char *p;

#define DECODE(b,x,l) { \
	if (acl_vstring_base64_decode((b),(x),(l)) == 0) { \
		printf("bad base64 encoded string: %s\r\n", (x)); \
		exit (1); \
	} \
}
	DECODE(str, ptr, strlen(ptr));

	p = acl_vstring_str(str);
	if (verbose) {
		printf(">>>decode result:|%s|\n>>>orignal str: {%s}, len: %d\n",
			p, ptr, (int) ACL_VSTRING_LEN(str));
	} else {
		printf("%s\n", p);
	}

	if (verbose) {
		b64_encode(p);
		printf("len: %d, %d\n", (int) ACL_VSTRING_LEN(str), (int) strlen(p));
	}

	acl_vstring_free(str);
}

static void bin_encode(const char *ptr, size_t len)
{
	ACL_VSTRING *vbuf = acl_vstring_alloc(128);
	if (acl_hex_encode(vbuf, ptr, (int) len)) {
		printf("Bin encode result: %s\r\n", acl_vstring_str(vbuf));
	} else {
		printf("Bin encode error for: %s\r\n", ptr);
	}
	acl_vstring_free(vbuf);
}

static void bin_decode(const char *ptr, size_t len)
{
	ACL_VSTRING *vbuf = acl_vstring_alloc(128);
	if (acl_hex_decode(vbuf, ptr, (int) len)) {
		printf("Bin decode retult: %s\r\n", acl_vstring_str(vbuf));
	} else {
		printf("Bin decode error for: %s\r\n", ptr);
	}
	acl_vstring_free(vbuf);
}

static void crc32_encode(const char *ptr, size_t len)
{
	unsigned hash = acl_hash_crc32(ptr, len);
	printf("hash result: %u\r\n", hash);
}

static char *load_from(const char *filename)
{
	char *data = acl_vstream_loadfile(filename);
	return data;
	/*
	char *ptr = strchr(data, '\r');
	if (ptr == NULL) {
		ptr = strchr(data, '\n');
	}
	if (ptr) {
		*ptr = 0;
	}
	return data;
	*/
}

static void usage(const char *prog)
{
	printf("usage: %s -s string\r\n"
		" -a action[default: encode, encode|decode]\r\n"
		" -t type [default: base64, base64|bin|crc32]\r\n"
		" -f filename\r\n"
		" -s string\r\n"
		" -V [verbose]\r\n"
		" -h help]\n", prog);
}

#define BASE64_TYPE	1
#define BIN_TYPE	2
#define CRC32_TYPE	3

int main(int argc, char *argv[])
{
	int ch, encode = 1, type = BASE64_TYPE, verbose = 0;
	char *buf = NULL;

	if (argc == 1) {
		usage(argv[0]);
		exit (1);
	}

	while ((ch = getopt(argc, argv, "s:f:a:t:hV")) > 0) {
		switch (ch) {
		case 's':
			buf = acl_mystrdup(optarg);
			break;
		case 'f':
			buf = load_from(optarg);
			printf("buf=%s\n", buf);
			break;
		case 'a':
			if (strcasecmp(optarg, "decode") == 0) {
				encode = 0;
			}
			break;
		case 't':
			if (strcasecmp(optarg, "bin") == 0) {
				type = BIN_TYPE;
			} else if (strcasecmp(optarg, "crc32") == 0) {
				type = CRC32_TYPE;
			}
			break;
		case 'h':
			usage(argv[0]);
			return 0;
		case 'V':
			verbose = 1;
			break;
		default:
			usage(argv[0]);
			return 0;
		}
	}

	if (buf == NULL) {
		printf("Not found -s for string\r\n");
		usage(argv[0]);
		return 0;
	}

	switch (type) {
	case BASE64_TYPE:
		if (encode) {
			b64_encode(buf);
		} else {
			b64_decode(buf, verbose);
		}
		break;
	case BIN_TYPE:
		if (encode) {
			bin_encode(buf, strlen(buf));
		} else {
			bin_decode(buf, strlen(buf));
		}
		break;
	case CRC32_TYPE:
		crc32_encode(buf, strlen(buf));
		break;
	default:
		break;
	}

	if (buf) {
		acl_myfree(buf);
	}

#if 0
	{
		char* p = "²Ë";
		char tmp[32];
		strcpy(tmp, p);
		p = tmp;
		p++;
		*p = 0;
		b64_encode(tmp);
		strcpy(tmp, "²ËÆ×");
		p = tmp;
		p++;
		b64_encode(p);
	}
#endif

	return 0;
}
