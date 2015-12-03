// xml.cpp : 定义控制台应用程序的入口点。
//
#include "stdafx.h"
#include <list>

using namespace std;

static void test(void)
{
	const char *data =
		"<?xml version=\"1.0\"?>\r\n"
		"<?xml-stylesheet type=\"text/xsl\"\r\n"
		"	href=\"http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl\"?>\r\n"
		"<!DOCTYPE refentry PUBLIC \"-//OASIS//DTD DocBook XML V4.1.2//EN\"\r\n"
		"	\"http://www.oasis-open.org/docbook/xml/4.1.2/docbookx.dtd\" [\r\n"
		"	<!ENTITY xmllint \"<command>xmllint</command>\">\r\n"
		"]>\r\n"
		"<root>test\r\n"
		"	<!-- <edition> - <!--0.5--> - </edition> -->\r\n"
		"	<user name = user_name>zsx\r\n"
		"		<age>38</age>\r\n"
		"	</user>\r\n"
		"</root>\r\n"
		"<!-- <edition><!-- 0.5 --></edition> -->\r\n"
		"<!-- <edition>0.5</edition> -->\r\n"
		"<!-- <edition> -- 0.5 -- </edition> -->\r\n"
		"<root name='root' id='root_id'>test</root>\r\n";
	acl::xml1 xml;

#if 1
	const char* p = data;
	char tmp[2];
	while (*p)
	{
		tmp[0] = *p++;
		tmp[1] = 0;
		xml.update(tmp);
	}
#else
	xml.update(data);
#endif

	const vector<acl::xml_node*>& elements = xml.getElementsByTagName("user");

	if (!elements.empty()) {
		vector<acl::xml_node*>::const_iterator cit = elements.begin();
		for (; cit != elements.end(); cit++) {
			acl::xml_node *node = *cit;
			printf("tagname: %s, text: %s\n", node->tag_name() ? node->tag_name() : "",
				node->text() ? node->text() : "");

			const acl::xml_attr* attr = (*cit)->first_attr();
			while (attr)
			{
				printf("test1: %s=%s\r\n", attr->get_name(), attr->get_value());
				attr = (*cit)->next_attr();
			}
		}
	}

	xml.reset();

	//////////////////////////////////////////////////////////////////////

	const char* s2 = "<html><head>Title\r\n</head><body>各位同学：\r\n"
		"本月考勤统计日为20日，烦请各位同学在19日下班之前尽快将个人需要申诉的事由找部门领导审批通\r\n"
		"过。逾期不再修改考勤。\r\n"
		"另烦请本月有请年假、事假、病假的同学补发邮件给部门领导审批并转发给行政部。谢谢各位同学配合。\r\n"
		"如在考勤日未收到以上邮件，我部门将按照考勤严格执行不再做任何修改。</body></html>\r\n";
	xml.update("<dummy_root>");
	xml.update(s2);
	xml.update("</dummy_root>");
	const acl::string& r2 = xml.getText();
	printf("\r\n|%s|\r\n", r2.c_str());

	printf("------------ the result is: ------------------\r\n");
	acl::string buf;
	xml.build_xml(buf);
	printf("%s\r\n", buf.c_str());

	//////////////////////////////////////////////////////////////////////////
	acl::xml1 xml2;
	acl::xml_node& root = xml2.get_root();
	acl::xml_node* node1, *node2, *node3, *node4;

	node1 = &xml2.create_node("test1");
	(*node1).add_attr("name1_1", "value1_1")
		.add_attr("name1_2", "value1_2")
		.add_attr("name1_3", "value1_3");
	root.add_child(node1);

	printf("----------------------node1's attr valuse list----------------\r\n");
	const acl::xml_attr* attr1 = node1->first_attr();
	while (attr1)
	{
		printf("node1: %s=%s\r\n", attr1->get_name(), attr1->get_value());
		attr1 = node1->next_attr();
	}
	printf("----------------------node1's attr end  list------------------\r\n");

	node2 = &xml2.create_node("test2");
	(*node2).add_attr("name2_1", "value2_1")
		.add_attr("name2_2", "value2_2");
	root.add_child(node2);

	node2 = &xml2.create_node("test11");
	(*node2).add_attr("name11_1", "value11_1")
		.add_attr("name11_2", "value11_2")
		.add_attr("name11_3", "value11_3");
	node1->add_child(node2);
	node2->set_text("hello world!");

	node1 = &xml2.create_node("test3");
	(*node1).add_attr("name3_1", "value3_1")
		.add_attr("name3_2", "value3_2");
	node2 = &xml2.create_node("test4");
	(*node2).add_attr("name4_1", "value4_1")
		.add_attr("name4_2", "value4_2")
		.add_attr("name4_3", "value4_3");
	root.add_child(node1).add_child(node2);

	node1 = &xml2.create_node("test5");
	root.add_child(node1);
	node2 = &xml2.create_node("test51");
	(*node2).add_attr("name51_1", "value51_1")
		.add_attr("name51_2", "value51_2");
	(*node1).add_child(node2);

	node3 = &xml2.create_node("test52");
	(*node3).add_attr("name52_1", "value52_1")
		.add_attr("name52_2", "value52_2");
	node4 = &xml2.create_node("test53");
	(*node4).add_attr("name53_1", "value53\"_1")
		.add_attr("name53_2", "value53_2");
	(*node1).add_child(node3).add_child(node4);

	root.add_child("test6", true, "text6")
		.add_child("test61", true, "text61")
			.add_attr("name61_1", "value61_1")
			.add_attr("name61_2", "value61_2")
			.add_attr("name61_3", "value61_3")
			.add_child("test611", true, "text611")
				.add_attr("name611_1", "value611_1")
				.add_attr("name611_2", "value611_2")
				.add_attr("name611_3", "value611_3")
				.get_parent()
			.get_parent()
		.add_child("test62", true, "text62")
			.add_attr("name62_1", "value62_1")
			.add_attr("name62_2", "value62_2")
			.add_attr("name62_3", "value62_3")
			.get_parent()
		.add_attr("name6_1", "value6_1")
		.add_attr("name6_2", "value6_2")
		.add_attr("name6_3", "value6_3");

	buf.clear();
	xml2.build_xml(buf);

	printf("-----------------xml2------------------------\r\n");
	printf("%s\r\n", buf.c_str());

	printf("-----------------xml2 iterator --------------\r\n");
	acl::xml_node* iter = xml2.first_node();
	while (iter)
	{
		if (iter->tag_name())
		{
			printf("tag: %s", iter->tag_name());
			if (iter->text())
				printf(", text: %s\r\n", iter->text());
			else
				printf("\r\n");
		}
		iter = xml2.next_node();
	}

	printf("------------------root first level child---------\r\n");
	iter = xml2.get_root().first_child();
	int  n = 0;
	while (iter)
	{
		n++;
		if (iter->tag_name())
			printf("tag: %s", iter->tag_name());
		if (iter->text())
			printf(", text: %s\r\n", iter->text());
		else
			printf("\r\n");
		iter = xml2.get_root().next_child();
	}
	if (n == xml2.get_root().children_count())
		printf("ok, children_count: %d\n", n);
	else
	{
		printf("error, n: %d, children_count: %d, enter any key to continue\r\n",
			n, xml2.get_root().children_count());
		getchar();
	}
}

