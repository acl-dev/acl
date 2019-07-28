// json.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <list>
#include <vector>
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/json.hpp"

using namespace std;

#if 1
static const char* default_data = "{\"DataValue\":{\"RemoteLoginRemind\":[{\"value\":\"New\",\"onclick\":\"CreateNewDoc()\"},{\"value\":\"Open\",\"onclick\":\"OpenDoc()\"},{\"value\":\"Close\",\"onclick\":\"CloseDoc()\"},[{\"value\":\"Save\",\"onclick\":\"SaveDoc()\"}]]}}";
#else
static const char* default_data = "{\"DataValue\": {\"RemoteLoginRemind\": \"true1\", \"ModifyPasswdRemind\": \"true2\", \"MailForwardRemind\": \"true3\", \"SecureLoginVerification\": \"remote\"}}";
#endif

static void test(bool once)
{
	acl::json json;

	if (once)
		json.update(default_data);
	else
	{
		const char* s = default_data;
		char  buf[2];

		while (*s)
		{
			buf[0] = *s;
			buf[1] = 0;
			json.update(buf);
			s++;
		}
	}

	printf("-------------------------------------------------\r\n");
	printf(">>>%s(%d)->source data: %s\r\n",
		__FUNCTION__, __LINE__, default_data);
	printf("-------------------------------------------------\r\n");
	printf(">>>%s(%d)->build  json: %s\r\n",
		__FUNCTION__, __LINE__, json.to_string().c_str());
	printf("-------------------------------------------------\r\n");
	if (json.to_string() == default_data)
		printf("%s(%d): build json OK!\r\n", __FUNCTION__, __LINE__);
	else
	{
		printf(">>>src:\r\n%s\r\n>>>dst:\r\n%s\r\n",
			default_data, json.to_string().c_str());
		printf("%s(%d): build json ERROR!\r\n", __FUNCTION__, __LINE__);
		exit (1);
	}
	printf("-------------------------------------------------\r\n");

	const char* tags;
	tags = "DataValue/RemoteLoginRemind";
	//tags = "DataValue";
	printf(">>>%s(%d)->tags: %s\r\n", __FUNCTION__, __LINE__, tags);

	const std::vector<acl::json_node*>& nodes = json.getElementsByTags(tags);
	if (nodes.empty())
	{
		printf(">>>%s(%d)->NOT FOUND(tags: %s)\r\n",
			__FUNCTION__, __LINE__, tags);
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
	printf(">>>%s(%d)->convert first json node to json:\r\n%s\r\n",
		__FUNCTION__, __LINE__, json1.to_string().c_str());

	printf("-------------------------------------------------\r\n");

	ACL_VSTRING *bf = acl_vstring_alloc(1);
	ACL_JSON_NODE* node = first->get_json_node()->tag_node;
	if (node)
	{
		acl_json_node_build(node, bf);
		printf(">>>%s(%d)->tag: %s, tag_node's string:\r\n%s\r\n",
			__FUNCTION__, __LINE__, first->tag_name(), acl_vstring_str(bf));
		printf("-------------------------------------------------\r\n");

		if (node->parent == first->get_json_node())
			printf(">>>%s(%d)->OK parent ok\r\n",
				__FUNCTION__, __LINE__);
		else
		{
			printf(">>>%s(%d)->ERROR not parent\r\n", __FUNCTION__, __LINE__);
			exit (1);
		}

		printf("-------------------------------------------------\r\n");
		acl::json_node* obj = first->get_obj();
		if (obj)
			printf(">>>%s(%d)->tag: %s, obj: %s\r\n",
				__FUNCTION__, __LINE__, first->tag_name(),
				obj->to_string().c_str());
		else
			printf(">>>%s(%d)->tag: %s, obj null\r\n",
				__FUNCTION__, __LINE__, first->tag_name());
	}
	else
		printf(">>>%s(%d)->tag_node(%s) for tags(%s), tag: %s, txt: %s\r\n",
			__FUNCTION__, __LINE__,
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

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		"-o [if parse the whole json once, default: false]\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	int   ch;
	bool  once = false;

	while ((ch = getopt(argc, argv, "ho")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 'o':
			once = true;
			break;
		default:
			break;
		}
	}

	test(once);

	printf("enter any key to exit\r\n");
	getchar();
	return 0;
}

