#include "stdafx.h"
#include <list>
#include <vector>
#include <map>
#include <stdio.h>
#include <iostream>
#include <time.h>
#include "struct.h"  // 由 gson 工具根据 struct.stub 转换而成
#include "struct.gson.h"    // 由 gson 工具根据 struct.stub 生成

static void parse(const char* s)
{
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

static void parse2(const char* s)
{
	acl::json json;
	json.update(s);

	if (json.finish() && json.to_string() == s) {
		printf("parse |%s||%s| ok\r\n", s, json.to_string().c_str());
	} else {
		printf("parse |%s| error\r\n", s);
	}
}

static void test(void)
{
	const char* s1 = "{\"files\":['1,;,;i,;]}[]]}}}}    ' ], \"values\": []}";
	const char* s2 = "{\"files\":[\";   ;  \"], \"values\":[]}";
	const char* s3 = "{\"files\":[\"   ;  \"], \"values\":[]}";
	const char* s4 = "{'files':[], 'values':[1, 2, 3, 4]}";
	const char* s5 = "{'files':[], 'values':[]}";
	const char* s6 = "{'files':[   xxxx,   yyyyy, zzzz], 'values':[]}";

	parse(s1);
	printf("--------------------------------------------------------\r\n");
	parse(s2);
	printf("--------------------------------------------------------\r\n");
	parse(s3);
	printf("--------------------------------------------------------\r\n");
	parse(s4);
	printf("--------------------------------------------------------\r\n");
	parse(s5);
	printf("--------------------------------------------------------\r\n");
	parse(s6);
	printf("--------------------------------------------------------\r\n");

	const char* ss1 = "[]";
	parse2(ss1);
	printf("--------------------------------------------------------\r\n");
	const char* ss2 = "{}";
	parse2(ss2);
	printf("--------------------------------------------------------\r\n");
	const char* ss3 = "[{}]";
	parse2(ss3);
	printf("--------------------------------------------------------\r\n");
	const char* ss4 = "{[]}";
	parse2(ss4);
	printf("--------------------------------------------------------\r\n");
	const char* ss5 = "{[{[{}]}]}";
	parse2(ss5);
	printf("--------------------------------------------------------\r\n");
	const char* ss6 = "{{{{{}}}}}";
	parse2(ss6);
	printf("--------------------------------------------------------\r\n");
	const char* ss7 = "{[{[{},[],{},[]]}]}";
	parse2(ss7);
	printf("--------------------------------------------------------\r\n");
}

int main(void)
{
	test();
	return 0;
}
