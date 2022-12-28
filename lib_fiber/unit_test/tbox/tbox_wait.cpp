#include "stdafx.h"
#include "test_tbox.h"

int tbox_thread_wait(AUT_LINE *test_line, void *arg acl_unused)
{
	int delay, count, min_diff;

	AUT_INT(test_line, "delay", delay, 200);
	AUT_INT(test_line, "count", count, 10);
	AUT_INT(test_line, "min_diff", min_diff, 5);

	acl::fiber_tbox<bool> tbox;

	struct timeval begin, end;

	for (int i = 0; i < count; i++) {
		gettimeofday(&begin, NULL);
		tbox.pop(delay);
		gettimeofday(&end, NULL);

		int cost = (int) acl::stamp_sub(end, begin);
		printf("thread-%lu: tbox wait cost=%d ms\r\n",
			acl::thread::self(), cost);

		if (abs(cost - delay) >= min_diff) {
			printf("Diff too large, diff=%d, cost=%d, delay=%d\r\n",
				abs(cost - delay), cost, delay);
			return -1;
		}
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////////

class tbox_waiter : public acl::fiber {
public:
	tbox_waiter(acl::fiber_tbox<bool>& tbox, int ndelay,
		int count, int min_diff)
	: ok_(true)
	, tbox_(tbox)
	, delay_(ndelay)
	, count_(count)
	, min_diff_(min_diff)
	{}

	~tbox_waiter(void) {}

	bool sucessful(void) const {
		return ok_;
	}

private:
	bool ok_;
	acl::fiber_tbox<bool>& tbox_;
	int delay_;
	int count_;
	int min_diff_;

	// @override
	void run(void) {
		struct timeval begin, end;

		for (int i = 0; i < count_; i++) {
			gettimeofday(&begin, NULL);
			tbox_.pop(delay_);
			gettimeofday(&end, NULL);

			int cost = (int) acl::stamp_sub(end, begin);
			if ((int) abs(cost - delay_) < min_diff_) {
				printf("fiber-%d: test ok\r\n", acl::fiber::self());
				continue;
			}

			printf("Diff too large, diff=%d, cost=%d, delay=%d\r\n",
				abs(cost - delay_), cost, delay_);
			ok_ = false;
			break;
		}
	}
};

int tbox_fiber_wait(AUT_LINE *test_line, void *arg acl_unused)
{
	int delay, count, min_diff, nfiber;

	AUT_INT(test_line, "delay", delay, 200);
	AUT_INT(test_line, "count", count, 10);
	AUT_INT(test_line, "min_diff", min_diff, 5);
	AUT_INT(test_line, "nfiber", nfiber, 1);

	acl::fiber_tbox<bool> tbox;
	std::vector<tbox_waiter*> fibers;

	for (int i = 0; i < nfiber; i++) {
		tbox_waiter* waiter = new tbox_waiter(tbox, delay, count, min_diff);
		waiter->start();
		fibers.push_back(waiter);
	}

	acl::fiber::schedule();

	for (std::vector<tbox_waiter*>::iterator it = fibers.begin();
		it != fibers.end(); ++it) {
		if (!(*it)->sucessful()) {
			printf("test failed\r\n");
			return -1;
		}
		delete *it;
	}

	return 0;
}
