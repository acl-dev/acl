// xml.cpp : 定义控制台应用程序的入口点。
//
#include "stdafx.h"
#include <sys/time.h>

class myobj : public acl::dbuf_obj
{
public:
	myobj(acl::dbuf_guard* guard) : dbuf_obj(guard)
	{
		ptr_ = strdup("hello");
	}

	void run()
	{
		printf("----> hello world <-----\r\n");
	}

private:
	char* ptr_;

	~myobj()
	{
		free(ptr_);
		printf("----> myobj destroied <-----\r\n");
	}
};

int main(void)
{
	acl::log::stdout_open(true);

	acl::dbuf_guard dbuf;

	for (int i = 0; i < 102400; i++)
		dbuf.dbuf_alloc(10);

	dbuf.dbuf_alloc(1024);
	dbuf.dbuf_alloc(1024);
	dbuf.dbuf_alloc(2048);
	dbuf.dbuf_alloc(1024);
	dbuf.dbuf_alloc(1024);
	dbuf.dbuf_alloc(1024);
	dbuf.dbuf_alloc(1024);
	dbuf.dbuf_alloc(1024);
	dbuf.dbuf_alloc(1024);
	dbuf.dbuf_alloc(1024);
	dbuf.dbuf_alloc(1024);
	dbuf.dbuf_alloc(10240);

	for (int i = 0; i < 10; i++)
	{
		myobj* obj = new (dbuf.dbuf_alloc(sizeof(myobj))) myobj(&dbuf);
		obj->run();
	}

	return 0;
}
