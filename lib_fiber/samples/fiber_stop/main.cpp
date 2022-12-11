#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>

class wait_box {
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

class fiber_waiter : public acl::fiber {
public:
	fiber_waiter(void) {
		box_  = new wait_box;
		stop_ = false;
	}

	void stop(void) {
		stop_ = true;
		int fd = ss_.unbind_sock();
		if (fd >= 0) {
			::close(fd);
			printf(">>close fd=%d ok<<\r\n", fd);
		}
		box_->wait();
	}

private:
	~fiber_waiter(void) {}

private:
	wait_box* box_;
	bool stop_;
	acl::socket_stream ss_;

	// @override
	void run(void) {
		const char* addr = "127.0.0.1:8088";
		if (!ss_.bind_udp(addr)) {
			printf("bind %s error %s\r\n", addr, acl::last_serror());
			exit(1);
		}

		printf("bind %s ok\r\n", addr);

		while (!stop_) {
			char buf[1024];
			int ret = ss_.read(buf, sizeof(buf) - 1, false);
			if (ret == -1) {
				printf("fiber_waiter-> read error %s\r\n",
					acl::last_serror());
				break;
			}
			buf[ret] = 0;
			printf(">>>%s\r\n", buf);
		}

		box_->done();
		printf("fiber_waiter-> will exit!\r\n");
		delete this;
	}
};

class fiber_stop : public acl::fiber {
public:
	fiber_stop(fiber_waiter* waiter) : waiter_(waiter) {}

private:
	~fiber_stop(void) {}

private:
	fiber_waiter *waiter_;

	// @override
	void run(void) {
		printf("fiber_stop-> wait for one second\r\n");
		sleep(1);
		printf("fiber_stop-> call stopping...\r\n");
		waiter_->stop();
		printf("fiber_stop-> will exit!\r\n");
	}
};

class thread_runner : public acl::thread
{
public:
	thread_runner(acl::fiber_event_t event_type)
	: event_type_(event_type)
	{
	}

protected:
	~thread_runner(void) {}

	// @override
	void* run(void) {
		fiber_waiter* waiter = new fiber_waiter;
		waiter->start();

		fiber_stop* stop = new fiber_stop(waiter);
		stop->start();

		acl::fiber::schedule_with(event_type_);
		printf("fiber schedule stopped\r\n");
		delete this;
		return NULL;
	}

private:
	acl::fiber_event_t event_type_;
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

	acl::thread* runner = new thread_runner(event_type);
	runner->start();
	runner->wait();

	return 0;
}
