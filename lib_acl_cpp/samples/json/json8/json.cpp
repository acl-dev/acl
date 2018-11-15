#include "stdafx.h"

int main()
{
	const char* sss =
		"["
		"{\"DataKey\":\"BindRule\",\"DataValue\":{\"waittime\":\"7\"},\"null_key\":null},"
		"{\"DataKey\":\"BindRule\",\"DataValue\":{\"waittime\":\"7\"},\"null_key\":null}"
		"]";

	acl::json json3(sss);
	const char* tags = "DataValue";

	printf("----------------------------------------------------\r\n");
	printf(">>%s\r\n", sss);
	const acl::string& s = json3.to_string();
	printf(">>%s\r\n", s.c_str());

	if (json3.to_string() == sss)
		printf("parse OK, enter any key to continue\r\n");
	else
	{
		printf("parse ERROR\r\n");
		return 1;
	}
	
	getchar();

	printf("----------------------------------------------------\r\n");

	acl::json_node* iter = json3.first_node();
	while (iter)
	{
		const char* tag = iter->tag_name();
		const char* txt = iter->get_text();
		printf("tag: %s, txt: %s\r\n", tag ? tag : "null", txt ? txt : "null");
		if (txt)
			iter->set_text("hello");
		printf("tag: %s, txt: %s\r\n", tag ? tag : "null", txt ? txt : "null");
		iter = json3.next_node();
	}
	printf("----------------------------------------------------\r\n");

	const std::vector<acl::json_node*>& nodes = json3.getElementsByTags(tags);
	if (nodes.empty() == false)
		printf(">>>%s\r\n", nodes[0]->to_string().c_str());

	return 0;
}
