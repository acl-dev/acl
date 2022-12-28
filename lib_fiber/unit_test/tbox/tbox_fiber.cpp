#include "stdafx.h"
#include "test_tbox.h"

class test_obj {
public:
	test_obj(void) {}
	~test_obj(void) {}
};

class fiber_consumer : public acl::fiber {
public:
	fiber_consumer(acl::fiber_tbox<test_obj>& tbox, int count, int ndelay)
	: tbox_(tbox)
	, count_(count)
	, delay_(ndelay)
	, ncount_(0)
	{
	}

	~fiber_consumer(void) {}

	int get_count(void) const {
		return ncount_;
	}

private:
	acl::fiber_tbox<test_obj>& tbox_;
	int count_;
	int delay_;
	int ncount_;

	// @override
	void run(void) {
		for (int i = 0; i < count_; i++) {
			test_obj* o = tbox_.pop(delay_);
			if (o) {
				delete o;
				ncount_++;
			}
		}
	}
};

class fiber_producer : public acl::fiber {
public:
	fiber_producer(acl::fiber_tbox<test_obj>& tbox, int count)
	: tbox_(tbox)
	, count_(count)
	{
	}

	~fiber_producer(void) {}

private:
	acl::fiber_tbox<test_obj>& tbox_;
	int count_;

	// @override
	void run(void) {
		for (int i = 0; i < count_; i++) {
			test_obj* o = new test_obj;
			tbox_.push(o);
		}
	}
};

int tbox_fiber_consume(AUT_LINE *test_line, void *arg acl_unused)
{
	int nconsumers, nproducers, count, delay;

	AUT_INT(test_line, "consumers", nconsumers, 1);
	AUT_INT(test_line, "producers", nproducers, 1);
	AUT_INT(test_line, "count", count, 1);
	AUT_INT(test_line, "delay", delay, 100);

	acl::fiber_tbox<test_obj> tbox;
	std::vector<fiber_consumer*> consumers;
	std::vector<fiber_producer*> producers;

	int nsent = 0;

	for (int i = 0; i < nconsumers; i++) {
		fiber_consumer* consumer = new fiber_consumer(tbox, count, delay);
		consumer->start();
		consumers.push_back(consumer);
	}

	for (int i = 0; i < nproducers; i++) {
		fiber_producer* producer = new fiber_producer(tbox, count);
		producer->start();
		producers.push_back(producer);
		nsent += count;
	}

	acl::fiber::schedule();

	int nget = 0;

	for (std::vector<fiber_consumer*>::iterator it = consumers.begin();
		it != consumers.end(); ++it) {
		nget += (*it)->get_count();
		delete *it;
	}

	for (std::vector<fiber_producer*>::iterator it = producers.begin();
		it != producers.end(); ++it) {
		delete *it;
	}

	if (nget != nsent) {
		printf("Test failed, total get=%d, total sent=%d\r\n",
			nget, nsent);
		return -1;
	}

	return 0;
}
