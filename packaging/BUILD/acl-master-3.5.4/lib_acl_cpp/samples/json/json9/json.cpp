#include "stdafx.h"

static void test_type(acl::json& json, const char* name)
{
	const std::vector<acl::json_node*>& nodes = json.getElementsByTagName(name);
	if (!nodes.empty())
	{
		printf("%s: type is %s, value is %s\r\n", name,
			nodes[0]->get_type(),
			nodes[0]->get_text() ? nodes[0]->get_text() : "NULL");
	}
	else
		printf("%s not found\r\n", name);
}

static void test_int64(acl::json& json, const char* name)
{
	const std::vector<acl::json_node*>& nodes = json.getElementsByTagName(name);
	if (nodes.empty())
		printf("%s not found\r\n", name);
	else
	{
		const long long int* n = nodes[0]->get_int64();
		if (n == NULL)
		{
			printf("%s: NULL\r\n", name);
			return;
		}
#if defined(_WIN32) || defined(_WIN64)
		printf("%s: %I64u, %I64d\r\n", name, *n, *n);
#else
		printf("%s: %llu, %lld\r\n", name, *n, *n);
#endif
	}
}
#include <iostream>
int main()
{
#if 1
	const char* sss =
		"[{\"DataKey1\": \"BindRule\", \"DataValue\": {\"waittime\": \"7\"}, \"null_key\": \"null\"},\r\n"
		"{\"DataKey2\": \"BindRule\", \"DataValue\": {\"waittime\": \"7\"}, \"null_key\": \"null\"},\r\n"
		"{\"member\": [25, 26, 27, 28, 29, true, false]},\r\n"
		"{\"max_uint64\": 18446744073709551615},\r\n"
		"[\"string\", true, false, 100, 200, 300, null, null],\r\n"
		"{\"hello world\": true, \"name\": null, \"age\": 25}]\r\n"
		"{\"hello\" : \"world\"} \r\n";
#else
	const char* sss = "{\"name\": \"100\"}";
#endif

	acl::json json;
	const char* ptr = json.update(sss);

	printf("-------------------------------------------------------\r\n");

	printf("%s\r\n", sss);

	printf("-------------------------------------------------------\r\n");

	printf("json finish: %s, left char: %s\r\n",
		json.finish() ? "yes" : "no", ptr);

	printf(">>>to string: %s\r\n", json.to_string().c_str());

	const char* ss =
		"[{\"DataKey1\": \"BindRule\", \"DataValue\": {\"waittime\": \"7\"}, \"null_key\": \"null\"}, "
		"{\"DataKey2\": \"BindRule\", \"DataValue\": {\"waittime\": \"7\"}, \"null_key\": \"null\"}, "
		"{\"member\": [25, 26, 27, 28, 29, true, false]}, "
		"{\"max_uint64\": 18446744073709551615 }, "
		"[\"string\", true, false, 100, 200, 300, null, null], "
		"{\"hello world\": true, \"name\": null, \"age\": 25}]";

	printf("-------------------------------------------------------\r\n");

	acl::string buf1(json.to_string()), buf2(ss);

	buf1.trim_space().trim_line();
	buf2.trim_space().trim_line();

	if (buf1 == buf2)
		printf("All OK\r\n\r\n");
	else
	{
		printf("Error\r\n");
		printf("-------------------------------------------------------\r\n");
		printf("%s\r\n", ss);
		printf("-------------------------------------------------------\r\n");
		printf("%s\r\n", json.to_string().c_str());
		printf("\r\n");
		exit (1);
	}
	

	test_type(json, "hello world");
	test_type(json, "name");
	test_type(json, "age");
	test_type(json, "member");
	test_type(json, "DataKey1");
	test_type(json, "DataValue");
	test_type(json, "null_key");
	test_type(json, "string");
	test_type(json, "waittime");
	test_type(json, "max_uint64");

	test_int64(json, "age");
	test_int64(json, "max_uint64");

#if defined(_WIN32) || defined(_WIN64)
	printf("Enter any key to exit ...");
	fflush(stdout);
	getchar();
#endif
	return 0;
}
