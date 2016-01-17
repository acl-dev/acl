#include "stdafx.h"

static void attr_print(const acl::xml_attr& attr, int depth)
{
	for (int i = 0; i < depth; i++)
		printf("\t");

	printf("%s=\"%s\"\r\n", attr.get_name(), attr.get_value());
}

static void node_attr_print(const acl::xml_node& node, int depth)
{
	const acl::xml_attr* attr = node.first_attr();
	while (attr)
	{
		attr_print(*attr, depth);
		attr = node.next_attr();
	}
}

static void xml_node_print(const acl::xml_node& node, int depth)
{
	for (int i = 0; i < depth; i++)
		printf("\t");

	printf("tag: %s\r\n", node.tag_name());

	const char* txt = node.text();
	for (int i = 0; i < depth; i++)
		printf("\t");
	printf("text: {%s}\r\n", txt ? txt : "");

	node_attr_print(node, depth + 1);
}

static void xml_node_walk(acl::xml_node& node, int depth)
{
	acl::xml_node* child = node.first_child();

	while (child)
	{
		xml_node_print(*child, depth);
		xml_node_walk(*child, depth + 1);
		child = node.next_child();
	}
}

static void test_build(void)
{
	const char* local_file = "./local.map";
	acl::xml2 xml(local_file, 1024000);
	acl::xml_node& root = xml.get_root();
	char  txt[102400];

	memset(txt, 'x', sizeof(txt));
	txt[sizeof(txt) - 1] = 0;

	root.add_child("users", true)
		.add_child("user", true)
			.add_attr("name", "zsxxsz")
			.add_attr("age", 100)
			.set_text(txt)
			.get_parent()
		.add_child("user", true)
			.add_attr("name", "zsx1")
			.add_attr("age", 102);

	acl::xml_node& node = xml.create_node("name", "value");
	(void) node;

	acl::string buf;
	xml.build_xml(buf);

	printf("%s\r\n", buf.c_str());
	printf("-------------- walk xml node ------------------------\r\n");
	xml_node_walk(xml.get_root(), 0);
	printf("-------------- walk xml node end --------------------\r\n");
	printf("-------------- print xml ----------------------------\r\n");
	printf("[%s]\r\n", xml.to_string());
	printf("-------------- print xml end ------------------------\r\n");
}

static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n", procname);
}

int main(int argc, char* argv[])
{
	int   ch;

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

	test_build();
	return 0;
}
