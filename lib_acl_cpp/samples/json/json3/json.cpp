#include "stdafx.h"
/**
 * 测试json解析器对于制表符解析问题，\t \n等
 */

int main()
{
	const char* type = "set";
	const char* tablename = "\txmailuser";
	const char* name = "	chenzhen";
	
	printf(">>>>{%s}\r\n", tablename);
	acl::json json;
	//printf(">>>root: %s\r\n", json.to_string().c_str());
	acl::json_node& first = json.get_root().add_child(false, true);
	first.add_child("type", type);
	first.add_child("tablename", tablename);
	first.add_child("name", name);

	// 生成json字符串
	printf("json to string:%s\r\n", json.to_string().c_str());
	printf("first to string: %s\r\n", first.to_string().c_str());

	acl::string buf;
	json.build_json(buf);
	// 根据生成的字符串获取键值
	acl::json json2(first);
	acl::json_node* root2 = &json2.get_root();
	acl::json_node* child = root2->first_child();

	printf(">>>json2: %s|%s\r\n", json2.to_string().c_str(), buf.c_str());

	const char* tag, *txt;
	while (child != NULL)
	{
		if ((tag = child->tag_name()) != NULL && *tag != 0
			&& (txt = child->get_text()) != NULL && *txt != 0)
		{
			printf("tag: %s, txt: %s\n", tag, txt);
		}
		else
			printf("no tag name or no txt in json node");

		child = root2->next_child();
	}
	printf("\r\n");
	return 0;
}
