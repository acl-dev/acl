// http_rpc_client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "acl_cpp/acl_cpp_init.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/http/http_request.hpp"
#include "google/protobuf/io/http_stream.h"

#include "util.h"
#include "test.pb.h"

using namespace google::protobuf::io;

static double __request_spent, __response_spent, __build_spent, __parse_spent;

static bool handle_one(http_request& rpc, bool output)
{
	//////////////////////////////////////////////////////////////////
	// 请求过程

	static tutorial::AddressBook address;
	int  person_count = 5;

	address.Clear();

	acl::string buf;

	// 给地址簿中添加用户列表
	for (int i = 0; i < person_count; i++)
	{
		tutorial::Person* person = address.add_person();
		buf.format("zsxxsz-%d", i);
		person->set_name(buf.c_str());
		buf.format("zsxxsz-%d@test.com", i);
		person->set_email(buf.c_str());
		person->set_id(i);

		// 给一个用户添加多个电话号码
		for (int j = 0; j < tutorial::Person::WORK; j++)
		{
			tutorial::Person::PhoneNumber* phone = person->add_phone();
			buf.format("11111111-%d-%d", i, j);
			phone->set_number(buf.c_str());
		}
	}

	//////////////////////////////////////////////////////////////////
	// 发送请求数据至服务端，并读取服务端的响应

	static tutorial::AddressBook address_result;
	address_result.Clear();

	if (rpc.rpc_request(address, &address_result) == false)
	{
		printf("rpc_request error\r\n");
		return false;
	}

	__request_spent += rpc.request_spent();
	__response_spent += rpc.response_spent();
	__build_spent += rpc.build_spent();
	__parse_spent += rpc.parse_spent();

	//////////////////////////////////////////////////////////////////
	// 分析响应数据

	// 列出地址簿中所有用户信息
	for (int i = 0; i < address_result.person_size(); i++)
	{
		const tutorial::Person& person = address_result.person(i);
		if (output)
		{
			printf("person->name: %s\r\n", person.name().c_str());
			printf("person->id: %d\r\n", person.id());
			printf("person->email: %s\r\n", person.has_email() ?
				person.email().c_str() : "null");
		}

		// 列出该用户的所有电话
		for (int j = 0; j < person.phone_size(); j++)
		{
			const tutorial::Person::PhoneNumber& phone = person.phone(j);
			if (output)
				printf("\tphone number: %s\r\n", phone.number().c_str());
		}
		if (output)
			printf("------------------------------------------\r\n");
	}

	return true;
}

static void handle_rpc(const char* addr, int max)
{
	http_request rpc(addr);

	struct timeval begin;
	gettimeofday(&begin, NULL);

	char  buf[64];
	ACL_METER_TIME("begin run");
	for (int i = 0; i < max;)
	{
		if (handle_one(rpc, i >= 1 ? false : true) == false)
			break;
		if (++i % 1000 == 0)
		{
			snprintf(buf, sizeof(buf), "total count: %d, curr: %d", max, i);
			ACL_METER_TIME(buf);
		}
	}

	struct timeval end;
	gettimeofday(&end, NULL);
	double n = util::stamp_sub(&end, &begin);
	printf("total check: %d, spent: %0.2f ms, speed: %0.2f\r\n"
		"total_build: %0.2f, total_parse: %0.2f, "
		"total_request: %0.2f, total_reponse: %0.2f\r\n",
		max, n, (max * 1000) /(n > 0 ? n : 1),
		__build_spent, __parse_spent,
		__request_spent, __response_spent);
}

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		"-s server_addr [default: 127.0.0.1:8888]\r\n"
		"-n max_request\r\n"
		"-m [use_mempool, default: false]\r\n", procname);
}

int main(int argc, char* argv[])
{

	char  addr[64];
	int   ch, max = 1;
	bool  use_mempool = false;

	snprintf(addr, sizeof(addr), "127.0.0.1:8888");

	while ((ch = getopt(argc, argv, "hs:n:m")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			snprintf(addr, sizeof(addr), "%s", optarg);
			break;
		case 'n':
			max = atoi(optarg);
			if (max <= 0)
				max = 1;
			break;
		case 'm':
			use_mempool = true;
			break;
		default:
			break;
		}
	}

#ifdef WIN32
	acl::acl_cpp_init();
#endif

	if (use_mempool)
		acl_mem_slice_init(8, 1024, 100000,
			ACL_SLICE_FLAG_GC2 |
			ACL_SLICE_FLAG_RTGC_OFF |
			ACL_SLICE_FLAG_LP64_ALIGN);

	handle_rpc(addr, max);

	// Optional:  Delete all global objects allocated by libprotobuf.
	google::protobuf::ShutdownProtobufLibrary();

#ifdef WIN32
	printf("Enter any key to exit\r\n");
	getchar();
#endif
	return 0;
}
