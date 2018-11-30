#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>

struct A {
	char buf[256];
};

class fiber_consumer : public acl::fiber
{
public:
	fiber_consumer(int n,
		acl::channel<int>& chan1,
		acl::channel<A>& chan2,
		acl::channel<acl::string*>& chan3)
		: count_(n), chan1_(chan1), chan2_(chan2), chan3_(chan3) {}
	~fiber_consumer(void) {}

protected:
	// @override
	void run(void)
	{
		for (int i = 0; i < count_; i++)
		{
			int n;
			chan1_.pop(n);

			A a;
			chan2_.pop(a);

			acl::string* s;
			chan3_.pop(s);

			if (i < 1000)
				printf(">>read: n = %d, a = %s, s = %s\r\n",
					n, a.buf, s->c_str());
			delete s;
		}
	}

private:
	int count_;
	acl::channel<int>& chan1_;
	acl::channel<A>& chan2_;
	acl::channel<acl::string*>& chan3_;
};

class fiber_producer : public acl::fiber
{
public:
	fiber_producer(int n,
		acl::channel<int>& chan1,
		acl::channel<A>& chan2,
		acl::channel<acl::string*>& chan3)
		: count_(n), chan1_(chan1), chan2_(chan2), chan3_(chan3) {}
	~fiber_producer(void) {}

protected:
	// @override
	void run(void)
	{
		for (int i = 0; i < count_; i++)
		{
			if (i < 1000)
				printf(">>send: %d\r\n", i);

			chan1_ << i;

			A a;
			snprintf(a.buf, sizeof(a.buf), "A-%d", i);
			chan2_ << a;

			acl::string* s = new acl::string;
			s->format("string-%d", i);
			chan3_ << s;
		}
	}

private:
	int count_;
	acl::channel<int>& chan1_;
	acl::channel<A>& chan2_;
	acl::channel<acl::string*>& chan3_;
};

static void usage(const char* procname)
{
	printf("usage: %s -h [help] -n count\r\n", procname);
}

int main(int argc, char *argv[])
{
	int  ch, n = 10;

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "hn:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			n = atoi(optarg);
			break;
		default:
			break;
		}
	}

	acl::channel<int> chan1;
	acl::channel<A> chan2;
	acl::channel<acl::string*> chan3;

	fiber_consumer consumer(n, chan1, chan2, chan3);
	consumer.start();

	fiber_producer producer(n, chan1, chan2, chan3);
	producer.start();

	acl::fiber::schedule();

	return 0;
}