static void test2(void)
{
	acl::xml1 body;
	body.get_root().add_child("test", true)
		.add_attr("mail", "\"zsxxsz1\" zsxxsz1@test.com")
		.add_attr("title", "中<国>人\'，</test>中国人\"，中国人\\\"")
		.add_attr("from", "\"zsxxsz2\" zsxxsz2@test.com");
	acl::string buf;
	body.build_xml(buf);
	printf("data: %s\r\n", buf.c_str());

	printf("------------------------------------------\r\n");

	acl::xml1 xml(buf.c_str());
	const acl::xml_node* node = xml.getFirstElementByTag("test");
	if (node == NULL)
		printf("test tag null\r\n");
	else
	{
		const char* mail = (*node)["mail"];
		const char* title = (*node)["title"];
		const char* from = (*node)["from"];
		printf("mail: %s\r\n", mail ? mail : "null");
		printf("title: %s\r\n", title ? title : "null");
		printf("from: %s\r\n", from ? from : "null");
	}
}

static void get_html(const char* file)
{
	acl::string buf;
	if (acl::ifstream::load(file, &buf) == false)
	{
		printf("load %s error %s\r\n", file, acl::last_serror());
		return;
	}

	acl::xml1 xml(buf.c_str());
	const acl::string& text = xml.getText();
	printf("text:{%s}\r\n", text.c_str());
}

int main(int argc, char* argv[])
{
	if (argc >= 2)
	{
		get_html(argv[1]);
		return 0;
	}

	test2();
	test();
	printf("enter any key to exit\r\n");
	getchar();
	return 0;
}
