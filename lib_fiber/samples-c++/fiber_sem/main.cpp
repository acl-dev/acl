#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>

class myfiber : public acl::fiber {
public:
	myfiber(acl::fiber_sem* sem) : sem_(sem) {}

private:
	~myfiber() {}

protected:
	// @override
	void run() {
		for (int i = 0; i < 2; i++) {
			printf("fiber-%d: begin wait\r\n", get_id());
			int left = sem_->wait();
			printf("fiber-%d: left=%d, begin yield\r\n", get_id(), left);
			acl::fiber::yield();
			printf("fiber-%d: begin post\r\n", get_id());
			sem_->post();
			printf("fiber-%d: post ok\r\n", get_id());
		}
		delete this;
	}

private:
	acl::fiber_sem* sem_;
};

class fiber_waiter : public acl::fiber {
public:
	fiber_waiter(acl::fiber_sem* sem, int timeout) : sem_(sem), timeout_(timeout) {}

private:
	acl::fiber_sem* sem_;
	int timeout_;
	~fiber_waiter() {}

	// @override
	void run() {
		int ret = sem_->wait(timeout_ * 1000);
		if (ret >= 0) {
			printf("fiber_waiter wait sem ok, ret=%d\r\n", ret);
		} else {
			printf("fiber_waiter wait error=%s\r\n", acl::last_serror());
		}

		delete this;
	}
};

static void usage(const char* procname) {
	printf("usage: %s -h [help] -n fibers_count -S -A [if using sem async]\r\n", procname);
}

int main(int argc, char *argv[]) {
	int  ch, n = 5, timeout = 5;
	bool share_stack = false, sem_async = false;

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "hn:SAw:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			n = atoi(optarg);
			break;
		case 'S':
			share_stack = true;
			break;
		case 'A':
			sem_async = true;
			break;
		case 'w':
			timeout = atoi(optarg);
			break;
		default:
			break;
		}
	}

	acl::fiber_sem* sem;
	if (sem_async) {
		sem = new acl::fiber_sem(1, acl::fiber_sem_t_async);
	} else {
		sem = new acl::fiber_sem(1);
	}

	for (int i = 0; i < n; i++) {
		acl::fiber* f = new myfiber(sem);
		f->start(share_stack ? 8000 : 32000, share_stack);
	}

	acl::fiber* f = new fiber_waiter(sem, timeout);
	f->start();

	acl::fiber::schedule();
	delete sem;
	return 0;
}
