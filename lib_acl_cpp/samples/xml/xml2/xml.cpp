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

int main(void)
{
	test();
#ifdef WIN32
	printf("enter any key to exit\r\n");
	getchar();
#endif
	return 0;
}
