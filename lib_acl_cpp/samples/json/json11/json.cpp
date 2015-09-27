#include "stdafx.h"

int main()
{
	const char* sss =
		"{\r\n"
		"	\"data\" : \"dGVzdHFxcQ==\", \r\n"
		"	\"receiver_id\" : [\r\n"
		"		\"1442365683\"\r\n"
		"	],\r\n"
		"	\"extra\" : \"\",\r\n"
		"	\"group_id\" : \"\"\r\n"
		"}\r\n";
	acl::json json;
	const char* ptr = json.update(sss);

	const std::vector<acl::json_node*> &receiver = 
		json.getElementsByTagName("receiver_id");

	std::vector<acl::json_node*>::const_iterator cit;
	for (cit = receiver.begin(); cit != receiver.end(); ++cit)
	{
		acl::json_node* node = (*cit)->get_obj();
		if (node == NULL)
		{
			printf("get_obj null error\r\n");
			return 1;
		}

		printf("--------list all elements of the array --------\r\n");

		acl::json_node* child = node->first_child();
		while (child)
		{
			const char* txt = child->get_text();
			if (txt && *txt)
				printf("%s\r\n", txt);
			else
				printf("null\r\n");
			child = node->next_child();
		}

		printf("----------------- list end --------------------\r\n");
	}

	printf("-------------------------------------------------------\r\n");

	printf("%s\r\n", sss);

	printf("-------------------------------------------------------\r\n");

	printf("json finish: %s, left char: %s\r\n",
		json.finish() ? "yes" : "no", ptr);

	printf(">>>to string: %s\r\n", json.to_string().c_str());

	return 0;
}
