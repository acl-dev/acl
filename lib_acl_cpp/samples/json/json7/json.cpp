#include "stdafx.h"

int main(void)
{
	acl::json json;
	acl::json_node& root = json.get_root();

	root.add_child("name1", "value1")
		.add_number("name2", 111111)
		.add_bool("name3", true);
	root.add_array(true)
		.add_array_number(100)
		.add_array_number(101)
		.add_array_number(102);
	root.add_array(true)
		.add_array_bool(true)
		.add_array_bool(false)
		.add_array_bool(true);
	root.add_array(true)
		.add_text("name11", "value11")
		.add_number("name12", 1000)
		.add_bool("name13", true);
	root.add_array(true)
		.add_child("node1",
			json.create_node()
				.add_child(json.create_node("name12_1_1", "value12_1_1")))
		.add_child("node2",
			json.create_node()
				.add_child(json.create_node("name12_1_2", "value12_1_2")))
		.add_child(json.create_node()
				.add_child(json.create_node("name12_1_3", "value12_1_3")));

	acl::string buf;                                                       
	json.build_json(buf);
	printf("%s\r\n", buf.c_str());

#ifdef WIN32
	printf("enter any key to exit!\r\n");
	getchar();
#endif
	return 0;
}
