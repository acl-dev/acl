// protobuf_client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "acl_cpp/stream/ifstream.hpp"
#include "acl_cpp/stream/ofstream.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "google/protobuf/io/acl_fstream.h"

#include "../util.h"
#include "../test.pb.h"

using namespace google::protobuf::io;

static void usage(const char* procname)
{
	printf("usage: %s -h [help] -n max\r\n", procname);
}

int main(int argc, char* argv[])
{
	int   total = 10000;
	int   ch;

	while ((ch = getopt(argc, argv, "hn:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			total = atoi(optarg);
			if (total <= 0)
				total = 10000;
			break;
		default:
			break;
		}
	}

	const char* path = "test.txt";

	// 将用户的信息序列化输出至文件中

	//打开本地文件进行数据写入
	acl::ofstream out_fp;
	if (out_fp.open_write(path) == false)
	{
		printf("open file %s error\r\n", path);
		return 1;
	}

	// 将文件输出流与系列化输出流进行关联
	acl_ofstream output(&out_fp);

	tutorial::AddressBook address;
	size_t  person_count = 5;

	acl::string buf;

	// 给地址簿中添加用户列表
	for (size_t i = 0; i < person_count; i++)
	{
		tutorial::Person* person = address.add_person();
		buf.format("zsxxsz-%d", (int) i);
		person->set_name(buf.c_str());
		buf.format("zsxxsz-%d@test.com", (int) i);
		person->set_email(buf.c_str());
		person->set_id(i);

		// 给一个用户添加多个电话号码
		for (size_t j = 0; j < tutorial::Person::WORK; j++)
		{
			tutorial::Person::PhoneNumber* phone = person->add_phone();
			buf.format("11111111-%d-%d", (int) i, (int) j);
			phone->set_number(buf.c_str());
		}
	}

	// 将地址簿数据序列化至磁盘文件流中
	address.SerializeToZeroCopyStream(&output);
	if (output.Flush() == false)
	{
		printf("flush failed!\r\n");
		return 1;
	}
	out_fp.close();

	/////////////////////////////////////////////////////////////////////////

	// 从序列化文件中读取用户信息

	// 打开本地文件输入流
	acl::ifstream in_fp;
	if (in_fp.open_read(path) == false)
	{
		printf("open file %s error\r\n", path);
		return 1;
	}

	// 将文件输入流与系列化输入流进行关联
	acl_ifstream input(&in_fp);

	address.Clear();

	// 从文件流中解析地址簿信息
	if (!address.ParseFromZeroCopyStream(&input))
	{
		printf("parse file %s failed\r\n", path);
		return 1;
	}

	// 列出地址簿中所有用户信息
	for (int i = 0; i < address.person_size(); i++)
	{
		const tutorial::Person& person = address.person(i);
		printf("person->name: %s\r\n", person.name().c_str());
		printf("person->id: %d\r\n", person.id());
		printf("person->email: %s\r\n", person.has_email() ?
			person.email().c_str() : "null");

		// 列出该用户的所有电话
		for (int j = 0; j < person.phone_size(); j++)
		{
			const tutorial::Person::PhoneNumber& phone = person.phone(j);
			printf("\tphone number: %s\r\n", phone.number().c_str());
		}
		printf("------------------------------------------\r\n");
	}

	acl::string buf1;
	acl::ifstream::load(path, &buf1);

	struct timeval begin;
	gettimeofday(&begin, NULL);

	ACL_METER_TIME("begin run");
	for (int i = 0; i < total; i++)
	{
		address.Clear();
		std::string data(buf1.c_str(), buf1.length());
		if (address.ParseFromString(data) == false)
		{
			printf("parse failed\r\n");
			break;
		}
		if (i % 1000 == 0)
		{
			char tmp[64];
			snprintf(tmp, sizeof(tmp), "total: %d, curr: %d", total, i);
			ACL_METER_TIME(tmp);
		}
	}

	struct timeval end;
	gettimeofday(&end, NULL);
	double n = util::stamp_sub(&end, &begin);
	printf("total check: %d, spent: %0.2f ms, speed: %0.2f\r\n",
			total, n, (total * 1000) /(n > 0 ? n : 1));

	// Optional:  Delete all global objects allocated by libprotobuf.
	google::protobuf::ShutdownProtobufLibrary();

#ifdef WIN32
	printf("Enter any key to exit\r\n");
	getchar();
#endif

	return 0;
}
