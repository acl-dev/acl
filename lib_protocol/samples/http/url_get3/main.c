#include "lib_acl.h"
#include "lib_protocol.h"

static void get_url(const char *url, const char *dump)
{
	ACL_METER_TIME("---begin----");
	if (dump && *dump)
		http_util_dump_url(url, dump);
	else
		http_util_dump_url_to_stream(url, ACL_VSTREAM_OUT);
	ACL_METER_TIME("----end----");
}

static void usage(const char *procname)
{
	printf("usage: %s -h[help] -r url -f dump_file\n"
		"example: %s -r http://www.sina.com.cn/ -f url_dump.txt\n",
		procname, procname);
}

int main(int argc, char *argv[])
{
	int   ch;
	char  url[256], dump[256];

	acl_lib_init();  /* ³õÊ¼»¯ acl ¿â */

	url[0] = 0;
	dump[0] = 0;
	while ((ch = getopt(argc, argv, "hr:t:f:X:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return (0);
		case 'r':
			ACL_SAFE_STRNCPY(url, optarg, sizeof(url));
			break;
		case 'f':
			ACL_SAFE_STRNCPY(dump, optarg, sizeof(dump));
			break;
		default:
			break;
		}
	}

	if (url[0] == 0) {
		usage(argv[0]);
		return (0);
	}

	get_url(url, dump);
	return (0);
}
