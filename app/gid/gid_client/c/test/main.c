#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "lib_gid.h"

static double stamp_sub(const struct timeval *from, const struct timeval *sub_by)
{
        struct timeval res;

        memcpy(&res, from, sizeof(struct timeval));

        res.tv_usec -= sub_by->tv_usec;
        if (res.tv_usec < 0) {
                --res.tv_sec;
                res.tv_usec += 1000000;
        }
        res.tv_sec -= sub_by->tv_sec;

        return (res.tv_sec * 1000.0 + res.tv_usec/1000.0);
}

static void test_get_gid(const char *tag, int n)
{
	int   i, errnum;
	acl_int64 gid = 0;
	struct timeval begin, end;
	double spent;

	gettimeofday(&begin, NULL);

	for (i = 0; i < n; i++) {
		/* 获得唯一 gid 号 */
		gid = gid_next(tag, &errnum);
		if (gid < 0) {
			printf("error: %d, %s, gid: %lld\r\n",
				errnum, gid_client_serror(errnum), gid);
			break;
		}
		if (n <= 100)
			printf("get gid: %lld\r\n", gid);
		else if (i > 0 && i % 10000 == 0) {
			printf(">> i: %d, gid: %llu\r\n", i, gid);
			ACL_METER_TIME("---------");
		}
	}

	gettimeofday(&end, NULL);

	spent = stamp_sub(&end, &begin);
	printf("total count: %d, curr gid: %lld, spent: %.2f, speed: %.2f\r\n",
		n, gid, spent, (n * 1000) / (spent  < 1 ? 1 : spent));
}

static const char *__tag = NULL;
static int   __n = 0;

static void thread_main(void *ctx acl_unused)
{
	test_get_gid(__tag, __n);
}

static void test_gets_gid(const char *tag, int n)
{
	acl_pthread_pool_t *thrpool = acl_thread_pool_create(10, 30);
	int   i;

	__tag = tag;
	__n = n;

	for (i = 0; i < 5; i++)
		acl_pthread_pool_add(thrpool, thread_main, NULL);
	acl_pthread_pool_destroy(thrpool);
}

static void usage(const char *progname)
{
	printf("usage:  %s -h[help] -s server_addr[127.0.0.1:7072]"
		" -p protocol[cmdline|json|xml|]"
		" -n count[100] -c cmd[get] -m[use mempool]"
		" -t tag[default:sid] -P[use thread pool]\r\n", progname);
}

int main(int argc, char *argv[])
{
	int   ch, n = 100, proto = GID_PROTO_JSON;
	int   use_mempool = 0, use_concurrent = 0;
	char  addr[64], cmd[32], tag[32];

	snprintf(addr, sizeof(addr), "127.0.0.1:7072");
	snprintf(cmd, sizeof(cmd), "get");
	snprintf(tag, sizeof(tag), "default");

	while ((ch = getopt(argc, argv, "hs:p:n:c:t:mP")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return (0);
		case 's':
			snprintf(addr, sizeof(addr), "%s", optarg);
			break;
		case 'p':
			if (strcasecmp(optarg, "cmdline") == 0)
				proto = GID_PROTO_CMDLINE;
			else if (strcasecmp(optarg, "json") == 0)
				proto = GID_PROTO_JSON;
			else if (strcasecmp(optarg, "xml") == 0)
				proto = GID_PROTO_XML;
			else {
				printf("invalid proto: %s\r\n", optarg);
				usage(argv[0]);
				return (0);
			}
			break;
		case 'n':
			n = atoi(optarg);
			break;
		case 'c':
			snprintf(cmd, sizeof(cmd), "%s", optarg);
			break;
		case 't':
			snprintf(tag, sizeof(tag), "%s", optarg);
			break;
		case 'm':
			use_mempool = 1;
			break;
		case 'P':
			use_concurrent = 1;
			break;
		default:
			break;
		}
	}
	if (use_mempool)
		acl_mem_slice_init(8, 1024, 100000,
			ACL_SLICE_FLAG_GC2 |
			ACL_SLICE_FLAG_RTGC_OFF |
			ACL_SLICE_FLAG_LP64_ALIGN);

	/* 初始化 */
	printf("proto: %d, addr: %s\n", proto, addr);
	gid_client_init(proto, addr);

	if (strcasecmp(cmd, "get") == 0) {
		if (use_concurrent)
			test_gets_gid(tag, n);
		else
			test_get_gid(tag, n);
	} else
		usage(argv[0]);

	return (0);
}
