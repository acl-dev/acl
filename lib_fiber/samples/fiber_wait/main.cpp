#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>

class wait_box
{
public:
	wait_box(void) {
		refer_ = 2;
	}

	void done(void) {
		box_.push(NULL);
		if (--refer_ == 0) {
			delete this;
		}
	}

	void wait(void) {
		box_.pop();
		if (--refer_ == 0) {
			delete this;
		}
	}

private:
	acl::atomic_long refer_;
	acl::fiber_tbox<bool> box_;

	~wait_box(void) {}
};

class fiber_waiter : public acl::fiber
{
public:
	fiber_waiter(acl::fiber_tbox<bool>& box) : box_(box) {}
	~fiber_waiter(void) {}

private:
	acl::fiber_tbox<bool>& box_;

	// @override
	void run(void) {
		box_.pop();
		printf("fiber_waiter got one stop messasge\r\n");
		acl::fiber::schedule_stop();
		delete this;
	}
};

class thread_runner : public acl::thread
{
public:
	thread_runner(acl::fiber_event_t event_type)
	: event_type_(event_type)
	{
		wait_box_ = new wait_box;
	}

	void stop(void) {
		printf("send stop message to stop_box\r\n");
		stop_box_.push(NULL);
		printf("wait for runner finish\r\n");
		wait_box_->wait();
		printf("runner finished\r\n");
	}

protected:
	~thread_runner(void) {}

	// @override
	void* run(void) {
		acl::fiber* fb = new fiber_waiter(stop_box_);
		fb->start();

		acl::fiber::schedule_with(event_type_);
		printf("fiber schedule stopped\r\n");
		wait_box_->done();
		printf("runner will exit\r\n");

		delete this;
		return NULL;
	}

private:
	acl::fiber_event_t event_type_;
	acl::fiber_tbox<bool> stop_box_;
	wait_box* wait_box_;
};

class thread_stop : public acl::thread
{
public:
	thread_stop(thread_runner& runner) : runner_(runner) {}
	~thread_stop(void) {}

private:
	thread_runner& runner_;

	// @override
	void* run(void) {
		printf("thread_stop: sleep one second\r\n");
		sleep(1);
		printf("thread_stop: stop the runner thread\r\n");
		runner_.stop();
		printf("thread_stop: stop the runner ok\r\n");
		return NULL;
	}
};

//////////////////////////////////////////////////////////////////////////////

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		" -e event_type[kernel|poll|select|io_uring]\r\n"
		, procname);
}

int main(int argc, char *argv[])
{
	int  ch;
	acl::fiber_event_t event_type = acl::FIBER_EVENT_T_KERNEL;

	acl::acl_cpp_init();
	acl::log::stdout_open(true);
	acl::fiber::stdout_open(true);

	while ((ch = getopt(argc, argv, "he:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'e':
			if (strcasecmp(optarg, "kernel") == 0) {
				event_type = acl::FIBER_EVENT_T_KERNEL;
			} else if (strcasecmp(optarg, "select") == 0) {
				event_type = acl::FIBER_EVENT_T_SELECT;
			} else if (strcasecmp(optarg, "poll") == 0) {
				event_type = acl::FIBER_EVENT_T_POLL;
			} else if (strcasecmp(optarg, "io_uring") == 0) {
				event_type = acl::FIBER_EVENT_T_IO_URING;
			}
			break;
		default:
			break;
		}
	}

	thread_runner* runner = new thread_runner(event_type);
	runner->set_detachable(true);
	runner->start();

	acl::thread* thr = new thread_stop(*runner);
	thr->set_detachable(false);
	thr->start();
	thr->wait();
	delete thr;

	return 0;
}
