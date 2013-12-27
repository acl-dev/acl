// json.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <list>
#include <vector>
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/json.hpp"

using namespace std;

#if 1
static const char* default_data = "{\"DataValue\": {\"RemoteLoginRemind\": [{\"value\": \"New\", \"onclick\": \"CreateNewDoc()\"}, {\"value\": \"Open\", \"onclick\": \"OpenDoc()\"}, {\"value\": \"Close\", \"onclick\": \"CloseDoc()\"}, [{\"value\": \"Save\", \"onclick\": \"SaveDoc()\"}]]}}";
#else
static const char* default_data = "{\"DataValue\": {\"RemoteLoginRemind\": \"true1\", \"ModifyPasswdRemind\": \"true2\", \"MailForwardRemind\": \"true3\", \"SecureLoginVerification\": \"remote\"}}";
#endif

static void test(void)
{
	acl::json json(default_data);

	printf("-------------------------------------------------\r\n");
	printf(">>>source data: %s\r\n", default_data);
	printf("-------------------------------------------------\r\n");
	printf(">>>build  json: %s\r\n", json.to_string().c_str());
	printf("-------------------------------------------------\r\n");
	if (json.to_string() == default_data)
		printf("OK!\r\n");
	else
		printf("ERROR!\r\n");
	printf("-------------------------------------------------\r\n");

	const char* tags;
	tags = "DataValue/RemoteLoginRemind";
	//tags = "DataValue";
	printf(">>>tags: %s\r\n", tags);

	const std::vector<acl::json_node*>& nodes = json.getElementsByTags(tags);
	if (nodes.empty())
	{
		printf("NOT FOUND\r\n");
		return;
	}

	acl::json_node* first = nodes[0];
	acl_assert(first);

	printf(">>>node(tags: %s, tag: %s, text: %s, left: %d) to string:\r\n%s\r\n",
		tags, first->tag_name() ? first->tag_name() : "null",
		first->get_text() ? first->get_text() : "null",
		first->get_json_node()->left_ch, first->to_string().c_str());
	printf("-------------------------------------------------\r\n");

	acl::json json1(*first);
	printf(">>>convert first json node to json: %s\r\n", json1.to_string().c_str());

	printf("-------------------------------------------------\r\n");

	ACL_VSTRING *bf = acl_vstring_alloc(1);
	ACL_JSON_NODE* node = first->get_json_node()->tag_node;
	if (node)
	{
		acl_json_node_build(node, bf);
		printf(">>>tag: %s, tag_node's string:\r\n%s\r\n",
			first->tag_name(), acl_vstring_str(bf));
		printf("-------------------------------------------------\r\n");

		if (node->parent == first->get_json_node())
			printf(">>>OK parent ok\r\n");
		else
			printf(">>>ERROR not parent\r\n");

		printf("-------------------------------------------------\r\n");
		acl::json_node* obj = first->get_obj();
		if (obj)
			printf(">>>tag: %s, obj: %s\r\n", first->tag_name(),
				obj->to_string().c_str());
		else
			printf(">>>tag: %s, obj null\r\n", first->tag_name());
	}
	else
		printf(">>>tag_node(%s) for tags(%s), tag: %s, txt: %s\r\n",
			first->get_json_node()->tag_node ? "not null" : "null",
			tags, first->tag_name() ? first->tag_name() : "null",
			first->get_text() ? first->get_text() : "null");

	acl_vstring_free(bf);

	/////////////////////////////////////////////////////////////////////

	printf("-------------------------------------------------\r\n");

	acl::json_node* iter = json.first_node();
	while (iter)
	{
		const char* tag = iter->tag_name();
		const char* txt = iter->get_text();

		node = iter->get_json_node();
		if (node->left_ch)
		{
			printf("%c", node->left_ch);
			fflush(stdout);
		}
		if (tag)
			printf("tag: %s, txt: %s\r\n",
				tag, txt ? txt : "null");
		if (node->right_ch)
		{
			printf("%c->", node->right_ch);
			fflush(stdout);
		}
		iter = json.next_node();
	}

	printf("\r\n");
	printf("-------------------------------------------------\r\n");
}

int main(void)
{
	test();
	printf("enter any key to exit\r\n");
	getchar();
	return 0;
}

