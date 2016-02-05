#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"
#include <stdio.h>
#include <string>

static void test(acl::string& buf)
{
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";

	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";

	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";

	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
	buf += "hello world!";
}

static void test_mmap(int max)
{
	const char* filename = "local.map";
	acl::fstream fp;
	if (fp.open(filename, O_RDWR | O_CREAT | O_TRUNC, 0600) == false)
	{
		printf("open %s error %s\r\n", filename, acl::last_serror());
		return;
	}

	size_t max_len = 1024 * 1024 * 500, init_len = 4096;
	acl::string buf(fp.file_handle(), max_len, init_len);

	ACL_METER_TIME(">> begin acl::string");

	for (int i = 0; i < max; i++)
	{
		test(buf);
	}

	ACL_METER_TIME(">> end acl::string");
}

static void test_mem(int max)
{
	size_t max_len = 1024 * 1024 * 500, init_len = 4096;
	acl::string buf(init_len);
	buf.set_max(max_len);

	ACL_METER_TIME(">> begin acl::string");

	for (int i = 0; i < max; i++)
	{
		test(buf);
	}

	ACL_METER_TIME(">> end acl::string");
}

static void usage(const char* procname)
{
	printf("usage: %s -h [help] -m [use_mmap] -n max_count\r\n", procname);
}

int main(int argc, char* argv[])
{
	bool use_mmap = false;
	int ch, max = 100000;

	while ((ch = getopt(argc, argv, "hmn:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 'm':
			use_mmap = true;
			break;
		case 'n':
			max = atoi(optarg);
			break;
		default:
			break;
		}
	}

	if (use_mmap)
		test_mmap(max);
	else
		test_mem(max);

	return (0);
}
