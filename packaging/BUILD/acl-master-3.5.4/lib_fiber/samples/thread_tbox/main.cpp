#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <vector>

static int __nloop    = 2;
static int __delay    = 0;

static acl::atomic_long __producing = 0;
static acl::atomic_long __consuming = 0;
static acl::atomic_long __timedout  = 0;

class myobj
{
public:
	myobj(void) {}
	~myobj(void) {}

	void test(void)
	{
		printf("hello world!\r\n");
	}
};

class producer : public acl::thread
{
public:
	producer(acl::fiber_tbox<myobj>& tbox)
	: tbox_(tbox)
	{
		this->set_detachable(false);
	}

private:
	~producer(void) {}

private:
	acl::fiber_tbox<myobj>& tbox_;

	// @override
	void* run(void)
	{
		for (int i = 0; i < __nloop; i++) {
			if (__delay > 0) {
				acl_doze(__delay);
			}
			myobj* o = new myobj;
			tbox_.push(o);
			__producing++;
		}

		printf("producer over=%lld\r\n", __producing.value());
		return NULL;
	}
};

class consumer : public acl::thread
{
public:
	consumer(acl::fiber_tbox<myobj>& tbox, int timeout)
	: tbox_(tbox)
	, timeout_(timeout)
	{
		this->set_detachable(false);
	}

private:
	~consumer(void) {}

private:
	acl::fiber_tbox<myobj>& tbox_;
	int timeout_;

	void* run(void)
	{
		int n = 0;
		for (int i = 0; i < __nloop; i++) {
			myobj* o = tbox_.pop(timeout_);
			if (o && i <= 10) {
				o->test();
			}
			if (o) {
				__consuming++;
				n++;
			} else {
				__timedout++;
				printf("pop timeout\r\n");
			}

			delete o;
		}

		printf("consumer-%lu over=%lld, n=%d\r\n",
			acl::thread::self(), __consuming.value(), n);
		return NULL;
	}
};

//////////////////////////////////////////////////////////////////////////////

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		" -c threads[default: 1]\r\n"
		" -n nloop[default: 2]\r\n"
		" -t timeout[default: -1 ms]\r\n"
		" -d delay[default: 0 ms]\r\n"
		, procname);
}

int main(int argc, char *argv[])
{
	int  ch, nthreads = 1, timeout = -1;

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "hc:n:d:t:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'c':
			nthreads = atoi(optarg);
			break;
		case 'n':
			__nloop = atoi(optarg);
			break;
		case 'd':
			__delay = atoi(optarg);
			break;
		case 't':
			timeout = atoi(optarg);
			break;
		default:
			break;
		}
	}

	acl::log::stdout_open(true);
	acl::fiber::stdout_open(true);
	acl::fiber_tbox<myobj> tbox;

	std::vector<acl::thread*> threads;
	for (int i = 0; i < nthreads; i++) {
		acl::thread* thr = new consumer(tbox, timeout);
		threads.push_back(thr);
		thr->start();
	}

	for (int i = 0; i < nthreads; i++) {
		acl::thread* thr = new producer(tbox);
		threads.push_back(thr);
		thr->start();
	}

	for (std::vector<acl::thread*>::iterator it = threads.begin();
		it != threads.end(); ++it) {

		(*it)->wait();
		delete *it;
	}

	printf("all over, nloop=%d, producing=%lld, consuming=%lld, timedout=%lld\r\n",
		__nloop, __producing.value(), __consuming.value(), __timedout.value());

	return 0;
}
