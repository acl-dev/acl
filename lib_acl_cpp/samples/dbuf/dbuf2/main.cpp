#include "stdafx.h"
#if !defined(_WIN32) && !defined(_WIN64)
#include <sys/time.h>
#endif

/**
 * dbuf_obj 子类，在 dbuf_pool 上动态分配，由 dbuf_guard 统一进行管理
 */
class myobj : public acl::dbuf_obj
{
public:
	myobj(acl::dbuf_guard* guard = NULL) : dbuf_obj(guard)
	{
		ptr_ = strdup("hello");
	}

	void run()
	{
		printf("----> run->hello world <-----\r\n");
	}

private:
	char* ptr_;

	// 将析构声明为私人，以强制要求该对象被动态分配，该析构函数将由
	// dbuf_guard 统一调用，以释放本类对象中产生的动态内存(ptr_)
	~myobj()
	{
		free(ptr_);
	}
};

static void test_dbuf(acl::dbuf_guard& dbuf)
{
	for (int i = 0; i < 102400; i++)
	{
		// 动态分配内存
		char* ptr = (char*) dbuf.dbuf_alloc(10);
		(void) ptr;
	}

	for (int i = 0; i < 102400; i++)
	{
		// 动态分配字符串
		char* str = dbuf.dbuf_strdup("hello world");
		if (i < 5)
			printf(">>str->%s\r\n", str);
	}

	// 动态分配内存

	(void) dbuf.dbuf_alloc(1024);
	(void) dbuf.dbuf_alloc(1024);
	(void) dbuf.dbuf_alloc(2048);
	(void) dbuf.dbuf_alloc(1024);
	(void) dbuf.dbuf_alloc(1024);
	(void) dbuf.dbuf_alloc(1024);
	(void) dbuf.dbuf_alloc(1024);
	(void) dbuf.dbuf_alloc(1024);
	(void) dbuf.dbuf_alloc(1024);
	(void) dbuf.dbuf_alloc(1024);
	(void) dbuf.dbuf_alloc(1024);
	(void) dbuf.dbuf_alloc(10240);

	for (int i = 0; i < 10000; i++)
	{
		// 动态分配 dbuf_obj 子类对象，并通过将 dbuf_guard 对象传入
		// dbuf_obj 的构造函数，从而将之由 dbuf_guard 统一管理，

		myobj* obj = dbuf.create<myobj>(&dbuf);

		// 验证 dbuf_obj 对象在 dbuf_guard 中的在在一致性
		assert(obj == dbuf[obj->pos()]);

		// 调用 dbuf_obj 子类对象 myobj 的函数 run
		if (i < 10)
			obj->run();
	}

	for (int i = 0; i < 10000; i++)
	{
		myobj* obj = dbuf.create<myobj>();

		assert(dbuf[obj->pos()] == obj);

		if (i < 10)
			obj->run();
	}

	for (int i = 0; i < 10000; i++)
	{
		myobj* obj = dbuf.create<myobj>(&dbuf);

		// 虽然多次将 dbuf_obj 对象置入 dbuf_guard 中，因为 dbuf_obj
		// 内部的引用计数，所以可以防止被重复添加
		(void) dbuf.push_back(obj);
		(void) dbuf.push_back(obj);
		(void) dbuf.push_back(obj);

		assert(obj == dbuf[obj->pos()]);

		if (i < 10)
			obj->run();
	}
}

static void wait_pause()
{
	printf("Enter any key to continue ...");
	fflush(stdout);
	getchar();
}

static void test1()
{
	// dbuf_gaurd 对象创建在栈上，函数返回前该对象自动销毁
	acl::dbuf_guard dbuf;

	test_dbuf(dbuf);
}

static void test2()
{
	// 动态创建 dbuf_guard 对象，需要手动销毁该对象
	acl::dbuf_guard* dbuf = new acl::dbuf_guard;

	test_dbuf(*dbuf);

	// 手工销毁该对象
	delete dbuf;
}

static void test3()
{
	// 将内存池对象 dbuf_pool 做为 dbuf_guard 构造函数参数传入，当
	// dbuf_guard 对象销毁时，dbuf_pool 对象一同被销毁
	acl::dbuf_guard dbuf(new acl::dbuf_pool);

	test_dbuf(dbuf);
}

static void test4()
{
	// 动态创建 dbuf_guard 对象，同时指定内存池中内存块的分配倍数为 10，
	// 即指定内部每个内存块大小为 4096 * 10 = 40 KB，同时
	// 指定内部动态数组的初始容量大小
	acl::dbuf_guard dbuf(10, 100);

	test_dbuf(dbuf);
}

static void test5()
{
	acl::dbuf_pool* dp = new acl::dbuf_pool;

	// 在内存池对象上动态创建 dbuf_guard 对象，这样可以将内存分配的次数
	// 进一步减少一次
	acl::dbuf_guard* dbuf = new (dp->dbuf_alloc(sizeof(acl::dbuf_guard)))
		acl::dbuf_guard(dp);

	test_dbuf(*dbuf);

	// 因为 dbuf_gaurd 对象也是在 dbuf_pool 内存池对象上动态创建的，所以
	// 只能通过直接调用 dbuf_guard 的析构函数来释放所有的内存对象；
	// 既不能直接 dbuf_pool->desotry()，也不能直接 delete dbuf_guard 来
	// 销毁 dbuf_guard 对象
	dbuf->~dbuf_guard();
}

class myobj2 : public acl::dbuf_obj
{
public:
	myobj2() {}

	void run()
	{
		printf("hello world\r\n");
	}

private:
	~myobj2() {}
};

class myobj3 : public acl::dbuf_obj
{
public:
	myobj3(int i) : i_(i) {}

	void run()
	{
		printf("hello world: %d\r\n", i_);
	}

private:
	~myobj3() {}

private:
	int i_;
};

class myobj_dummy // : public acl::dbuf_obj
{
public:
	myobj_dummy() {}

	void run()
	{
		printf("can't be compiled\r\n");
	}

private:
	~myobj_dummy() {}
};

static void test6()
{
	acl::dbuf_guard dbuf;

	myobj* o = dbuf.create<myobj>();
	o->run();

	myobj* o1 = dbuf.create<myobj>(&dbuf);
	o1->run();

	myobj2* o2 = dbuf.create<myobj2>();
	o2->run();

	myobj3* o3 = dbuf.create<myobj3>(10);
	o3->run();

	for (int i = 0; i < 10; i++)
	{
		myobj3* o4 = dbuf.create<myobj3>(i);
		o4->run();
	}

	// below codes can't be compiled, because myobj_dummy isn't
	// acl::dbuf_obj's subclass
	// myobj_dummy* dummy = dbuf.create<myobj_dummy>();
	// dummy->run();
}

int main(void)
{
	acl::log::stdout_open(true);

	test1();
	wait_pause();

	test2();
	wait_pause();

	test3();
	wait_pause();

	test4();
	wait_pause();

	test5();
	wait_pause();

	test6();
	return 0;
}
