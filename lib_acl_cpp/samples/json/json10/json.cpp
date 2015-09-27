#include "stdafx.h"

int main()
{
	acl::json json;
	acl::json_node& root = json.get_root();

	//////////////////////////////////////////////////////////////////////
	// 方法一：

	root.add_child("Para", json.create_node()
				.add_child("xxx", "111")
				.add_child("yyy", "222")
				.add_bool("zzz", true)
				.add_number("eee", 100));

	printf("%s\r\n", json.to_string().c_str());
	acl::string buf1;
	json.build_json(buf1);

	//////////////////////////////////////////////////////////////////////

	// 在重新使用前需要重置 json 生成器状态
	json.reset();

	//////////////////////////////////////////////////////////////////////
	// 方法二：

	acl::json_node& node1 = json.create_node();
	root.add_child("Para", node1);

	acl::json_node& node11 = json.create_node("xxx", "111");
	node1.add_child(node11);

	acl::json_node& node12 = json.create_node("yyy", "222");
	node1.add_child(node12);

	acl::json_node& node13 = json.create_node("zzz", true);
	node1.add_child(node13);

	acl::json_node& node14 = json.create_node("eee", (long long int) 100);
	node1.add_child(node14);

	printf("%s\r\n", json.to_string().c_str());
	acl::string buf2;
	json.build_json(buf2);

	if (buf2 == buf1)
		printf("OK\r\n");
	else
		printf("ERROR\r\n");
	return 0;
}
