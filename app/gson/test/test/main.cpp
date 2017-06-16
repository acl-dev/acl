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
	const char *s = "{\"name\": \"zsxxsz\", \"domain\": \"263.net\", \"age\": 11, \"male\": true}";
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
		printf("name: %s, domain: %s, age: %d, male: %s\r\n",
			u.name.c_str(), u.domain.c_str(), u.age,
			u.male ? "yes" : "no");
}

int main(void)
{
	serialize();
	deserialize();
	printf("Enter any key to continue ..."); fflush(stdout); getchar();
	return 0;
}
