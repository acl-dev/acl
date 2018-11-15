// json.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <list>
#include <vector>
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/json.hpp"

using namespace std;

#if 1
static const char* default_data = \
    "{ 'menu name': {\r\n"
    "    'id:file': 'file',\r\n"
    "    'value{': 'File',\r\n"
    "    'popup{}': {\r\n"
    "        'menuitem1}': [\r\n"
    "            {'value': 'New', 'onclick': 'CreateNewDoc()'},\r\n"
    "            {'value': 'Open', 'onclick': 'OpenDoc()'},\r\n"
    "            {'value': 'Close', 'onclick': 'CloseDoc()'}\r\n"
    "        ],\r\n"
    "        'menuname[]': 'hello world',\r\n"
    "        'inner': { 'value' : 'new ', 'value' : 'open' },\r\n"
    "        'menuitem2': [\r\n"
    "            {'value': 'New', 'onclick': 'CreateNewDoc()'},\r\n"
    "            {'value': 'Open', 'onclick': 'OpenDoc()'},\r\n"
    "            {'value': 'Close', 'onclick': 'CloseDoc()'},\r\n"
    "            {{'value': 'Help', 'onclick': 'Help()'}}"
    "        ]\r\n"
    "    }\r\n"
    " },\r\n"
    " 'help': 'hello world!',\r\n"
    " 'menuitem2': [\r\n"
    "   { 'value1': 'Open1',  'onclick': 'Open1()'},\r\n"
    "   { 'value2': 'Open2',  'onclick': 'Open2()'},\r\n"
    "   [{'value3': 'Open3',  'onclick': 'Open3()'}],\r\n"
    "   [{'value4': 'Open4'}, 'onclick', 'Open4()'],\r\n"
    "   [ 'value5', 'Open5',  'onclick', 'Open5()'],\r\n"
    "   { 'value6': 'Open6',  'onclick': 'Open6()'}\r\n"
    " ]\r\n"
    "}\r\n";
static const char* default_result = "{\"menu name\":{\"id:file\":\"file\",\"value{\":\"File\",\"popup{}\":{\"menuitem1}\":[{\"value\":\"New\",\"onclick\":\"CreateNewDoc()\"},{\"value\":\"Open\",\"onclick\":\"OpenDoc()\"},{\"value\":\"Close\",\"onclick\":\"CloseDoc()\"}],\"menuname[]\":\"hello world\",\"inner\":{\"value\":\"new \",\"value\":\"open\"},\"menuitem2\":[{\"value\":\"New\",\"onclick\":\"CreateNewDoc()\"},{\"value\":\"Open\",\"onclick\":\"OpenDoc()\"},{\"value\":\"Close\",\"onclick\":\"CloseDoc()\"},{{\"value\":\"Help\",\"onclick\":\"Help()\"}}]}},\"help\":\"hello world!\",\"menuitem2\":[{\"value1\":\"Open1\",\"onclick\":\"Open1()\"},{\"value2\":\"Open2\",\"onclick\":\"Open2()\"},[{\"value3\":\"Open3\",\"onclick\":\"Open3()\"}],[{\"value4\":\"Open4\"},\"onclick\",\"Open4()\"],[\"value5\",\"Open5\",\"onclick\",\"Open5()\"],{\"value6\":\"Open6\",\"onclick\":\"Open6()\"}]}";

#elif 1
static const char* default_data = "{\"menuitem2\": [{\"value1\": \"Open1\", \"onclick\": \"Open1()\"}, {\"value2\": \"Open2\", \"onclick\": \"Open2()\"}, [{\"value3\": \"Open3\", \"onclick\": \"Open3()\"}], [\"value4\", \"Open4\", \"onclick\", \"Open4()\"], [{\"value5\": \"Open5\"}, \"onclick\", \"Open5()\"], {\"value6\": \"Open6\", \"onclick\": \"Open6()\"}]}";
static const char* default_result = default_data;
#elif 1
static const char* default_data = "{\"item1\": [\"open\", \"onclick\", \"item2\", \"Help\"]}";
static const char* default_result = default_data;
#else
static const char* default_data = "{\"item1\": [{\"open\": \"onclick\"}, {\"item2\": \"Help\"}]}";
static const char* default_result = default_data;
#endif

