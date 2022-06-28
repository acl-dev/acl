// url_coder.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "liburl.h"

int main(void)
{
	test_url_coder();
#ifdef WIN32
	printf("enter any key to exit ...\r\n");
	getchar();
#endif

	return 0;
}

