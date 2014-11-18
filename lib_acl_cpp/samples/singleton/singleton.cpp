// singleton.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "acl_cpp/stdlib/singleton.hpp"

// 跟踪调用过程的计数器
static int __nstep = 0;

class singleton_test : public acl::singleton <singleton_test>
{
public:
	singleton_test()
	{
		// 如果该句话打印先于 main 函数中的打印结果，则
		// 说明该单例是在 main 函数执行先被初始化的
		printf("step %d: singleton_test construct called\r\n", ++__nstep);
		fflush(stdout);
	}

	virtual ~singleton_test()
	{
		printf("step %d: singleton_test destruct called\r\n", ++__nstep);
		fflush(stdout);
		getchar();
	}

	const singleton_test& init() const
	{
		printf("step %d: singleton_test init called\r\n", ++__nstep);
		fflush(stdout);
		return *this;
	}

	const singleton_test& set(const char* name) const
	{
		printf("step %d: singleton_test set(%s) called\r\n", ++__nstep, name);
		fflush(stdout);
		return *this;
	}
};

//////////////////////////////////////////////////////////////////////////

class singleton_test2
{
public:
	singleton_test2()
	{
		// 如果该句话打印先于 main 函数中的打印结果，则
		// 说明该单例是在 main 函数执行先被初始化的
		printf("step %d: singleton_test2 construct called\r\n", ++__nstep);
		fflush(stdout);
	}

	virtual ~singleton_test2()
	{
		printf("step %d: singleton_test2 destruct called\r\n", ++__nstep);
		fflush(stdout);
		getchar();
	}

	const singleton_test2& init() const
	{
		printf("step %d: singleton_test2 init called\r\n", ++__nstep);
		fflush(stdout);
		return *this;
	}

	const singleton_test2& set(const char* name) const
	{
		printf("step %d: singleton_test2 set(%s) called\r\n", ++__nstep, name);
		fflush(stdout);
		return *this;
	}
};

//singleton_test::get_const_instance();
int main()
{
	printf("step %d: first line in main\r\n", ++__nstep);
	fflush(stdout);

	// 方法一
	const singleton_test& test1 = singleton_test::get_instance().init();
	const singleton_test& test2 = singleton_test::get_instance();
	test1.set("test1");
	test2.set("test2");

	// 方法二，用 VC2003 编译成 release 版本时，
	// 该方式可以保证单体实例在 main 之前被构造
	acl::singleton2<singleton_test2>::get_instance().init();
	acl::singleton2<singleton_test2>::get_instance().set("test1");
	acl::singleton2<singleton_test2>::get_instance().set("test2");
	return 0;
}
