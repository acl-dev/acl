// json.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <list>
#include <vector>
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/json.hpp"

using namespace std;

static void test(void)
{
	acl::json json;

	//////////////////////////////////////////////////////////////////////////

	acl::json_node& root = json.get_root();
	acl::json_node *node0, *node1, *node2, *node3;

	//node0 = &json.create_node();
	//root.add_child(node0);
	node0 = &root;

	node1 = &json.create_node("name1", "value1");
	node0->add_child(node1);

	node1 = &json.create_node("name2", "value2");
	node0->add_child(node1);

	node1 = &json.create_node();
	node2 = &json.create_node("name3", "value3");
	node1->add_child(node2);
	node2 = &json.create_node("name4", node1);
	node0->add_child(node2);

	////////////////////////////////////////////////////////////////////////////

	node1 = &json.create_array();
	node2 = &json.create_node("name5", node1);
	node0->add_child(node2);

	node3 = &json.create_node().add_child(json.create_node("name6", "value6"));
	node1->add_child(node3);

	node3 = &json.create_node("name7", "value7");
	node1->add_child(node3);

	node1->add_child(json.create_array_text("name7_string1"))
		.add_child(json.create_array_text("name7_string2"));

	node3 = &json.create_node();
	node1->add_child(node3);

	node1 = &json.create_node("name8", "value8");
	node2 = &json.create_node("name9", "value9");
	(*node3).add_child(node1).add_child(node2);

	//////////////////////////////////////////////////////////////////////////

	acl::json_node& node_a =
		json.create_node("name12",
				json.create_node()
				.add_child(json.create_node("name12_1_1", "value12_1_1"))
				.add_child(json.create_node("name12_1_2", "value12_1_2"))
				.add_child(json.create_node("name12_1_3", "value12_1_3"))
				.add_child(json.create_node("name12_1_4", "value12_1_4")));
	acl::json_node& node_b =
			json.create_node()
			.add_child(json.create_node("name13_1", "value13_1"))
			.add_child(json.create_node("name13_2", "value13_2"))
			.add_child(json.create_node("name13_3", "value13_3"));
	acl::json_node& node_c =
			json.create_node()
			.add_child(json.create_node("name14_1", "value14_1"));
	acl::json_node& node_d =
			json.create_node()
			.add_child(json.create_node("name15_1", "value15_1"));
	acl::json_node& node_e =
		json.create_node()
		.add_child(json.create_node("name15_2", "value15_2"));

	node0->add_child(node_a)
		.add_child(node_b)
		.add_child(node_c)
		.add_child(node_d)
		.add_child(node_e)
		.add_child(json.create_node("name16", "value16"))
		.add_child(json.create_node("name17", "value17"))
		.add_child(json.create_node("name18", "value18"));

	//////////////////////////////////////////////////////////////////////////
	node0->add_text("name19", "value19")
		.add_text("name20", "value20")
		.add_text("name21", "value21");
	(*node0).add_child("name23",
			json.create_node()
			.add_text("name24", "value24")
			.add_text("name24_1", "value24_1"))
		.add_child(true, true).add_text("name25", "value25")
			.add_text("name26", "value26")
			.add_text("name27", "value27")
			.add_bool("name_bool", true)
			.add_null("name_null")
			.add_array_bool("array_bool", false)
			.add_array_null()
			.get_parent()
		.add_text("name28", "value28")
		.add_text("name29", "value29")
		.add_text("name30", "value30");
	//////////////////////////////////////////////////////////////////////////

	// 遍历所有有标签名的结点

	printf("----------------------------------------------------------\r\n");
	acl::json_node* iter = json.first_node();
	while (iter)
	{
		if (iter->tag_name())
		{
			printf("tag: %s", iter->tag_name());
			if (iter->get_text())
				printf(", value: %s\r\n", iter->get_text());
			else
				printf("\r\n");
		}
		iter = json.next_node();
	}

	printf("------------------root first level child---------\r\n");
	iter = node0->first_child();
	while (iter)
	{
		if (iter->tag_name())
			printf("tag: %s", iter->tag_name());
		if (iter->get_text())
			printf(", text: %s\r\n", iter->get_text());
		else
			printf("\r\n");
		iter = node0->next_child();
	}

	//////////////////////////////////////////////////////////////////////////

	printf("-------------------------------------------------\r\n");

	acl::string buf;
	json.build_json(buf);

	printf("-----------------json------------------------\r\n");
	printf("%s\r\n", buf.c_str());
}

int main(void)
{
	test();
	printf("enter any key to exit\r\n");
	getchar();
	return 0;
}

