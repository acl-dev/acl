#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"
#include <stdio.h>
#include <string>
#include <iostream>

static void test_append(acl::string& buf)
{
	for (size_t i = 0; i < 50; i++) {
		buf.format_append("%zd->hello world!\r\n", i);
	}
}

static void test_mmap_write(int max, size_t off)
{
	const char* filename = "local.map";
	acl::fstream fp;
	if (!fp.open(filename, O_RDWR | O_CREAT /* | O_TRUNC */, 0600))
	{
		printf("open %s error %s\r\n", filename, acl::last_serror());
		return;
	}

	size_t max_len = 1024 * 1024 * 500, init_len = 4096;
	acl::string buf(fp.file_handle(), max_len, init_len, off);

	ACL_METER_TIME(">> begin acl::string");

	for (int i = 0; i < max; i++)
	{
		test_append(buf);
	}

	ACL_METER_TIME(">> end acl::string");
}

static void test_mmap_read(size_t off)
{
	const char* filename = "local.map";
	acl::fstream fp;
	if (!fp.open(filename, O_RDWR | O_CREAT, 0600))
	{
		printf("open %s error %s\r\n", filename, acl::last_serror());
		return;
	}

	size_t max_len = 1024 * 1024 * 500, init_len = 0;
	acl::string buf(fp.file_handle(), max_len, init_len, off);

	char tmp[256];
	memcpy(tmp, buf.c_str(), sizeof(tmp) - 1);
	tmp[sizeof(tmp) - 1] = 0;
	printf("[%s]\r\n", tmp);
}

static void test_mem(int max)
{
	size_t max_len = 1024 * 1024 * 500, init_len = 4096;
	acl::string buf(init_len);
	buf.set_max(max_len);

	ACL_METER_TIME(">> begin acl::string");

	for (int i = 0; i < max; i++)
	{
		test_append(buf);
	}

	ACL_METER_TIME(">> end acl::string");
}

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		" -m [use_mmap]\r\n"
		" -o mapped_offset[default 0]\r\n"
		" -R [mapping for reading]\r\n"
		" -n max_count\r\n"
		, procname);
}

int main(int argc, char* argv[])
{
	bool use_mmap = false, mapped_read = false;
	int ch, max = 100000;
	size_t mapped_off = 0;

	while ((ch = getopt(argc, argv, "hmn:o:R")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 'm':
			use_mmap = true;
			break;
		case 'o':
			mapped_off = (size_t) atoi(optarg);
			break;
		case 'R':
			mapped_read = true;
			break;
		case 'n':
			max = atoi(optarg);
			break;
		default:
			break;
		}
	}

	if (!use_mmap)
		test_mem(max);
	else if (mapped_read)
		test_mmap_read(mapped_off);
	else
		test_mmap_write(max, mapped_off);

	acl::string buf("hello world!");

	// XXX: fixme?
	// std::cout << buf << std::endl;

	std::string tmp = buf;
	std::cout << tmp << std::endl;
	return (0);
}
