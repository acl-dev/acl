// xml.cpp : 定义控制台应用程序的入口点。
//
#include "stdafx.h"
#include <list>

using namespace std;

static void test()
{
	acl::string buf;
	acl::xml xml;
	acl::xml_node& root = xml.get_root();
	acl::xml_node& server = xml.create_node("server");
	root.add_child(server);

	acl::xml_node& proc1 = xml.create_node("porc");
	server.add_child(proc1);
	proc1.add_child("pid", false, "1111");
	proc1.add_child("conns", false, "111");
	proc1.add_child("load", false, "0.5");

	acl::xml_node& proc2 = xml.create_node("proc");
	server.add_child(proc2);
	proc2.add_child("pid", false, "2111");
	proc2.add_child("conns", false, "211");
	proc2.add_child("load", false, "2.5");

	xml.build_xml(buf);
	printf("%s\r\n", buf.c_str());
}

static void print_attrs(acl::xml_node& node)
{
	const acl::xml_attr* iter = node.first_attr();

	if (iter != NULL)
		printf("; attrs:");

	while (iter)
	{
		printf(" %s=%s;", iter->get_name(), iter->get_value());
		iter = node.next_attr();
	}
}

static void test2()
{
	acl::xml xml;
	acl::xml_node& root = xml.get_root();
	root.add_child("users", true)
		.add_child("user", true)
			.add_attr("account", "\"zsxxsz\" <zsxxsz@263.net>")
			.add_attr("sex", "<male>")
			.add_attr("addr", "\"Beijing\"")
			.add_attr("nick", "<xsz@sina.com>")
			.add_child("name", false, "zsxxsz")
			.add_child("email", false, "\'zsxxsz\' <zsxxsz@263.net>")
			.add_child("sex", false, "male")
			.get_parent()
		.add_child("user", true)
			.add_attr("account", "\"zsxxsz\" <zsxxsz@263.net>")
			.add_attr("sex", "<male>")
			.add_attr("addr", "\"&Beijing&\"")
			.add_attr("nick", "<xsz@sina.com>")
			.add_child("name", false, "zsx1")
			.add_child("email", false, "\"zsx1\" <zsx1@263.net>")
			.add_child("age", false, "22");

	acl::string buf;
	xml.build_xml(buf);
	printf("%s\r\n", buf.c_str());

	xml.reset();
	xml.xml_decode(true);

	const char* p = buf.c_str();
	char tmp[2];
	while (*p)
	{
		tmp[0] = *p;
		tmp[1] = 0;
		xml.update(tmp);
		++p;
	}

	acl::xml_node* iter = xml.first_node();
	while (iter)
	{
		const char* txt = iter->text();
		printf("tag: %s; txt: %s", iter->tag_name(), txt ? txt : "null");
		print_attrs(*iter);
		printf("\r\n");
		iter = xml.next_node();
	}
}

int main(void)
{
	test();
	test2();
#ifdef WIN32
	printf("enter any key to exit\r\n");
	getchar();
#endif
	return 0;
}
