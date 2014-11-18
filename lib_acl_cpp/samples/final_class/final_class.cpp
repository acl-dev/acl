// final_class.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <stdio.h>
#include "acl_cpp/stdlib/final_tpl.hpp"
//#include "lib_acl.hpp"

class CMyFinalClass : public acl::final_tpl<CMyFinalClass>
{
public:
	CMyFinalClass(int n) : dummy_(n) {}
	~CMyFinalClass() {}

	void Test()
	{
		printf("hello, I'm the final class\n");
	}
protected:
private:
	int dummy_;
};

class CMyClass  //: CMyFinalClass
{
public:
	CMyClass() {}
	~CMyClass() {}
};

int main(void)
{
	CMyFinalClass m(1);
	m.Test();

	printf(">>enter any key to exit ...\n");
	getchar();
	return 0;
}

