#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>

class base_task
{
public:
	base_task(int id) : id_(id), key_(0) {}
	virtual ~base_task(void) {}

	// @override
	bool on_trigger(void)
	{
		trigger_me();
		delete this;
		return false;
	}

	// @override
	int get_ttl(void) const
	{
		return 1000;
	}

	// @override
	void set_key(long long key)
	{
		key_ = key;
	}

	// @override
	long long get_key(void) const
	{
		return key_;
	}

protected:
	int id_;

	virtual void trigger_me(void) = 0;

private:
	long long key_;
};

class mytask1 : public base_task
{
public:
	mytask1(const char* name, int id) : base_task(id), name_(name) {}
	~mytask1(void) { printf("%s free now\r\n", name_.c_str()); }

protected:
	// @override
	void trigger_me(void)
	{
		printf("%s->%s id=%d\r\n", __FUNCTION__, name_.c_str(), id_);
	}

private:
	acl::string name_;
};

class mytask2 : public base_task
{
public:
	mytask2(const char* name, int id) : base_task(id), name_(name) {}
	~mytask2(void) { printf("%s free now\r\n", name_.c_str()); }

protected:
	// @override
	void trigger_me(void)
	{
		printf("%s->%s id=%d\r\n", __FUNCTION__, name_.c_str(), id_);
	}

private:
	acl::string name_;
};

class producer_fiber : public acl::fiber
{
public:
	producer_fiber(acl::fiber_trigger<base_task>& trigger) : trigger_(trigger) {}
	~producer_fiber(void) {}

protected:
	void run(void)
	{
		int n = 0, max = 6;
		while (n < max)
		{
			base_task* task;
			if (n % 2 == 0)
				task = new mytask1("mytask1", n);
			else
				task = new mytask2("mytask2", n);
			n++;
			trigger_.add(task);
			sleep(1);
		}

		sleep(2);
		acl::fiber::schedule_stop();
	}

private:
	acl::fiber_trigger<base_task>& trigger_;
};

//////////////////////////////////////////////////////////////////////////////

int main(void)
{
	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	acl::timer_trigger<base_task> timer;
	acl::fiber_trigger<base_task> trigger(timer);
	trigger.start();

	producer_fiber producer(trigger);
	producer.start();

	acl::fiber::schedule();
	return 0;
}
