#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "lib_acl.h"

static void usage(const char *prog)
{
	printf("usage: %s -e string [-d base64_string, -h help]\n", prog);
}

static void b64_encode(const char *ptr)
{
	ACL_VSTRING *str = acl_vstring_alloc(1);

	acl_vstring_base64_encode(str, ptr, strlen(ptr));

	printf(">>>encode result:%s\n", acl_vstring_str(str));

	acl_vstring_free(str);
}

static void b64_decode(const char *ptr)
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
	printf(">>>decode result:|%s|\n>>>orignal str: {%s}, len: %d\n", p, ptr, (int) ACL_VSTRING_LEN(str));
	b64_encode(p);
	printf("len: %d, %d\n", (int) ACL_VSTRING_LEN(str), (int) strlen(p));

	acl_vstring_free(str);
}

int main(int argc, char *argv[])
{
	char  ch;

	if (argc == 1) {
		usage(argv[0]);
		exit (1);
	}

	while ((ch = getopt(argc, argv, "e:d:h")) > 0) {
		switch (ch) {
			case 'e':
				b64_encode(optarg);
				break;
			case 'd':
				b64_decode(optarg);
				break;
			case 'h':
				usage(argv[0]);
				exit (0);
			default:
				usage(argv[0]);
				exit (1);
		}
	}

	{
		char* p = "²Ë";
		char buf[32];
		strcpy(buf, p);
		p = buf;
		p++;
		*p = 0;
		b64_encode(buf);
		strcpy(buf, "²ËÆ×");
		p = buf;
		p++;
		b64_encode(p);
	}

	exit (0);
}
