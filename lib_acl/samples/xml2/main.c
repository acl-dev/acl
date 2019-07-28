#include "lib_acl.h"

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

static void usage(const char* proc)
{
	printf("usage: %s -h[help] -m[use_memslice] -c cache_count -f filepath\r\n", proc);
}

int main(int argc, char* argv[])
{
	char  buf[8192], filepath[256];
	int   ret, n;
	ACL_VSTREAM* fp;
	ACL_XML* xml;
	struct timeval begin, end;
	double spent;
	int   ch, use_slice = 0, cache_count = 1000;

	filepath[0] = 0;
	while ((ch = getopt(argc, argv, "hmc:f:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 'm':
			use_slice = 1;
			break;
		case 'c':
			cache_count = atoi(optarg);
			if (cache_count <= 0)
				cache_count = 1000;
			break;
		case 'f':
			snprintf(filepath, sizeof(filepath), "%s", optarg);
			break;
		default:
			break;
		}
	}

	if (use_slice)
		acl_mem_slice_init(8, 1024, 100000,
			ACL_SLICE_FLAG_GC2 |
			ACL_SLICE_FLAG_RTGC_OFF |
			ACL_SLICE_FLAG_LP64_ALIGN);

	if (filepath[0] == 0)
	{
		usage(argv[0]);
		return 1;
	}

	xml = acl_xml_alloc();
	if (cache_count > 0)
		acl_xml_cache(xml, cache_count);

	fp = acl_vstream_fopen(filepath, O_RDONLY, 0600, 8192);
	if (fp == NULL)
	{
		printf("open file %s error %s\r\n",
			filepath, acl_last_serror());
		acl_xml_free(xml);
		return 1;
	}

	gettimeofday(&begin, NULL);
	n = 0;
	ACL_METER_TIME("------begin------");
	while (1)
	{
		ret = acl_vstream_fgets(fp, buf, sizeof(buf) - 1);
		if (ret == ACL_VSTREAM_EOF)
			break;
		buf[ret] = 0;
		acl_xml_parse(xml, buf);
		if (++n % 10000 == 0)
		{
			printf("line: %d\r\n", n);
			ACL_METER_TIME("-------ok------");
		}
		if (n % cache_count == 0)
		{
			printf("reset xml, line: %d\r\n", n);
			acl_xml_reset(xml);
		}
	}

	gettimeofday(&end, NULL);
	spent = stamp_sub(&end, &begin);
	printf("\r\ntotal spent: %0.2f ms\r\n", spent);

	acl_xml_free(xml);
	acl_vstream_fclose(fp);
	return 0;
}