static void test(void)
{
	acl::json json1(default_data);

	printf("-------------------------------------------------\r\n");
	printf("source:\r\n%s\r\n", default_data);
	printf("-------------------------------------------------\r\n");
	printf(">>build to_string:\r\n%s\r\n", json1.to_string().c_str());
	printf("-------------------------------------------------\r\n");
	if (json1.to_string() != default_result)
	{
		printf("%s(%d): result should:\r\n%s\r\n",
			__FUNCTION__, __LINE__, default_result);
		exit (1);
	}
	else
		printf("%s(%d): Build json OK!\r\n", __FUNCTION__, __LINE__);

	acl::string buf1 = json1.to_string();
	json1.reset();
	json1.update(default_data);
	if (json1.to_string() != buf1)
	{
		printf("-------------------------------------------------\r\n");
		printf("parse error after reset\r\n");
		printf("first parse:\r\n%s\r\n", buf1.c_str());
		printf("second parse:\r\n%s\r\n", json1.to_string().c_str());
		printf("-------------------------------------------------\r\n");
		return;
	}

#if 0
	acl::json_node* iter = json1.first_node();
	while (iter)
	{
		const char* tag = iter->tag_name();
		const char* txt = iter->get_text();

		ACL_JSON_NODE* node = iter->get_json_node();
		for (int i = 0; i < node->depth; i++)
			putchar('\t');
		if (node->left_ch)
		{
			printf("%c", node->left_ch);
			fflush(stdout);
		}
		if (tag)
			printf("tag: %s, txt: %s",
				tag, txt ? txt : "null");
		/*
		if (node->right_ch)
		{
			printf("%c->", node->right_ch);
			fflush(stdout);
		}
		*/
		iter = json1.next_node();
		putchar('\n');
	}

	printf("\r\n");
	printf("-------------------------------------------------\r\n");
#endif

	const char* tags;

	if (0)
		tags = "menu name/popup{}/menuitem1}";
	else if (0)
		tags = "menuitem2/*/*/onclick";
	else if (1)
		tags = "menu name/popup{}";
	else
		tags = "menu name/id:file";

	printf("-------------------------------------------------\r\n");
	printf(">>>%s(%d)->tags: %s\r\n", __FUNCTION__, __LINE__, tags);

	const std::vector<acl::json_node*>& nodes = json1.getElementsByTags(tags);
	if (nodes.empty())
	{
		printf("%s(%d): NOT FOUND(tags: %s)\r\n",
			__FUNCTION__, __LINE__, tags);
		return;
	}

	acl::json json2(*nodes[0]);
	printf(">>>%s(%d)->json2(tags: %s) result:\r\n%s\r\n",
		__FUNCTION__, __LINE__, tags, json2.to_string().c_str());
	printf("-------------------------------------------------\r\n");
	printf(">>>%s(%d)->before set node's string:\r\n%s\r\n",
		__FUNCTION__, __LINE__, (*nodes[0]).to_string().c_str());
	printf("-------------------------------------------------\r\n");
	nodes[0]->set_tag("popup");
	nodes[0]->set_text("popup text");
	printf(">>>%s(%d)->after set node's string:\r\n%s\r\n",
		__FUNCTION__, __LINE__, (*nodes[0]).to_string().c_str());

	printf("-------------------------------------------------\r\n");
	printf(">>>>root to string:\r\n%s\r\n", json1.get_root().to_string().c_str());
	printf("-------------------------------------------------\r\n");

	acl::json json3(json1.get_root());
	printf(">>>%s(%d)->json3(from json's root) result:\r\n%s\r\n",
		__FUNCTION__, __LINE__, json3.to_string().c_str());

	printf("-------------------------------------------------\r\n");

	if (json1.to_string() == json3.to_string())
		printf(">>%s(%d): json1=json3, OK!\r\n", __FUNCTION__, __LINE__);
	else
	{
		printf("%s(%d): ERROR! json1 != json3\r\n", __FUNCTION__, __LINE__);
		printf(">>%s(%d)->json1:\r\n%s\r\n",
			__FUNCTION__, __LINE__, json1.to_string().c_str());
		printf(">>%s(%d)->json3:\r\n%s\r\n",
			__FUNCTION__, __LINE__, json3.to_string().c_str());
		exit (1);
	}
	printf("-------------------------------------------------\r\n");
}

int main(void)
{
	test();
	printf("enter any key to exit\r\n");
	getchar();
	return 0;
}

