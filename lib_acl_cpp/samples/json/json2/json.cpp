// json.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <list>
#include <vector>
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/json.hpp"

using namespace std;

static void list_print(const vector<acl::json_node*>& nodes)
{
	vector<acl::json_node*>::const_iterator cit = nodes.begin();
	for (; cit != nodes.end(); ++cit)
	{
		printf(">>>json node to string: %s\r\n", (*cit)->to_string().c_str());
		const char* name = (*cit)->tag_name();
		const char* text = (*cit)->get_text();
		acl::json_node* node1 = (*cit)->first_child();
		printf("tag: %s, value: %s\r\n", name ? name : "null",
			node1 ? "obj" : (text ? text : "null"));

		if (node1 == NULL)
			continue;

		// 第一层子结点
		while (node1)
		{
			name = node1->tag_name();
			text = node1->get_text();
			acl::json_node* node2 = node1->first_child();
			printf("\ttag: %s, value: %s\r\n",
				name ? name : "null",
				node2 ? "obj" : (text ? text : "null"));

			if (node2 == NULL)
			{
				node1 = (*cit)->next_child();
				continue;
			}

			// 第二层子结点
			while (node2)
			{
				name = node2->tag_name();
				text = node2->get_text();
				acl::json_node* node3 = node2->first_child();

				printf("\t\ttag: %s, value: %s\r\n",
					name ? name : "null",
					node3 ? "obj" : (text ? text : "null"));

				if (node3 == NULL)
				{
					node2 = node1->next_child();
					continue;
				}

				// 第三层子结点
				while (node3)
				{
					name = node3->tag_name();
					text = node3->get_text();
					acl::json_node* node4 = node3->first_child();

					printf("\t\t\ttag: %s, value: %s\r\n",
						name ? name : "null",
						node4 ? "obj" : (text ? text : "null"));

					node3 = node2->next_child();
				}

				node2 = node1->next_child();
			}
			node1 = (*cit)->next_child();
		}
	}
}

static void test(void)
{
	static const char* data = \
"{'Action' : 'set' , 'Object' : 'user' , 'UpdateTime' : '<updatetime_value>' , 'Key' : '<xmuserid>',\r\n"
"  'DataList' : [\r\n"
"    {'DataKey' : 'BindInfo' , 'DataValue' : {'BindList' : [{'BindName' : 'BindText'}, {'BindType' : '<bindtype_value>' , 'BindId' : '<bindid_value>'}]}},\r\n"
"    {'DataKey' : 'BindRule' , 'DataValue' : [{'name1':'value1'}, {'name2': 'value2'}]},\r\n"
"    {'DataKey' : 'UserTap' ,  'DataValue' : {'RemoteLogin' : 'true' , 'ModifyPass' : 'true' , 'MailForward' : 'true' , 'Secure' : 'remote'}} \r\n"
"]}\r\n";
	acl::json json(data);

	printf("-------------------------------------------------------\r\n");

	printf("%s", data);

	printf("-------------------------------------------------------\r\n");

	const char* tags_list[] = {
		"DataList",
		"DataList/DataKey",
		"DataList/DataValue",
		"DataList/DataValue/BindList",
		"DataValue/BindList/BindName",
		"DataList/DataValue/name1",
		"DataList/*/name1",
		"DataList/DataValue/MailForward",
		"DataList/*/MailForward",
		"DataList/MailForward",
		NULL
	};

	for (int i = 0; tags_list[i] != NULL; i++)
	{
		const vector<acl::json_node*>& dlist = json.getElementsByTags(tags_list[i]);
		if (dlist.empty())
			printf(">>> empty tags: %s\r\n", tags_list[i]);
		else
		{
			printf(">>> find %s:\r\n", tags_list[i]);
			list_print(dlist);
		}
		printf("-------------------------------------------------------\r\n");
	}
}

int main(void)
{
	test();
	printf("enter any key to exit\r\n");
	getchar();
	return 0;
}

