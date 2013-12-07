// json.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <list>
#include <vector>
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/json.hpp"

using namespace std;

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
    " }\r\n,"
    " 'help': 'hello world!',\r\n"
    " 'menuitem2': [\r\n"
    "   {'value': 'New', 'onclick': 'CreateNewDoc()'},\r\n"
    "   {'value': 'Open', 'onclick': 'OpenDoc()'},\r\n"
    "   {'value': 'Close', 'onclick': 'CloseDoc()'},\r\n"
    "   [{'value': 'Save', 'onclick': 'SaveDoc()'}]"
    " ]\r\n"
    "}\r\n";

static void test(void)
{
	acl::json json(default_data);

	printf("-------------------------------------------------\r\n");
	printf("%s\r\n", json.to_string().c_str());

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
	printf("from: %s\r\n", default_data);
	printf("-------------------------------------------------\r\n");
	printf(">>> tags: %s\r\n", tags);

	const std::vector<acl::json_node*>& nodes = json.getElementsByTags(tags);
	if (nodes.empty())
	{
		printf("NOT FOUND\r\n");
		return;
	}

	acl::json json2(*nodes[0]);
	printf(">>> result: %s\r\n", json2.to_string().c_str());
	printf("-------------------------------------------------\r\n");
	printf(">>> before set node's string: %s\r\n", (*nodes[0]).to_string().c_str());
	printf("-------------------------------------------------\r\n");
	nodes[0]->set_tag("popup");
	nodes[0]->set_text("popup text");
	printf(">>> after set node's string: %s\r\n", (*nodes[0]).to_string().c_str());

	printf("-------------------------------------------------\r\n");

	acl::json json3(json.get_root());
	printf("result: %s\r\n", json3.to_string().c_str());

	printf("-------------------------------------------------\r\n");

	if (json.to_string() == json3.to_string())
		printf("OK\r\n");
	else
		printf("ERROR\r\n");
	printf("-------------------------------------------------\r\n");
}

int main(void)
{
	test();
	printf("enter any key to exit\r\n");
	getchar();
	return 0;
}

