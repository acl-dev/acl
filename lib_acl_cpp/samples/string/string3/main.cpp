#include "lib_acl.h"
#include <vector>
#include <iostream>
#include "acl_cpp/lib_acl.hpp"
#include <stdio.h>
#include <string>

class mystring : public acl::string
{
public:
	mystring()
	{
	}

	~mystring()
	{
	}
};

static void test(void)
{
	for (int j = 0; j < 10000000; j++)
	{
//		const char* tmpstr = "this,is,a,test.";
		const char* tmpstr = ",,,,";
		acl::string aclstr(tmpstr); // = acl::string(tmpstr);
		std::vector<acl::string>& aclstr_vec = aclstr.split2(",");
		if (j >= 10)
			continue;
		int vecsize = (int) aclstr_vec.size();
		for (int i = 0; i< vecsize;++i)
			printf("%s\r\n", aclstr_vec[i].c_str());
	}

	printf("enter any key to continue...");
	fflush(stdout);
	getchar();
}

int main(void)
{
	if (0)
		for (int i = 0; i < 100; i++)
			test();

	acl::string b(20);
	size_t size = b.capacity();
	for (int i = (int) size - 1; i >= 0; i--)
	{
		b[i] = '1';
	}
	b[b.capacity()] = '\0';
	printf("cap: %d\n", (int) size);

	printf("---------------------------------------------------\n");

	size = b.capacity();
	printf("cap11: %d\n", (int) size);
	for (int i = 0; i < (int) size; i++)
		b[i] = '2';
	size = b.capacity();
	printf("cap12: %d\n", (int) size);

	printf("---------------------------------------------------\n");

	size = b.capacity();
	printf("cap20: %d\n", (int) size);

	b[b.capacity()] = '\0';
	size = b.capacity();
	printf("cap21: %d\n", (int) size);

	for (int i = 0; i < (int) size; i++)
	{
		b[i] = '2';
		printf("capxx22: %d\n", (int) b.capacity());
	}
	size = b.capacity();
	printf("cap22: %d\n", (int) size);

	printf("---------------------------------------------------\n");

	b[b.capacity()] = '\0';
	size = b.capacity();
	printf("cap31: %d\n", (int) size);

	for (int i = 0; i < (int) size; i++)
		b[i] = '3';
	size = b.capacity();
	printf("cap32: %d\n", (int) size);
	b[size + 1] = '\0';
	b[size + 2] = '\0';
	b[size + 3] = '\0';
	size = b.capacity();
	printf("cap33: %d\n", (int) size);

	printf("buf: %s, len: %d, cap: %d\r\n",
		b.c_str(), (int) b.length(), (int) b.capacity());

	printf("----------------------------\n");

	mystring a;

	a[4] = '5';
	a[3] = '4';
	a[2] = '3';
	a[1] = '2';
	a[0] = '1';
	a[5] = 0;
	printf(">>>str: %s, len: %d, %d\r\n",
		a.c_str(), (int) a.length(), (int) strlen(a.c_str()));
	printf("over now\r\n");

#ifdef WIN32
	printf("Enter any key to exit ...\r\n");
	getchar();
#endif // WIN32

	return 0;
}
