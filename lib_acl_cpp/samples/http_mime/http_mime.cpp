// http_mime.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "acl_cpp/lib_acl.hpp"

static void test(const char* path, const char* boundary)
{
	acl::http_mime mime(boundary);
	mime.set_saved_path(path);
	acl::string buf;

	if (acl::ifstream::load(path, &buf) == false)
	{
		printf("load %s error\r\n", path);
		return;
	}

	mime.update(buf.c_str(), buf.length());
#if 0
	const std::list<acl::http_mime_node*>& nodes = mime.get_nodes();
	std::list<acl::http_mime_node*>::const_iterator cit = nodes.begin();
	for (; cit != nodes.end(); ++cit)
	{
		acl::http_mime_node* node = *cit;
		if (node->get_mime_type() == acl::HTTP_MIME_PARAM)
		{
			const char* name = node->get_name();
			const char* value = node->get_value();
			printf("name: %s, value: %s\r\n", name ? name : "null",
				value ? value : "null");
		}
		else if (node->get_mime_type() == acl::HTTP_MIME_FILE)
		{
			const char* filename = node->get_filename();
			printf("filename: %s\r\n", filename ? filename : "null");
			node->save(filename);
		}
	}
#else
	printf("length: %d\r\n", (int) buf.length());
	const std::list<acl::http_mime_node*>& nodes = mime.get_nodes();
	std::list<acl::http_mime_node*>::const_iterator cit = nodes.begin();
	for (; cit != nodes.end(); ++cit)
	{
		const char* n1, *f, *v;
		n1 = (*cit)->get_name();
		f = (*cit)->get_filename();
		v = (*cit)->get_value();
		printf(">>>name: %s, value: %s, file: %s<br>\r\n",
				n1 ? n1 : "null", v ? v : "null", f ? f : "null");
	}

	const acl::http_mime_node* node = mime.get_node("filename");
	if (node == NULL || node->get_mime_type() != acl::HTTP_MIME_FILE)
	{
		printf("filename not found\r\n");
		return;
	}
	const char* ptr = node->get_filename();
	if (ptr == NULL || *ptr == 0)
		printf("filename's value null\r\n");
	else
		printf("filename: %s\r\n", ptr);
#endif
}

int main(int argc, char* argv[])
{
	const char* path = "./tmp.txt";
	const char* boundary = "-----------------------------5169208281820";
//	const char* boundary = "------WebKitFormBoundaryztuvecMyltzibUyI";
//	const char* boundary = "--gvdrLIiwm31yiNkOc7Hr3HdHouL22D-P_49Q";
	if (argc >= 2)
		path = argv[1];
	test(path, boundary);
#ifdef WIN32
	printf("enter any key to exit ...\r\n");
	getchar();
#endif
	return 0;
}
