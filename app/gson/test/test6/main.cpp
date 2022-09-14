#include "stdafx.h"
#include <stdio.h>
#include <time.h>
#include <vector>
#include <map>

#include "struct.h"
#include "struct.gson.h"

static void show_user(user& u)
{
	printf("\t\t\tuser --> name: %s, addr: %s, male: %s, age: %d\r\n",
		u.name.empty() ? "null" : u.name.c_str(),
		u.addr.empty() ? "null" : u.addr.c_str(),
		u.male ? "yes" : " no",
		u.age);
}

static void show_group(const std::string& tag, const group& g)
{
	printf("\ttag --> %s\r\n", tag.c_str());

	printf("\t\tname --> %s\r\n", g.name.c_str());
	for (auto cit : g.users) {
		show_user(cit);
	}
}

static void show_company(const company& com)
{
	printf("\r\ncompany --> %s\r\n", com.name.c_str());

	for (auto cit : com.groups) {
		show_group(cit.first, cit.second);
	}
}

static void test(void)
{
	company com;
	com.name = "263";

	group g;

	g.name = "dev1";
	g.users.emplace_back("zsx11", 11, true);
	g.users.emplace_back("zsx12", 12, false);
	g.users.emplace_back("zsx13", 13, true);
	com.groups["group1"] = g;

	g.users.clear();

	g.name = "dev2";
	g.users.emplace_back("zsx21", 21, true);
	g.users.emplace_back("zsx22", 22, false);
	g.users.emplace_back("zsx23", 23, true);
	com.groups["group2"] = g;

	g.users.clear();

	g.name = "dev3";
	g.users.emplace_back("zsx31", 31, true);
	g.users.emplace_back("zsx32", 32, false);
	g.users.emplace_back("zsx33", 33, true);
	com.groups["group3"] = g;

	acl::json json;
	acl::json_node& node = acl::gson(json, com);

	printf("------------------------------------------------------\r\n");
	printf("node  to_string: %s\r\n", node.to_string().c_str());
	printf("------------------------------------------------------\r\n");

	company com1;
	acl::json json1;
	json1.update(node.to_string());

	printf("json1 to_string: %s\r\n", json1.to_string().c_str());
	printf("------------------------------------------------------\r\n");

	std::pair<bool, std::string> ret = acl::gson(json1.get_root(), com1);
	if (ret.first == false) {
		printf("error: %s\r\n", ret.second.c_str());
	} else {
		show_company(com1);
	}
}

static const char* data =
 "{'name':'263',\r\n"
 "  'groups':[\r\n"
 "    {'group1':{\r\n"
 "      'name':'dev1',\r\n"
 "      'users':[\r\n"
 "        {'name':'zsx11','age':11,'male':true,'addr':''},\r\n"
 "        {'name':'zsx12','age':12,'male':false,'addr':''},\r\n"
 "        {'name':'zsx13','age':13,'male':true,'addr':''}\r\n"
 "      ]}},\r\n"
 "    {'group2':{\r\n"
 "      'name':'dev2',\r\n"
 "      'users':[\r\n"
 "        {'name':'zsx21','age':21,'male':true,'addr':''},\r\n"
 "        {'name':'zsx22','age':22,'male':false,'addr':''},\r\n"
 "        {'name':'zsx23','age':23,'male':true,'addr':''}\r\n"
 "      ]}},\r\n"
 "    {'group3':{\r\n"
 "      'name':'dev3',\r\n"
 "      'users':[\r\n"
 "        {'name':'zsx31','age':31,'male':true,'addr':''},\r\n"
 "        {'name':'zsx32','age':32,'male':false,'addr':''},\r\n"
 "        {'name':'zsx33','age':33,'male':true,'addr':''}\r\n"
 "      ]}}\r\n"
 "  ]}";

static void parse(void)
{
	printf("\r\nOriginal json data:\r\n\r\n");
	printf("%s\r\n", data);

	acl::json json(data);

	company com;
	std::pair<bool, std::string> ret = acl::gson(json.get_root(), com);
	show_company(com);
}

int main(void)
{
	test();

	printf("Enter any key to continue ...");
	fflush(stdout);
	getchar();
	parse();
	return 0;
}
