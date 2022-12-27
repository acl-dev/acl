#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <vector>

static int __total_count = 2;
static int __diff_delay = 10000;
static int __delay = 10;
static acl::fiber_event_t __event_type = acl::FIBER_EVENT_T_KERNEL;
static acl::atomic_long __producing = 0;
static acl::atomic_long __consuming = 0;
static acl::atomic_long __timedout  = 0;

class myobj
{
public:
	myobj(bool stop = false) : stop_(stop) {}
	~myobj(void) {}

	void test(void)
	{
		printf("thread-%lu: hello world!\r\n", acl::thread::self());
	}

	bool stop_;
};

//////////////////////////////////////////////////////////////////////////////

static void push_one(acl::fiber_tbox<myobj>& tbox)
{
	myobj* o = new myobj;
	tbox.push(o);

	long long p = __producing++;
	long long c = __consuming;

	if (__diff_delay > 0 && p - c >= __diff_delay) {
		printf("diff=%lld sleep %d ms\r\n", p - c, __delay);
		acl::fiber::delay(100);
	}
}

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
		while (true) {
			if (__producing.value() >= __total_count) {
				break;
			}
			push_one(tbox_);
		}

		printf("thread-%lu, fiber-%d, push over\n",
			acl::thread::self(), acl::fiber::self());

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

		//usleep(100000);
		acl::fiber::schedule_with(__event_type);
		printf("producer fiber thread-%lu exit!\r\n", acl::thread::self());
		return NULL;
	}
};

class thread_producer : public acl::thread
{
public:
	thread_producer(acl::fiber_tbox<myobj>& tbox) : tbox_(tbox)
	{
		this->set_detachable(false);
	}

private:
	~thread_producer(void) {}

private:
	acl::fiber_tbox<myobj>& tbox_;

	void* run(void)
	{
		while (1) {
			if (__producing.value() >= __total_count) {
				break;
			}
			push_one(tbox_);
		}

		printf("producer thread-%lu exit!\r\n", acl::thread::self());
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

			if (__consuming < 5) {
				o->test();
			}

			if (o->stop_) {
				delete o;
				break;
			}

			delete o;

			if (++__consuming % 100000 != 0) {
				continue;
			}

			char buf[256];
			snprintf(buf, sizeof(buf), "%lld", __consuming.value());
			acl::meter_time(__FILE__, __LINE__, buf);
		}

		printf("thread-%lu, fiber-%d consumer over now!\r\n",
			acl::thread::self(), acl::fiber::self());

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

		acl::fiber::schedule_with(__event_type);

		printf("consumer fiber thread-%lu exit, consume=%lld\r\n",
			acl::thread::self(), __consuming.value());
		return NULL;
	}
};

class thread_consumer : public acl::thread
{
public:
	thread_consumer(acl::fiber_tbox<myobj>& tbox, int timeout)
	: tbox_(tbox)
	, timeout_(timeout)
	{
		this->set_detachable(false);
	}

private:
	~thread_consumer(void) {}

private:
	acl::fiber_tbox<myobj>& tbox_;
	int timeout_;

	void* run(void)
	{
		while (true) {
			myobj* o = tbox_.pop(timeout_);

			if (!o) {
				continue;
			}

			if (++__consuming < 5) {
				o->test();
			}

			if (o->stop_) {
				delete o;
				break;
			}

			delete o;
		}

		printf("consumer thread-%lu exit, consume=%lld!\r\n",
			acl::thread::self(), __consuming.value());
		return NULL;
	}
};

//////////////////////////////////////////////////////////////////////////////

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		" -e event_type[kernel|io_uring|poll|select]\r\n"
		" -p producer_threads[default: 1]\r\n"
		" -c consumer_threads[default: 1]\r\n"
		" -P producer_fibers_per_thread[default: 1]\r\n"
		" -s alone_thread_producer_count[default: 0]\r\n"
		" -r alone_thread_consumer_count[default: 0]\r\n"
		" -C consumer_fibers_per_thread[default: 1]\r\n"
		" -n nloop[default: 2]\r\n"
		" -t timeout[default: -1 ms]\r\n"
		" -d delay[default: 10 ms]\r\n"
		" -k diff_delay[default: 10000]\r\n"
		, procname);
}

int main(int argc, char *argv[])
{
	int  ch, producer_threads = 1, consumer_threads = 1, timeout = -1;
	int  producer_fibers = 1, consumer_fibers = 1;
	int  alone_thread_consumer_count = 0, alone_thread_producer_count = 0;

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

#define	EQ	!strcasecmp

	while ((ch = getopt(argc, argv, "he:p:c:P:C:n:d:t:k:s:r:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'e':
			if (EQ(optarg, "io_uring")) {
				__event_type = acl::FIBER_EVENT_T_IO_URING;
			} else if (EQ(optarg, "poll")) {
				__event_type = acl::FIBER_EVENT_T_POLL;
			} else if (EQ(optarg, "select")) {
				__event_type = acl::FIBER_EVENT_T_SELECT;
			} else if (EQ(optarg, "kernel")) {
				__event_type = acl::FIBER_EVENT_T_KERNEL;
			}
			break;
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
		case 's':
			alone_thread_producer_count = atoi(optarg);
			break;
		case 'r':
			alone_thread_consumer_count = atoi(optarg);
			break;
		case 'n':
			__total_count = atoi(optarg);
			break;
		case 'd':
			__delay = atoi(optarg);
			break;
		case 'k':
			__diff_delay = atoi(optarg);
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

	int nconsumers = 0;

	std::vector<acl::thread*> producers, consumers;

	for (int i = 0; i < consumer_threads; i++) {
		acl::thread* thr = new consumer(tbox, timeout, consumer_fibers);
		consumers.push_back(thr);
		thr->start();
	}

	nconsumers += consumer_threads * consumer_fibers;

	for (int i = 0; i < producer_threads; i++) {
		acl::thread* thr = new producer(tbox, producer_fibers);
		producers.push_back(thr);
		thr->start();
	}

	for (int i = 0; i < alone_thread_consumer_count; i++) {
		acl::thread* thr = new thread_consumer(tbox, timeout);
		consumers.push_back(thr);
		thr->start();
		nconsumers++;
	}

	for (int i = 0; i < alone_thread_producer_count; i++) {
		acl::thread* thr = new thread_producer(tbox);
		producers.push_back(thr);
		thr->start();
	}

	for (std::vector<acl::thread*>::iterator it = producers.begin();
		it != producers.end(); ++it) {

		(*it)->wait();
		delete *it;
	}

	for (int i = 0; i < nconsumers; i++) {
		myobj* o = new myobj(true);
		tbox.push(o);
	}

	for (std::vector<acl::thread*>::iterator it = consumers.begin();
		it != consumers.end(); ++it) {

		(*it)->wait();
		delete *it;
	}

	printf("all over, nloop=%d, producing=%lld, consuming=%lld, timedout=%lld\r\n",
		__total_count, __producing.value(), __consuming.value(), __timedout.value());

	return 0;
}
