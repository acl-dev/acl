#include "stdafx.h"

//////////////////////////////////////////////////////////////////////////

class mythread : public acl::thread
{
public:
	mythread() {}
	~mythread() {}

protected:
	virtual void* run()
	{
		const char* myname = "run";
		printf("%s: thread id: %lu, %lu\r\n",
			myname, thread_id(), acl::thread::thread_self());
		return NULL;
	}

private:

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

	if (thr.wait(NULL) == false)
		printf("wait thread failed\r\n");
	else
		printf("wait thread ok\r\n");
}

int main(void)
{
	// ≥ı ºªØ acl ø‚
	acl::acl_cpp_init();

	test_thread();

#ifdef WIN32
	printf("enter any key to exit ...\r\n");
	getchar();
#endif
	return 0;
}
