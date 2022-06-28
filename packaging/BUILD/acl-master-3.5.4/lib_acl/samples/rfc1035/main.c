#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "lib_acl.h"
#include "rfc1035.h"

static int __qid = 0;

static void build_request(const char *domain)
{
	char *sbuf;
	char buf[1024];
	int ret;

	memset(buf, 0, sizeof(buf));

	ret = rfc1035BuildAQuery(domain, buf, sizeof(buf), __qid++, NULL);
	if (ret <= 0) {
		printf("rfc1035BuildAQuery error, domain=%s\r\n", domain);
		return;
	}

	printf(">>ret=%d\r\n", ret);
	sbuf = (char*) acl_base64_encode(buf, ret);
	printf(">>%s\r\n", sbuf);
	acl_myfree(sbuf);
}

static void parse_result(const char *filepath)
{
	rfc1035_message *answers;
	ssize_t len;
	int   i, ret;
	char *buf = acl_vstream_loadfile2(filepath, &len);

	if (buf == NULL) {
		printf("load from file %s error %s\r\n", filepath, acl_last_serror());
		return;
	}

	ret = rfc1035MessageUnpack(buf, (int) len, &answers);
	if (ret <= 0) {
		printf("invalid answers\r\n");
		if (ret == 0) {
			rfc1035MessageDestroy(answers);
		}
		acl_myfree(buf);
		return;
	}

	for (i = 0; i < ret; i++) {
		if (answers->answer[i].type == RFC1035_TYPE_A) {
			char ip[64];
			struct in_addr sin_addr;
			memcpy(&sin_addr, answers->answer[i].rdata, 4);
			inet_ntop(AF_INET, &sin_addr, ip, sizeof(ip));
			printf(">>ip=%s, ttl=%d\r\n", ip, answers->answer[i].ttl);
		} else if (answers->answer[i].type == RFC1035_TYPE_CNAME) {
			char name[128];
			size_t n = sizeof(name) > answers->answer[i].rdlength
				? answers->answer[i].rdlength : sizeof(name);
			ACL_SAFE_STRNCPY(name, answers->answer[i].rdata, n);
			printf(">>cname=%s, ttl=%d\r\n", name, answers->answer[i].ttl);
		} else {
			printf("not support type=%d\n", answers->answer[i].type);
		}
	}

	acl_myfree(buf);
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -e [encoding] -d [decoding] xxx\r\n", procname);
}

int main(int argc, char *argv[])
{
	int ch, encoding = 1;
	ACL_VSTRING *buf = acl_vstring_alloc(64);

	while ((ch = getopt(argc, argv, "hed")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			acl_vstring_free(buf);
			return 0;
		case 'e':
			encoding = 1;
			break;
		case 'd':
			encoding = 0;
			break;
		default:
			break;
		}
	}

	if (argv[optind] == NULL) {
		usage(argv[0]);
		return 1;
	}

	printf("xx=%s, optind=%d\r\n", argv[optind] ? argv[optind] : "null", optind);
	acl_vstring_strcpy(buf, argv[optind]);

	if (encoding) {
		build_request(acl_vstring_str(buf));
	} else {
		parse_result(acl_vstring_str(buf));
	}

	acl_vstring_free(buf);
	return (0);
}
