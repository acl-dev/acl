// url_coder.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/url_coder.hpp"
#include "acl_cpp/http/http_utils.hpp"

using namespace acl;

int main(void)
{
	url_coder coder1;

	coder1.set("name1", "value1");
	coder1.set("name2", 2);
	coder1.set("name3", true, "value%d", 3);
	coder1.set("name4", "中国人");
	printf("coder1 >> %s, name1: %s, name2: %s, name3: %s, name4: %s\r\n",
		coder1.to_string().c_str(),
		coder1["name1"], coder1["name2"], coder1["name3"], coder1["name4"]);
	coder1.del("name1");
	const char* ptr = coder1["name1"];
	printf("coder1 >> %s, name1: %s, name2: %s, name3: %s, name4: %s\r\n",
		coder1.to_string().c_str(), ptr ? ptr : "null",
		coder1["name2"], coder1["name3"], coder1["name4"]);

	//////////////////////////////////////////////////////////////////////////

	url_coder coder2;

	coder2 = coder1;
	coder2.set("name5", "&=value5=&");
	ptr = coder2["name1"];
	printf("--------------------------------------------------------\r\n");
	printf("coder2 >> %s, name1: %s, name2: %s, name3: %s, name4: %s, name5: %s\r\n",
		coder2.to_string().c_str(), ptr ? ptr : "null",
		coder2["name2"], coder2["name3"], coder2["name4"], coder2["name5"]);

	//////////////////////////////////////////////////////////////////////////

	url_coder coder3(coder2);

	coder3.set("name5", 5);
	coder3.set("name6", "=&外国人&=");
	ptr = coder3["name1"];
	printf("--------------------------------------------------------\r\n");
	printf("coder3 >> %s, name1: %s, name2: %s, name3: %s, name4: %s, name5: %s, name6: %s\r\n",
		coder3.to_string().c_str(), ptr ? ptr : "null",
		coder3["name2"], coder3["name3"], coder3["name4"],
		coder3["name5"], coder3["name6"]);

	//////////////////////////////////////////////////////////////////////////

	url_coder coder4;
	const char* s = "name1=value1&name2=2&name3=value3&name4=%D6%D0%B9%FA%C8%CB";
	coder4.decode(s);
	printf("--------------------------------------------------------\r\n");
	printf("coder4 >> %s, name1: %s, name2: %s, name3: %s, name4: %s\r\n",
		coder4.to_string().c_str(),
		coder4["name1"], coder4["name2"], coder4["name3"], coder4["name4"]);

	//////////////////////////////////////////////////////////////////////////

	printf("--------------------------------------------------------\r\n");
	acl::url_coder coder5;
	coder5.set("n0", "").set("n1", "v1").set("n2", "");
	acl::string s5 = coder5.to_string();
	coder5.reset();

	coder5.decode(s5);
	bool found0, found3;
	const char* v0 = coder5.get("n0", &found0);
	const char* v3 = coder5.get("n3", &found3);
	printf(">>>url=%s, n0=%s, %s, n1=%s, n3=%s, %s\r\n", s5.c_str(),
		v0, found0 ? "found it" : "not found", coder5.get("n1"),
		v3, found3 ? "found it" : "not found");

	printf("enter any key to continue ...\r\n");
	getchar();

	printf("\r\n");

	acl::http_url hu;
	const char* urls[] = {
		"http://www.google.com/",
		"https://www.google.com/",
		"https://www.google.com/test",
		"http://www.google.com/test?name=value&name2=value2",
		"/test",
		"/",
		"/test?",
		"/test?name1=value1&name2=value2",
		"/path/test",
		"/path/test?name=value",
		NULL,
	};

	for (size_t i = 0; urls[i] != NULL; i++) {
		if (!hu.parse(urls[i])) {
			printf("parse url=%s failed\r\n", urls[i]);
			break;
		}

		printf("url:%s\r\n", urls[i]);
		printf("proto=%s, port=%d, domain=%s, path=%s, params=%s\r\n",
			hu.get_proto(), hu.get_port(), hu.get_domain(),
			hu.get_url_path(), hu.get_url_params());
		printf("\r\n");

		hu.reset();
	}

#if defined(_WIN32) || defined(_WIN64)
	printf("Enter any key to exit ...\r\n");
	getchar();
#endif
	return 0;
}

