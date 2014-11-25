// http_rpc_server.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"
#include "google/protobuf/io/http_stream.h"
#include "test.pb.h"

using namespace google::protobuf::io;

static double __header_spent, __body_spent, __parse_spent, __build_spent, __response_spent;

static bool handle_one(http_response& response, bool output)
{
	tutorial::AddressBook addr_req;
	if (response.read_request(&addr_req) == false)
		return false;

	tutorial::AddressBook addr_res;

	for (int i = 0; i < addr_req.person_size(); i++)
	{
		const tutorial::Person& person = addr_req.person(i);
		if (output)
		{
			printf("person->name: %s\r\n", person.name().c_str());
			printf("person->id: %d\r\n", person.id());
			printf("person->email: %s\r\n", person.has_email() ?
				person.email().c_str() : "null");
		}

		tutorial::Person* person2 = addr_res.add_person();
		person2->set_name(person.name().c_str());
		person2->set_email(person.has_email() ?
			person.email().c_str() : "null");
		person2->set_id(i);

		// 列出该用户的所有电话
		for (int j = 0; j < person.phone_size(); j++)
		{
			const tutorial::Person::PhoneNumber& phone = person.phone(j);
			if (output)
				printf("\tphone number: %s\r\n", phone.number().c_str());

			tutorial::Person::PhoneNumber* phone2 = person2->add_phone();
			phone2->set_number(phone.number().c_str());
		}
		if (output)
			printf("------------------------------------------\r\n");
	}

	return response.send_response(addr_res);
}

static void handle_client(acl::socket_stream* client)
{
	acl::http_response res(client);
	http_response response(&res);

	int   i = 0;

	__header_spent = 0;
	__body_spent = 0;
	__parse_spent = 0;
	__build_spent = 0;
	__response_spent = 0;

	char  buf[64];
	ACL_METER_TIME("begin run");
	while (true)
	{
		if (handle_one(response, i > 1 ? false : true) == false)
			break;

		__header_spent += response.header_spent();
		__body_spent += response.body_spent();
		__parse_spent += response.parse_spent();
		__build_spent += response.build_spent();
		__response_spent += response.response_spent();

		if (++i % 1000 == 0)
		{
			snprintf(buf, sizeof(buf), "total: %d", i);
			ACL_METER_TIME(buf);
		}
	}

	printf("total count: %d, header_spent: %0.2f, body_spent: %0.2f, "
		"parse_spent: %0.2f, build_spent: %0.2f, response_spent: %0.2f\r\n",
		i, __header_spent, __body_spent, __parse_spent,
		__build_spent, __response_spent);
}

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		"-s listen_addr [127.0.0.1:8888]\r\n"
		"-m [use_mempool, default: false]\r\n", procname);
}

int main(int argc, char* argv[])
{
	bool  use_mempool = false;
	char  addr[64];
	int   ch;

	snprintf(addr, sizeof(addr), "127.0.0.1:8888");

	while ((ch = getopt(argc, argv, "hs:m")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			snprintf(addr, sizeof(addr), "%s", optarg);
			break;
		case 'm':
			use_mempool = true;
			break;
		default:
			break;
		}
	}

	// 该函数的调用必须在所有使用 acl 的函数之前
	if (use_mempool)
		acl_mem_slice_init(8, 1024, 100000,
			ACL_SLICE_FLAG_GC2 |
			ACL_SLICE_FLAG_RTGC_OFF |
			ACL_SLICE_FLAG_LP64_ALIGN);

#ifdef WIN32
	acl::acl_cpp_init();
#endif

	acl::server_socket server;
	if (server.open(addr) == false)
	{
		printf("can't listen on %s\r\n", addr);
		return 1;
	}

	printf("listen on: %s ok\r\n", addr);

	while(true)
	{
		acl::socket_stream* conn = server.accept();
		if (conn == NULL)
		{
			printf("accept error\r\n");
			break;
		}
		handle_client(conn);
	}
	return 0;
}
