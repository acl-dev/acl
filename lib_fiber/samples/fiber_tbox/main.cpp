#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <vector>

static int __nloop = 2;
static int __delay = 0;

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

//////////////////////////////////////////////////////////////////////////////

class fiber_producer : public acl::fiber
{
public:
	fiber_producer(acl::fiber_tbox<myobj>& tbox) : tbox_(tbox) {}

private:
	~fiber_producer(void) {}

private:
	acl::fiber_tbox<myobj>& tbox_;

	void run(void)
	{
		for (int i = 0; i < __nloop; i++) {
			myobj* o = new myobj;
			__producing++;
			tbox_.push(o);
		}
		delete this;
	}
};

class producer : public acl::thread
{
public:
	producer(acl::fiber_tbox<myobj>& tbox, int nfibers)
	: tbox_(tbox)
	, nfibers_(nfibers)
	{
		this->set_detachable(false);
	}

private:
	~producer(void) {}

private:
	acl::fiber_tbox<myobj>& tbox_;
	int nfibers_;

	// @override
	void* run(void)
	{
		for (int i = 0; i < nfibers_; i++) {
			acl::fiber* fb = new fiber_producer(tbox_);
			fb->start();
		}

		acl::fiber::schedule();
		return NULL;
	}
};

//////////////////////////////////////////////////////////////////////////////

class fiber_consumer : public acl::fiber
{
public:
	fiber_consumer(acl::fiber_tbox<myobj>& tbox, int timeout)
	: tbox_(tbox)
	, timeout_(timeout)
	{
	}

private:
	~fiber_consumer(void) {}

	acl::fiber_tbox<myobj>& tbox_;
	int timeout_;

	void run(void)
	{
		while (true) {
			myobj* o = tbox_.pop(timeout_);
			if (!o) {
				continue;
			}

			delete o;

			if (++__consuming % 100000 != 0) {
				continue;
			}

			char buf[256];
			snprintf(buf, sizeof(buf), "%lld", __consuming.value());
			acl::meter_time(__FILE__, __LINE__, buf);
		}

		delete this;
	}
};

class consumer : public acl::thread
{
public:
	consumer(acl::fiber_tbox<myobj>& tbox, int timeout, int nfibers)
	: tbox_(tbox)
	, timeout_(timeout)
	, nfibers_(nfibers)
	{
		this->set_detachable(false);
	}

private:
	~consumer(void) {}

private:
	acl::fiber_tbox<myobj>& tbox_;
	int timeout_;
	int nfibers_;

	void* run(void)
	{
		for (int i = 0; i < nfibers_; i++) {
			acl::fiber* fb = new fiber_consumer(tbox_, timeout_);
			fb->start();
		}

		acl::fiber::schedule();
		return NULL;
	}
};

//////////////////////////////////////////////////////////////////////////////

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		" -p producer_threads[default: 1]\r\n"
		" -c consumer_threads[default: 1]\r\n"
		" -P producer_fibers_per_thread[default: 1]\r\n"
		" -C consumer_fibers_per_thread[default: 1]\r\n"
		" -n nloop[default: 2]\r\n"
		" -t timeout[default: -1 ms]\r\n"
		" -d delay[default: 0 ms]\r\n"
		, procname);
}

int main(int argc, char *argv[])
{
	int  ch, producer_threads = 1, consumer_threads = 1, timeout = -1;
	int  producer_fibers = 1, consumer_fibers = 1;

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "hp:c:P:C:n:d:t:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'p':
			producer_threads = atoi(optarg);
			break;
		case 'c':
			consumer_threads = atoi(optarg);
			break;
		case 'P':
			producer_fibers = atoi(optarg);
			break;
		case 'C':
			consumer_fibers = atoi(optarg);
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
	for (int i = 0; i < consumer_threads; i++) {
		acl::thread* thr = new consumer(tbox, timeout, consumer_fibers);
		threads.push_back(thr);
		thr->start();
	}

	for (int i = 0; i < producer_threads; i++) {
		acl::thread* thr = new producer(tbox, producer_fibers);
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
