#include "stdafx.h"
#include <list>
#include <vector>
#include <map>
#include <stdio.h>
#include <iostream>
#include <time.h>
#include "struct.h"  // 由 gson 工具根据 struct.stub 转换而成
#include "struct.gson.h"    // 由 gson 工具根据 struct.stub 生成

static void test3(void)
{
	//const char* s = "{\"files\":['1,;,;i,;]}[]]}}}}    ' ]}";
	//const char* s = "{\"files\":[\";   ;  \"]}";
	//const char* s = "{\"files\":[\";   ;  \"]}";
	const char* s = "{'files':[], 'values':[1, 2, 3, 4]}";
	//const char* s = "{'files':[], 'values':[]}";

	acl::json json(s);
	files_outdate files;
	acl::string err;
	if (acl::deserialize<files_outdate>(json, files, &err) == false) {
		printf("parse error=%s\r\n", err.c_str());
		return;
	}

	printf("ok, files size=%lu, values size=%lu\r\n",
		(unsigned long) files.files.size(),
		(unsigned long) files.values.size());

	for (std::vector<acl::string>::const_iterator cit = files.files.begin();
		cit != files.files.end(); ++cit) {

		printf("[%s]\r\n", (*cit).c_str());
	}

	for (std::vector<int>::const_iterator cit = files.values.begin();
		cit != files.values.end(); ++cit) {

		printf("int->%d\r\n", *cit);
	}
}

int main(void)
{
	test3();
	return 0;
}
