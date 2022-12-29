#include "stdafx.h"
#include <vector>
#include "test_tbox.h"

class test_obj {
public:
	test_obj(void) {}
	~test_obj(void) {}
};

class consumer_schedule : public acl::thread {
public:
	consumer_schedule(acl::fiber_tbox<test_obj>& tbox, int nfiber,
		int count, int ndelay)
	: tbox_(tbox)
	, nfiber_(nfiber)
	, count_(count)
	, delay_(ndelay)
	, ncount_(0)
	{
	}

	~consumer_schedule(void) {}

	int get_count(void) const {
		return ncount_;
	}

private:
	acl::fiber_tbox<test_obj>& tbox_;
	int nfiber_;
	int count_;
	int delay_;
	int ncount_;

	// @override
	void* run(void) {
		for (int i = 0; i < nfiber_; i++) {
			go[&] {
				struct timeval begin, end;
				for (int j = 0; j < count_; j++) {
					gettimeofday(&begin, NULL);
					test_obj* o = tbox_.pop(delay_);
					gettimeofday(&end, NULL);

					if (o) {
						delete o;
						ncount_++;
						continue;
					}

					int cost = (int) acl::stamp_sub(end, begin);
					printf("Fiber pop timeout delay=%d, cost=%d\r\n", delay_, cost);
				}
			};
		}

		acl::fiber::schedule();
		printf("---consumer n=%d---\n", ncount_);
		return NULL;
	}
};

class producer_schedule : public acl::thread {
public:
	producer_schedule(acl::fiber_tbox<test_obj>& tbox, int nfiber, int count)
	: tbox_(tbox)
	, nfiber_(nfiber)
	, count_(count)
	{
	}

	~producer_schedule(void) {}

private:
	acl::fiber_tbox<test_obj>& tbox_;
	int nfiber_;
	int count_;

	// @override
	void* run(void) {
		int n = 0;
		for (int i = 0; i < nfiber_; i++) {
			go[&] {
				for (int j = 0; j < count_; j++) {
					test_obj* o = new test_obj;
					tbox_.push(o);
					n++;
				}
			};
		}

		acl::fiber::schedule();
		return NULL;
	}
};

class consumer : public acl::thread {
public:
	consumer(acl::fiber_tbox<test_obj>& tbox, int count, int ndelay)
	: tbox_(tbox)
	, count_(count)
	, delay_(ndelay)
	, ncount_(0)
	{
	}

	~consumer(void) {}

	int get_count(void) const {
		return ncount_;
	}

private:
	acl::fiber_tbox<test_obj>& tbox_;
	int count_;
	int delay_;
	int ncount_;

	// @override
	void* run(void) {
		for (int i = 0; i < count_; i++) {
			struct timeval begin, end;
			gettimeofday(&begin, NULL);
			test_obj* o = tbox_.pop(delay_);
			gettimeofday(&end, NULL);

			if (o) {
				delete o;
				ncount_++;
				continue;
			}
			int cost = (int) acl::stamp_sub(end, begin);
			printf("Thread pop timeout, delay=%d, cost=%d\r\n", delay_, cost);
		}
		return NULL;
	}
};

class producer : public acl::thread {
public:
	producer(acl::fiber_tbox<test_obj>& tbox, int count)
	: tbox_(tbox)
	, count_(count)
	{
	}

	~producer(void) {}

private:
	acl::fiber_tbox<test_obj>& tbox_;
	int count_;

	// @override
	void* run(void) {
		for (int i = 0; i < count_; i++) {
			test_obj* o = new test_obj;
			tbox_.push(o);
		}
		return NULL;
	}
};

int tbox_mixed_consume(AUT_LINE *test_line, void *arg acl_unused)
{
	int nthread_consumer, nthread_producer;
	int nthread_consumer_alone, nthread_producer_alone;
	int nfiber, count, delay;

	AUT_INT(test_line, "threads_consumer", nthread_consumer, 1);
	AUT_INT(test_line, "threads_producer", nthread_producer, 1);
	AUT_INT(test_line, "threads_consumer_alone", nthread_consumer_alone, 1);
	AUT_INT(test_line, "threads_producer_alone", nthread_producer_alone, 1);
	AUT_INT(test_line, "fibers", nfiber, 10);
	AUT_INT(test_line, "count", count, 1);
	AUT_INT(test_line, "delay", delay, 100);

	acl::fiber_tbox<test_obj> tbox;

	int nsent = 0;

	std::vector<consumer*> consumers;
	std::vector<producer*> producers;
	std::vector<consumer_schedule*> consumers_schedule;
	std::vector<producer_schedule*> producers_schedule;

	for (int i = 0; i < nthread_consumer_alone; i++) {
		consumer* thr = new consumer(tbox, count, delay);
		thr->start();
		consumers.push_back(thr);
	}

	for (int i = 0; i < nthread_producer_alone; i++) {
		producer* thr = new producer(tbox, count);
		thr->start();
		producers.push_back(thr);
		nsent += count;
	}

	for (int i = 0; i < nthread_consumer; i++) {
		consumer_schedule* thr = new consumer_schedule(tbox, nfiber,
				count, delay);
		thr->start();
		consumers_schedule.push_back(thr);
	}

	for (int i = 0; i < nthread_producer; i++) {
		producer_schedule* thr = new producer_schedule(tbox, nfiber,
				count);
		thr->start();
		nsent += nfiber * count;
		producers_schedule.push_back(thr);
	}

	int nget = 0;

	for (std::vector<consumer*>::iterator it = consumers.begin();
		it != consumers.end(); ++it) {
		(*it)->wait();
		nget += (*it)->get_count();
		delete *it;
	}

	for (std::vector<producer*>::iterator it = producers.begin();
		it != producers.end(); ++it) {
		(*it)->wait();
		delete *it;
	}

	for (std::vector<consumer_schedule*>::iterator
		it = consumers_schedule.begin();
		it != consumers_schedule.end(); ++it) {
		(*it)->wait();
		nget += (*it)->get_count();
		delete *it;
	}

	for (std::vector<producer_schedule*>::iterator
		it = producers_schedule.begin();
		it != producers_schedule.end(); ++it) {
		(*it)->wait();
		delete *it;
	}

	if (nget != nsent) {
		printf("Test failed, total get=%d, total sent=%d\r\n",
			nget, nsent);
		return -1;
	}

	return 0;
}
