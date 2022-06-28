#include "stdafx.h"

//////////////////////////////////////////////////////////////////////////

class mythread : public acl::thread
{
public:
	mythread(bool auto_destroy = false)
		: auto_destroy_(auto_destroy)
	{}
	~mythread() {}

protected:
	virtual void* run()
	{
		const char* myname = "run";
		printf("%s: thread id: %lu, %lu\r\n",
			myname, thread_id(), acl::thread::thread_self());
		if (auto_destroy_)
			delete this;
		return NULL;
	}

private:
	bool auto_destroy_;
};

//////////////////////////////////////////////////////////////////////////

static void test_thread(void)
{
	const char* myname = "test_thread";
	mythread thr;

	thr.set_detachable(false);
	if (thr.start() == false)
	{
		printf("start thread failed\r\n");
		return;
	}

	printf("%s: thread id is %lu, main thread id: %lu\r\n",
		myname, thr.thread_id(), acl::thread::thread_self());

	printf("begin wait for thread exit\r\n");
	if (thr.wait(NULL) == false)
		printf("wait thread failed\r\n");
	else
		printf("wait thread ok\r\n");
}

static void test_thread2(void)
{
	mythread* thr = new mythread(true);

	thr->set_detachable(true);
	if (thr->start() == false)
		printf("start thread failed\r\n");
}

class mythread3 : public acl::thread
{
public:
	mythread3(acl::thread_cond& cond) : cond_(cond) {}
	~mythread3(void) {}

protected:
	void* run(void)
	{
		int i = 0;
		while (i++ < 5)
		{
			printf("sleep one second\r\n");
			sleep(1);
		}

		cond_.notify();
		return NULL;
	}

private:
	acl::thread_cond& cond_;
};

static void test_thread3(void)
{
	acl::thread_cond cond;
	mythread3 thread(cond);
	thread.start(true);
	printf("wait %s\r\n", cond.wait() ? "ok" : "error");
}

int main(void)
{
	// ³õÊ¼»¯ acl ¿â
	acl::acl_cpp_init();

	test_thread();
	if (0)
	test_thread2();

	printf("enter any key to exit ...\r\n");
	getchar();

	if (0)
	test_thread3();

	return 0;
}
