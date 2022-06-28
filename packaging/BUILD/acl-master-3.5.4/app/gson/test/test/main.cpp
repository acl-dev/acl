#include "stdafx.h"
#include <list>
#include <vector>
#include <map>
#include <stdio.h>
#include <iostream>
#include <time.h>
#include "struct.h"  // 由 gson 工具根据 struct.stub 转换而成
#include "struct.gson.h"    // 由 gson 工具根据 struct.stub 生成

// 序列化过程
static void serialize(void)
{
	user u;

	u.name = "zsxxsz";
	u.domain = "263.net";
	u.age = 11;
	u.male = true;

	acl::json json;

	// 将 user 对象转换为 json 对象
	acl::json_node& node = acl::gson(json, u);

	printf("serialize:\r\n");
	printf("json: %s\r\n", node.to_string().c_str());
	printf("\r\n");
}

// 反序列化过程
static void deserialize(void)
{
	const char *s = "{\"name\": \"zsxxsz\", \"domain\": \"263.net\", \"age\": 11, \"male\": true, \"n0\": 10, \"n1\": 0, \"n2\": 1, \"n3\": 1.1, \"n4\": 0.0 }";
	printf("deserialize:\r\n");

	acl::json json;
	json.update(s);
	user u;

	// 将 json 对象转换为 user 对象
	std::pair<bool, std::string> ret = acl::gson(json.get_root(), u);

	// 如果转换失败，则打印转换失败原因
	if (ret.first == false)
		printf("error: %s\r\n", ret.second.c_str());
	else
		printf("ok, name: %s, domain: %s, age: %d, male: %s, n0=%d, n1=%.2f, n2=%.2f, n3=%.2f, n4=%.2f\r\n",
			u.name.c_str(), u.domain.c_str(), u.age,
			u.male ? "yes" : "no", u.n0, u.n1, u.n2, u.n3, u.n4);
}

int main(void)
{
	acl::string buf;
	std::string sbuf("hello world");
	buf << sbuf << "\r\n" << &sbuf << "\r\n";
	buf += sbuf;
	buf += "\r\n";
	buf += &sbuf;
	printf("buf=[%s]\r\n", buf.c_str());

	sbuf += "\r\n";
	sbuf += buf;
	printf("sbuf=[%s]\r\n", sbuf.c_str());

	buf = sbuf;
	printf("buf=[%s]\r\n", buf.c_str());

	sbuf.clear();
	buf >> sbuf;
	printf("sbuf=[%s]\r\n", sbuf.c_str());

	buf = sbuf;
	sbuf.clear();
	buf >> &sbuf;
	printf("sbuf=[%s]\r\n", sbuf.c_str());

	serialize();
	deserialize();
	printf("Enter any key to continue ..."); fflush(stdout); getchar();
	return 0;
}
