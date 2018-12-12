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
	u.age = 11;
	u.male = true;

	u.province_name = "山东省";
	u.position = "山东省";

	u.shcool = "山东工业大学";
	u.class_name = "热处理专业";

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
	const char *s = "{\"shcool\": \"山东工业大学\", \"class_name\": \"热处理专业\", \"province_name\": \"山东省\", \"position\": \"山东省\", \"name\": \"zsxxsz\", \"nicks\": [\"\", \"大仙\"], \"age\": 11, \"male\": true, \"ages\": [1, 2, 3, 4, 5] }";
	printf("deserialize:\r\n");

	acl::json json;
	json.update(s);
	user u;

	// 将 json 对象转换为 user 对象
	std::pair<bool, std::string> ret = acl::gson(json.get_root(), u);

	// 如果转换失败，则打印转换失败原因
	if (ret.first == false)
	{
		printf("error: %s\r\n", ret.second.c_str());
		exit (1);
	}
	else
	{
		printf(">>> %s:\r\n", __FUNCTION__);
		printf("name: %s, age: %d, male: %s\r\n",
			u.name.c_str(), u.age, u.male ? "yes" : "no");
		printf("province_name: %s, position: %s\r\n",
			u.province_name.c_str(), u.position.c_str());
		printf("shcool: %s, class_name: %s\r\n",
			u.shcool.c_str(), u.class_name.c_str());
		for (std::vector<std::string>::const_iterator cit = u.nicks.begin();
			cit != u.nicks.end(); ++cit)
		{

			printf("nick: %s\r\n", (*cit).c_str());
		}
	}
}

static void test1(void)
{
	union
	{
		struct user_male*   m;
		struct user_female* f;
		struct people*      p;
	} obj;

	struct user_male* m = new struct user_male;
	(*m).favorite = "";
	(*m).height = 170;
	(*m).name = "";
	(*m).nicks.push_back("");
	(*m).nicks.push_back("大仙");
	(*m).age = 11;
	(*m).male = true;

	(*m).province_name = "山东省";
	(*m).position = "山东省";

	(*m).shcool = "山东工业大学";
	(*m).class_name = "热处理专业";

	obj.m = m;
	acl::json json;

	// 将 user 对象转换为 json 对象
	acl::json_node& node = acl::gson(json, obj.m);

	printf("serialize:\r\n");
	printf("json: %s\r\n", node.to_string().c_str());
	printf("\r\n");

	delete obj.p;
}

static void test2(void)
{
	const char *s = "{\"shcool\": \"山东工业大学\", \"class_name\": \"热处理专业\", \"province_name\": \"山东省\", \"position\": \"山东省\", \"name\": \"zsxxsz\", \"age\": 11, \"male\": true, \"favorite\": \"pingpang\", \"height\": 170, \"ages\": [1, 2, 3, 4, 5]}";
	printf("deserialize:\r\n");

	acl::json json;
	json.update(s);
	user_male u;

	// 将 json 对象转换为 user 对象
	std::pair<bool, std::string> ret = acl::gson(json.get_root(), u);

	// 如果转换失败，则打印转换失败原因
	if (ret.first == false)
		printf("error: %s\r\n", ret.second.c_str());
	else
	{
		printf("name: %s, age: %d, male: %s, favorite: %s, height: %d\r\n",
			u.name.c_str(), u.age, u.male ? "yes" : "no",
			u.get_favorite(), u.get_height());
		printf("province_name: %s, position: %s\r\n",
			u.province_name.c_str(), u.position.c_str());
		printf("shcool: %s, class_name: %s\r\n",
			u.shcool.c_str(), u.class_name.c_str());
	}

	printf("ages: ");
	for (std::vector<int>::const_iterator cit = u.ages.begin();
		cit != u.ages.end(); ++cit)
	{
		if (cit != u.ages.begin())
			printf(", ");
		printf("%d", *cit);
	}
	printf("\r\n");
}

static void test3(void)
{
	const char* s = "{\"status\":200,\"msg\":\"ok\",\"disk\":\"/data\",\"files\":[]}";

	acl::json json(s);
	files_outdate files;
	if (acl::deserialize<files_outdate>(json, files)) {
		printf("ok, files size=%lu\r\n", (unsigned long) files.files.size());
	} else {
		printf("parse error\r\n");
	}
}

int main(void)
{
	test1(); exit(0);
	printf("------------------------serialize----------------------\r\n");
	serialize();

	printf("------------------------deserialize--------------------\r\n");
	deserialize();

	printf("------------------------test1--------------------------\r\n");
	test1();

	printf("------------------------test2--------------------------\r\n");
	test2();

	printf("------------------------test3--------------------------\r\n");
	test3();

	printf("Enter any key to continue ..."); fflush(stdout); getchar();
	return 0;
}
