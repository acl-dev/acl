#include "lib_acl.h"
#include "lib_protocol.h"

static void *test_thread(void *dummy acl_unused)
{
	printf("last error: %s\r\n", acl_last_serror());
	return 0;
}

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

static int header_http(const char* filepath, int max)
{
	struct timeval begin, end;
	double n;
	ACL_VSTREAM* fp;
	int   i, ret;

	fp = acl_vstream_fopen(filepath, O_RDONLY, 0600, 8192);
	if (fp == NULL)
	{
		printf("open file %s error\r\n", filepath);
		return 0;
	}

	gettimeofday(&begin, NULL);
	for (i = 0; i < max; i++)
	{
		HTTP_HDR_REQ* hdr_req = http_hdr_req_new();

		if (acl_vstream_fseek(fp, 0, SEEK_SET) < 0)
		{
			printf("fseek error\r\n");
			break;
		}
		if ((ret = http_hdr_req_get_sync(hdr_req, fp, 0)) < 0)
		{
			http_hdr_req_free(hdr_req);
			printf("get header error: %d\r\n", ret);
			break;
		}
		if (http_hdr_req_parse(hdr_req) < 0)
		{
			printf("parse error\r\n");
			http_hdr_req_free(hdr_req);
			break;
		}

		http_hdr_req_free(hdr_req);

		if (i % 1000 == 0)
		{
			char  line[64];
			snprintf(line, sizeof(line), "total: %d, curr: %d", max, i);
			ACL_METER_TIME(line);
		}
	}

	gettimeofday(&end, NULL);
	n = stamp_sub(&end, &begin);

	printf("total: %d, spent: %0.2f, speed: %0.2f\r\n",
		max, n, (max * 1000) /(n > 0 ? n : 1));

	acl_vstream_close(fp);
	return 0;
}

static int header_read(const char* filepath, int max)
{
	struct timeval begin, end;
	double n;
	ACL_VSTREAM* fp;
	char  line[1024];
	int   i;

	fp = acl_vstream_fopen(filepath, O_RDONLY, 0600, 8192);
	if (fp == NULL)
	{
		printf("open file %s error\r\n", filepath);
		return 0;
	}

	gettimeofday(&begin, NULL);
	for (i = 0; i < max; i++)
	{
		if (acl_vstream_fseek(fp, 0, SEEK_SET) < 0)
		{
			printf("fseek error\r\n");
			break;
		}
		while (1)
		{
			if (acl_vstream_gets_nonl(fp, line, sizeof(line)) == ACL_VSTREAM_EOF)
				break;
		}

		if (i % 1000 == 0)
		{
			snprintf(line, sizeof(line), "total: %d, curr: %d", max, i);
			ACL_METER_TIME(line);
		}
	}

	gettimeofday(&end, NULL);
	n = stamp_sub(&end, &begin);

	printf("total: %d, spent: %0.2f, speed: %0.2f\r\n",
		max, n, (max * 1000) /(n > 0 ? n : 1));

	acl_vstream_close(fp);
	return 0;
}

static int header_parse(const char* filepath, int max)
{
	int i;
	double n;
	ACL_VSTREAM* fp;
	ACL_ITER iter;
	struct timeval begin, end;
	char  line[1024];
	ACL_ARGV* tokens = acl_argv_alloc(10);

	fp = acl_vstream_fopen(filepath, O_RDONLY, 0600, 8192);
	if (fp == NULL)
	{
		printf("open file %s error\r\n", filepath);
		return 0;
	}

	while (1)
	{
		i = acl_vstream_gets_nonl(fp, line, sizeof(line));
		if (i == 0 || i == ACL_VSTREAM_EOF)
			break;
		acl_argv_add(tokens, line, NULL);
	}

	acl_foreach(iter, tokens)
	{
		printf("%s\r\n", (const char*) iter.data);
	}

	printf("----------------------------------------------------\r\n");

	gettimeofday(&begin, NULL);

#define	HDR_RES

	ACL_METER_TIME("begin run");
	for (i = 0; i < max; i++)
	{
#ifdef	HDR_RES
		HTTP_HDR_RES* hdr;
#else
		HTTP_HDR_REQ* hdr;
#endif
		int j = 0;

#ifdef	HDR_RES
		hdr = http_hdr_res_new();
#else
		hdr = http_hdr_req_new();
#endif
		acl_foreach(iter, tokens)
		{
			HTTP_HDR_ENTRY* entry =
				http_hdr_entry_new((const char*) iter.data);
			http_hdr_append_entry(&hdr->hdr, entry);
			if (++j == 200)
				break;
		}

#ifdef	HDR_RES
		http_hdr_res_free(hdr);
#else
		http_hdr_req_free(hdr);
#endif

		if (i % 1000 == 0)
		{
			snprintf(line, sizeof(line), "total: %d, curr: %d", max, i);
			ACL_METER_TIME(line);
		}
	}

	gettimeofday(&end, NULL);
	n = stamp_sub(&end, &begin);

	printf("total: %d, spent: %0.2f, speed: %0.2f\r\n",
		max, n, (max * 1000) /(n > 0 ? n : 1));
	acl_argv_free(tokens);
	acl_vstream_fclose(fp);

	return 0;
}

static void usage(const char* procname)
{
	printf("usage: %s -h [help] -n max -f header_file -a action [parse|read|http]\r\n", procname);
}

int main(int argc, char* argv[])
{
	int   ch, max = 10;
	char  filepath[256], action[64];

	if (0)
		acl_mem_slice_init(8, 1024, 100000,
			ACL_SLICE_FLAG_GC2 |
			ACL_SLICE_FLAG_RTGC_OFF |
			ACL_SLICE_FLAG_LP64_ALIGN);

	filepath[0] = 0;
	action[0] = 0;

	while ((ch = getopt(argc, argv, "hn:f:a:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			max = atoi(optarg);
			if (max <= 0)
				max = 10;
			break;
		case 'f':
			snprintf(filepath, sizeof(filepath), "%s", optarg);
			break;
		case 'a':
			snprintf(action, sizeof(action), "%s", optarg);
			break;
		default:
			break;
		}
	}

	if (filepath[0] == 0)
	{
		usage(argv[0]);
		return 0;
	}

	if (1)
	{
		acl_pthread_t tid;
		acl_pthread_attr_t attr;

		acl_pthread_attr_init(&attr);
		acl_pthread_create(&tid, &attr, test_thread, NULL);
		acl_pthread_join(tid, NULL);
	}
	printf("last error: %s\r\n", acl_last_serror());
	if (strcasecmp(action, "parse") == 0)
		return header_parse(filepath, max);
	else if (strcasecmp(action, "read") == 0)
		return header_read(filepath, max);
	else if (strcasecmp(action, "http") == 0)
		return header_http(filepath, max);
	else
	{
		printf("unknown action: %s\r\n", action);
		usage(argv[0]);
	}
	return 0;
}
