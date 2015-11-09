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
	acl::json_node& first = json.get_root().add_child(false, true);
	first.add_text("type", type);
	first.add_text("tablename", tablename);
	first.add_text("name", name);

	// 生成json字符串
	printf("json to string:%s\r\n", json.to_string().c_str());
	printf("first json node to string: %s\r\n", first.to_string().c_str());

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

	/////////////////////////////////////

	const char* sss = "{\"DataKey\": \"BindRule\", \"DataValue\": {\"waittime\": \"7\"}, \"null_key\": null}";

	acl::json json3(sss);
	const char* tags = "DataValue";

	printf("----------------------------------------------------\r\n");
	printf(">>%s\r\n", sss);
	acl::json_node* iter = json3.first_node();
	while (iter)
	{
		tag = iter->tag_name();
		txt = iter->get_text();
		printf("tag: %s, txt: %s\r\n", tag ? tag : "null", txt ? txt : "null");
		if (txt)
			iter->set_text("hello");
		printf("tag: %s, txt: %s\r\n", tag ? tag : "null", txt ? txt : "null");
		iter = json3.next_node();
	}
	printf("----------------------------------------------------\r\n");

	const std::vector<acl::json_node*>& nodes = json3.getElementsByTags(tags);
	if (nodes.empty() == false)
	{
		printf(">>>%s\r\n", nodes[0]->to_string().c_str());
	}
	return 0;
}
