#include "stdafx.h"
#include "test_file.h"


int file_load(AUT_LINE *test_line, void *arg acl_unused)
{
	const char* filename;
	int show;

	AUT_STR(test_line, "filename", filename, "main.cpp");
	AUT_INT(test_line, "show", show, 0);

	acl::string buf1;
	if (acl::ifstream::load(filename, buf1) == false) {
		printf("Load %s error %s\r\n", filename, acl::last_serror());
		return -1;
	}

	acl::string buf2;

	go[filename, &buf2] {
		if (acl::ifstream::load(filename, buf2) == false) {
			printf("Go load %s error %s\r\n",
				filename, acl::last_serror());
		}
	};

	acl::fiber::schedule();

	if (buf2 == buf1) {
		printf("Load %s ok, file size=%zd, %zd!\r\n",
			filename, buf2.size(), buf1.size());
		if (show) {
			printf("%s\r\n", buf2.c_str());
		}
		return 0;
	} else {
		printf("Load %s error!\r\n", filename);
		return -1;
	}
}
