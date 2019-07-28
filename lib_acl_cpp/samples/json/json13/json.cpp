#include "stdafx.h"
/**
 * 测试json解析器对于制表符解析问题，\t \n等
 */

int main(void)
{
	const char* s = "{ 'cmd': 'GET',\r\n"
		"'data': { 'count': 2, iptables: [\r\n"
		"	{'test1': '192.168.1.1'},\r\n"
		"	{'test2': '192.168.1.2'},\r\n"
		"	{'test3': '192.168.1.3'},\r\n"
		"	{'test4': '192.168.1.4'},\r\n"
		"	{'test5': '192.168.1.5'},\r\n"
		"	{'test6': '192.168.1.6'},\r\n"
		"	{'test7': 192 },\r\n"
		"	{'test8': true}\r\n"
		"]}}";


	acl::json json(s);
	const char* tags = "data/iptables";

	acl::json_node* node = json.getFirstElementByTags(tags);
	if (node == NULL)
	{
		printf("no tags: %s\r\n", tags);
		return 0;
	}

	printf("tag: %s, type: %s\r\n", node->tag_name(), node->get_type());

	acl::json_node* array = node->get_obj();
	if (array == NULL)
	{
		printf("get_obj null\r\n");
		return 0;
	}
	if (array->is_array() == false)
	{
		printf("not array: %s\r\n", array->to_string().c_str());
		return 0;
	}
	else
		printf("Array: %s\r\n", array->to_string().c_str());

	printf("-------------------------------------------------------\r\n");

	acl::json_node* child = array->first_child();
	while (child)
	{
		printf("type: %s->%s\r\n", child->get_type(),
			child->to_string().c_str());

		const char* ptr = (*child)["test1"];
		if (ptr)
			printf(">>> found, test1: %s\r\n", ptr);

		acl::json_node* iter = child->first_child();
		while (iter)
		{
			printf("type: %s, string: %s, tag: %s, txt: %s\r\n",
				iter->get_type(), iter->to_string().c_str(),
				iter->tag_name(), iter->get_text());

			iter = child->next_child();
		}

		child = array->next_child();
	}

	return 0;
}
