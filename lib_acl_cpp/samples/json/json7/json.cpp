#include "stdafx.h"

int main(void)
{
	acl::string buf;                                                       
	acl::json json;
	const char* s;

	////////////////////////////////////////////////////////////////////
#if 1
	acl::json_node& root = json.get_root();

	root.add_text("name1", "value1")
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

	json.build_json(buf);
	printf("%s\r\n", buf.c_str());
	s = "{\"name1\":\"value1\",\"name2\":111111,\"name3\":true,"
		"[100,101,102],[true,false,true],"
		"[\"name11\":\"value11\",\"name12\":1000,\"name13\":true],"
		"[\"node1\":{\"name12_1_1\":\"value12_1_1\"},"
		 "\"node2\":{\"name12_1_2\":\"value12_1_2\"},"
		 "{\"name12_1_3\":\"value12_1_3\"}]}";
	if (strcmp(s, buf.c_str()) != 0)
	{
		printf("%s\r\n", s);
		printf("%s(%d): Error\r\n", __FUNCTION__, __LINE__);
		return 1;
	}

	////////////////////////////////////////////////////////////////////

	printf("\r\n");
	json.reset();
	buf.clear();
	acl::json_node& array = json.create_array();
	json.get_root().add_child("array", array);

	acl::json_node& node1 = json.create_node();
	node1.add_number("number11", 100);
	node1.add_number("number12", 200);
	node1.add_number("number13", 300);
	array.add_child(node1);

	acl::json_node& node2 = json.create_node();
	node2.add_number("number21", 1000)
		.add_number("number22", 2000)
		.add_number("number23", 3000);
	array.add_child(node2);

	acl::json_node& node3 = json.create_node();
	node3.add_text("number31", "value31")
		.add_text("number32", "value32")
		.add_text("number33", "value33");
	array.add_child(node3);

	acl::json_node& node4 = json.create_node();
	node4.add_bool("number41", true).add_bool("number42", false);
	array.add_child(node4);

	json.build_json(buf);
	printf("%s\r\n", buf.c_str());

	s = "{\"array\":["
	  "{\"number11\":100,\"number12\":200,\"number13\":300},"
	  "{\"number21\":1000,\"number22\":2000,\"number23\":3000},"
	  "{\"number31\":\"value31\",\"number32\":\"value32\",\"number33\":\"value33\"},"
	  "{\"number41\":true,\"number42\":false}]}";
	if (strcmp(s, buf.c_str()) != 0)
	{
		printf("%s\r\n", s);
		printf("%s(%d): Error\r\n", __FUNCTION__, __LINE__);
		return 1;
	}

	////////////////////////////////////////////////////////////////////

	printf("\r\n");
	json.reset();
	buf.clear();
	json.build_json(buf);
	printf("%s\r\n", buf.c_str());

	s = "{}";
	if (strcmp(s, buf.c_str()) != 0)
	{
		printf("%s\r\n", s);
		printf("%s(%d): Error\r\n", __FUNCTION__, __LINE__);
		return 1;
	}

	////////////////////////////////////////////////////////////////////

	printf("\r\n");
	json.reset();
	buf.clear();
	json.get_root().add_array();
	json.build_json(buf);
	printf("%s\r\n", buf.c_str());

	s = "{[]}";
	if (strcmp(s, buf.c_str()) != 0)
	{
		printf("%s\r\n", s);
		printf("%s(%d): Error\r\n", __FUNCTION__, __LINE__);
		return 1;
	}

	////////////////////////////////////////////////////////////////////

	printf("\r\n");
	json.reset();
	buf.clear();
	json.get_root().add_array(true)
		.add_array_number(10)
		.add_array_number(100)
		.add_array_number(1000);
	json.build_json(buf);
	printf("%s\r\n", buf.c_str());

	s = "{[10,100,1000]}";
	if (strcmp(s, buf.c_str()) != 0)
	{
		printf("%s\r\n", s);
		printf("%s(%d): Error\r\n", __FUNCTION__, __LINE__);
		return 1;
	}

	////////////////////////////////////////////////////////////////////

	printf("\r\n");
	json.reset();
	buf.clear();
	acl::json_node& array1 = json.create_array();
	json.get_root().add_child(array1);

	array1.add_array_text("hello").add_array_text("world");
	json.build_json(buf);
	printf("%s\r\n", buf.c_str());
	
	s = "{[\"hello\",\"world\"]}";
	if (strcmp(s, buf.c_str()) != 0)
	{
		printf("%s\r\n", s);
		printf("%s(%d): Error\r\n", __FUNCTION__, __LINE__);
		return 1;
	}


	////////////////////////////////////////////////////////////////////

	printf("\r\n");
	json.reset();
	buf.clear();
	acl::json_node& array2 = json.create_array();
	json.get_root().add_child(array2);

	array2.add_child(json.create_array(), true)
			.add_child(json.create_array(), true)
				.add_child(json.create_array());
	json.build_json(buf);
	printf("%s\r\n", buf.c_str());
	
	s = "{[[[[]]]]}";
	if (strcmp(s, buf.c_str()) != 0)
	{
		printf("%s\r\n", s);
		printf("%s(%d): Error\r\n", __FUNCTION__, __LINE__);
		return 1;
	}

	////////////////////////////////////////////////////////////////////

	printf("\r\n");
	json.reset();
	buf.clear();
	acl::json_node& array3 = json.create_array();
	json.get_root().add_child(array3);

	acl::json_node& n3 = json.create_node();
//	n3.add_child("hello", "world");
	array3.add_child(n3);
	json.build_json(buf);
	printf("%s\r\n", buf.c_str());

	s = "{[{}]}";
	if (strcmp(s, buf.c_str()) != 0)
	{
		printf("%s\r\n", s);
		printf("%s(%d): Error\r\n", __FUNCTION__, __LINE__);
		return 1;
	}

	////////////////////////////////////////////////////////////////////

	printf("\r\n");
	json.reset();
	buf.clear();
	acl::json_node& array4 = json.create_array();
	json.get_root().add_child(array4);

	acl::json_node& n4 = json.create_node();
	n4.add_text("hello", "world");
	array4.add_child(n4);
	json.build_json(buf);
	printf("%s\r\n", buf.c_str());

	s = "{[{\"hello\":\"world\"}]}";
	if (strcmp(s, buf.c_str()) != 0)
	{
		printf("%s\r\n", s);
		printf("%s(%d): Error\r\n", __FUNCTION__, __LINE__);
		return 1;
	}

#endif
	////////////////////////////////////////////////////////////////////

	printf("\r\n");
	json.reset();
	buf.clear();
	acl::json_node& array5 = json.create_array();
	json.get_root().add_child(array5);

	array5.add_child(json.create_node());
	array5.add_child(json.create_node());
	json.build_json(buf);
	printf("%s\r\n", buf.c_str());
	
	s = "{[{},{}]}";
	if (strcmp(s, buf.c_str()) != 0)
	{
		printf("%s\r\n", s);
		printf("%s(%d): Error\r\n", __FUNCTION__, __LINE__);
		return 1;
	}

	////////////////////////////////////////////////////////////////////

	printf("\r\n");
	json.reset();
	buf.clear();
	acl::json_node& array6 = json.create_array();
	json.get_root().add_child(array6);

	array6.add_array_text("hello").add_array_text("world");
	array6.add_child(json.create_array(), true)
			.add_child(json.create_array(), true)
				.add_child(json.create_array());
	array6.add_child(json.create_node());
	array6.add_child(json.create_node());
	json.build_json(buf);
	printf("%s\r\n", buf.c_str());
	
	s = "{[\"hello\",\"world\",[[[]]],{},{}]}";
	if (strcmp(s, buf.c_str()) != 0)
	{
		printf("%s\r\n", s);
		printf("%s(%d): Error\r\n", __FUNCTION__, __LINE__);
		return 1;
	}


	////////////////////////////////////////////////////////////////////

	printf("\r\n");
	json.reset();
	buf.clear();
	json.get_root().add_text("name", "value");
	json.build_json(buf);
	printf("%s\r\n", buf.c_str());
	s = "{\"name\":\"value\"}";
	if (strcmp(s, buf.c_str()) != 0)
	{
		printf("%s\r\n", s);
		printf("%s(%d): Error\r\n", __FUNCTION__, __LINE__);
		return 1;
	}

	////////////////////////////////////////////////////////////////////

	printf("\r\n");
	printf("ALL OVER NOW!\r\n\r\n");

#ifdef WIN32
	printf("enter any key to exit!\r\n");
	getchar();
#endif
	return 0;
}
