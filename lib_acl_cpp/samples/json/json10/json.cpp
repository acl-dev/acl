#include "stdafx.h"

int main()
{
	acl::json json;
	acl::json_node& root = json.get_root();


	// 生成 json 串：{"1": {"1": {"1": "aa"}}}
	root.add_child("1", true).add_child("1", true).add_text("1", "aa");
	printf("%s\r\n", json.to_string().c_str());

	json.reset();

	//////////////////////////////////////////////////////////////////////
	// 以下三种方法可以生成相同的 json 串如下：
	// {"cmd": "add", "Para": {"xxx": "111", "yyy": "222", "zzz": true, "eee": 100}, "status": true, "length": 100}
	
	//////////////////////////////////////////////////////////////////////
	// 方法一：

	root.add_text("cmd", "add")	// 添加 root 节点的子节点并返回 root
	  .add_child("Para", true)	// 添加 root 的子节点(Para)并返回 Para
	    .add_text("xxx", "111")	// 添加 Para 的子节点并返回 Para
	    .add_text("yyy", "222")	// 添加 Para 的子节点并返回 Para
	    .add_bool("zzz", true)	// 添加 Para 的子节点并返回 Para
	    .add_number("eee", 100)	// 添加 Para 的子节点并返回 Para
	  .get_parent()			// 返回 Para 的父节点(root节点)
	  .add_bool("status", true)	// 添加 root 节点的子节点 status
	  .add_number("length", 100);	// 添加 root 节点的子节点 length

	printf("%s\r\n", json.to_string().c_str());
	acl::string buf1;
	json.build_json(buf1);

	//////////////////////////////////////////////////////////////////////
	// 方法二：

	// 在重新使用前需要重置 json 生成器状态
	json.reset();

	root.add_text("cmd", "add")
		.add_child("Para",
			json.create_node()
				.add_text("xxx", "111")
				.add_text("yyy", "222")
				.add_bool("zzz", true)
				.add_number("eee", 100))
		.add_bool("status", true)
		.add_number("length", 100);

	printf("%s\r\n", json.to_string().c_str());
	acl::string buf2;
	json.build_json(buf2);

	//////////////////////////////////////////////////////////////////////
	// 方法三：

	// 在重新使用前需要重置 json 生成器状态
	json.reset();

	acl::json_node& cmd = json.create_node("cmd", "add");
	root.add_child(cmd);

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

	acl::json_node& status = json.create_node("status", true);
	root.add_child(status);

	acl::json_node& length = json.create_node("length", (long long int) 100);
	root.add_child(length);

	printf("%s\r\n", json.to_string().c_str());
	acl::string buf3;
	json.build_json(buf3);

	//////////////////////////////////////////////////////////////////////
	// 比较三种方法的结果是否相等

	if (buf2 == buf1 && buf3 == buf2)
		printf("OK\r\n");
	else
		printf("ERROR\r\n");
	return 0;
}
