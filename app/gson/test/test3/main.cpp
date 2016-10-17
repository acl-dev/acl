#include "stdafx.h"
#include <list>
#include <vector>
#include <map>
#include <stdio.h>
#include <iostream>
#include <time.h>
#include "struct.h"  // 由 gson 工具根据 struct.stub 转换而成
#include "gson.h"    // 由 gson 工具根据 struct.stub 生成

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
	const char *s = "{\"shcool\": \"山东工业大学\", \"class_name\": \"热处理专业\", \"province_name\": \"山东省\", \"position\": \"山东省\", \"name\": \"zsxxsz\", \"age\": 11, \"male\": true}";
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
	{
		printf("name: %s, age: %d, male: %s\r\n",
			u.name.c_str(), u.age, u.male ? "yes" : "no");
		printf("province_name: %s, position: %s\r\n",
			u.province_name.c_str(), u.position.c_str());
		printf("shcool: %s, class_name: %s\r\n",
			u.shcool.c_str(), u.class_name.c_str());
	}
}

int main(void)
{
	serialize();
	deserialize();
	return 0;
}
