#include "stdafx.h"
#include <list>
#include <vector>
#include <map>
#include <stdio.h>
#include <iostream>
#include "struct.h"
#include "struct.gson.h"

void test_base()
{
	base b;
	b.a = 1;
	b.a_ptr = new int(1);
	b.acl_string = "a";
	b.acl_string_ptr = new acl::string("a");
	b.string = "b";
	b.string_ptr = 0;
	b.b = 2;
	b.b_ptr = new unsigned int( 2);
	b.c = 3;
	b.c_ptr = new int64_t(3);
	b.d = 4;
	b.d_ptr = new unsigned long(4);
	b.e = 5;
	b.e_ptr = new unsigned long long(5);
	b.f = 6;
	b.f_ptr = new long(6);
	b.g = 7;
	b.g_ptr = new long long(7);
	b.h = 9;
	b.h_ptr = new float(9);
	b.i = 10;
	b.i_ptr = new double(9);

	list1 obj;
	obj.b = b;
	obj.b_ptr = new base(b);
	obj.bases_list.push_back(b);
	obj.bases_list_ptr = new std::list<base>;
	obj.bases_list_ptr->push_back(b);
	obj.bases_ptr_list_ptr = new std::list<base*>;
	obj.bases_ptr_list_ptr->push_back(NULL);
	obj.bases_ptr_list_ptr->push_back(NULL);
	obj.bases_ptr_list_ptr->push_back(NULL);
	obj.bases_ptr_list_ptr->push_back(NULL);
	obj.string_map.insert(std::make_pair("key", "value"));
	obj.base_map.insert(std::make_pair("key", b));
	obj.int_map.insert(std::make_pair("key", 12121));
	obj.bool_map.insert(std::make_pair("key", false));

	obj.base_list_map.insert( std::make_pair("base", obj.bases_list));
	obj.vector_list_base.push_back(obj.bases_list);

	//set
	obj.str_set_.insert("hello1");
	obj.str_set_.insert("hello2");
	obj.str_set_.insert("hello3");
	obj.bool_set_.insert(true);
	obj.bool_set_.insert(false);
	obj.int_set_.insert(1);
	obj.int_set_.insert(11);
	obj.int_set_.insert(111);

	acl::json json;
	acl::json_node &node = acl::gson(json, obj);
	printf("%s\n\n", node.to_string().c_str());


	list1 obj2;
	acl::json json2;
	json2.update(node.to_string().c_str());
	printf("%s\n\n",json2.to_string().c_str());
	std::pair<bool,std::string> ret = acl::gson(json2.get_root(), obj2);
	if(ret.first == false)
		printf("%s\n\n",ret.second.c_str());
	else
	{
		acl::json json3;
		acl::json_node &node3 = acl::gson(json3, obj2);
		printf("%s\n\n", node3.to_string().c_str());
	}
	
}
void genfile()
{
	acl::gsoner gr;

	gr.read_file("struct.h");
	gr.parse_code();
	gr.gen_gson();
}

int main(void)
{
	test_base();
	getchar();
	return 0;
}
