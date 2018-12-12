#include "stdafx.h"
#include <list>
#include <vector>
#include <map>
#include <stdio.h>
#include <iostream>
#include <time.h>
#include "struct.h"
#include "struct.gson.h"

static void print_user(user& u)
{
	printf("\t\tuser ---> name: %s, addr: %s, male: %s, age: %d\r\n",
		u.get_name(), u.addr.c_str(), u.male ? "yes" : "no", u.age);
}

static void print_group(const group& g)
{
	printf("\tgroup --> name: %s\r\n", g.name.c_str());
	for (auto cit : g.users)
		print_user(cit);
}

static void print_company(const company& com)
{
	printf("\r\ncompany ---> name: %s, addr: %s\r\n",
		com.name.c_str(), com.addr.c_str());

	for (auto cit : com.groups)
		print_group(cit);

	for (auto cit : com.users)
		printf("%s=%s\r\n", cit.first.c_str(), cit.second.c_str());
}

static void test(void)
{
	company com;
	com.name = "263";
	com.addr = "beijing";

	group g;

	g.name = "dev1";
	g.users.emplace_back("zsx11", 11, true);
	g.users.emplace_back("zsx12", 12, false);
	g.users.emplace_back("zsx13", 13, true);
	com.groups.emplace_back(g);

	g.name = "dev2";
	g.users.clear();
	g.users.emplace_back("zsx21", 11, true);
	g.users.emplace_back("zsx22", 12, false);
	g.users.emplace_back("zsx23", 13, true);
	com.groups.emplace_back(g);

	g.name = "dev3";
	g.users.clear();
	g.users.emplace_back("zsx31", 11, true);
	g.users.emplace_back("zsx32", 12, false);
	g.users.emplace_back("zsx33", 13, true);
	com.groups.emplace_back(g);

	g = {"dev4", {{"zsx41", 11, true}, {"zsx42", 11, true}}};
	com.groups.emplace_back(g);

	g.set_name("dev5");
	g.users.clear();
	user u;
	u.set_name("zsx51").set_age(11).set_male(true);
	g.users.push_back(u);
	u.set_name("zsx52").set_age(12).set_male(false);
	g.users.push_back(u);
	u.set_name("zsx53").set_age(13).set_male(true).set_nick("zsxxsz");
	g.users.push_back(u);
	com.groups.push_back(g);

	com.users["zsx1"] = "xsz1";
	com.users["zsx2"] = "xsz2";
	com.users["zsx3"] = "xsz3";

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
	if (ret.first == false)
		printf("error: %s\r\n", ret.second.c_str());
	else
	{
		printf("==================== All OK ===================\r\n");
		print_company(com1);
	}
}

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n", procname);
}

int main(int argc, char* argv[])
{
	int  ch;

	while ((ch = getopt(argc, argv, "h")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		default:
			break;
		}
	}

	test();
	printf("Enter any key to continue ..."); fflush(stdout); getchar();
	return 0;
}
