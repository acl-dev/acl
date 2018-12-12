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

	std::vector<std::string>  names;
	names.push_back("zsx11");
	names.push_back("zsx12");
	names.push_back("zsx13");
	names.push_back("zsx14");

	u.names["zsx1"] = names;

	names.clear();

	names.push_back("zsx21");
	names.push_back("zsx22");
	names.push_back("zsx23");
	names.push_back("zsx24");

	u.names["zsx2"] = names;

	acl::json json;

	// 将 user 对象转换为 json 对象
	acl::json_node& node = acl::gson(json, u);

	printf(">> serialize:\r\n");
	printf("json: %s\r\n", node.to_string().c_str());
	printf("\r\n");
}

// 反序列化过程
static void deserialize(void)
{
	const char *s = "{\"name\": \"zsxxsz\", \"domain\": \"263.net\", \"age\": 11, \"male\": true, \"names\": [{\"zsx1\": [\"zsx11\", \"zsx12\", \"zsx13\", \"zsx14\"]}, {\"zsx2\": [\"zsx21\", \"zsx22\", \"zsx23\", \"zsx24\"]}], \"values5\": [100, 1000, 10000]}";

	acl::json json;
	json.update(s);
	user u;

	printf(">> deserialize:\r\n");

	// 将 json 对象转换为 user 对象
	std::pair<bool, std::string> ret = acl::gson(json.get_root(), u);

	// 如果转换失败，则打印转换失败原因
	if (ret.first == false)
	{
		printf("error: %s\r\n", ret.second.c_str());
		return;
	}

	printf("name: %s, domain: %s, age: %d, male: %s\r\n", u.name.c_str(),
		u.domain.c_str(), u.age, u.male ? "yes" : "no");

	std::map<std::string, std::vector<std::string> >::const_iterator
		cit = u.names.find("zsx2");
	if (cit != u.names.end())
	{
		printf("zsx2:");
		for (std::vector<std::string>::const_iterator cit2
			= cit->second.begin();
			cit2 != cit->second.end(); ++cit2)
		{
			printf(" %s", (*cit2).c_str());
		}

		printf("\r\n");
	}

	printf("pids: ");
	for (std::set<int>::const_iterator cit2 = u.values5.begin();
		cit2 != u.values5.end(); ++cit2)
	{
		if (cit2 != u.values5.begin())
			printf(", ");
		printf("%d", *cit2);
	}
	printf("\r\n");
}

int main(void)
{
	serialize();
	deserialize();
	printf("Enter any key to continue ..."); fflush(stdout); getchar();
	return 0;
}
