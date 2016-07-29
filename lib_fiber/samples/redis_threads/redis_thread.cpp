#include "stdafx.h"
#include "redis_thread.h"

double redis_thread::stamp_sub(const struct timeval *from,
	const struct timeval *sub_by)
{
	struct timeval res;

	memcpy(&res, from, sizeof(struct timeval));

	res.tv_usec -= sub_by->tv_usec;
	if (res.tv_usec < 0)
	{
		--res.tv_sec;
		res.tv_usec += 1000000;
	}
	res.tv_sec -= sub_by->tv_sec;

	return (res.tv_sec * 1000.0 + res.tv_usec/1000.0);
}

void redis_thread::fiber_redis(ACL_FIBER *fiber, void *ctx)
{
	redis_thread* thread = (redis_thread*) ctx;
	acl::redis_client_cluster *cluster = &thread->get_cluster();
	acl::redis cmd(cluster);
	int oper_count = thread->get_oper_count();

	acl::string key, val;

	int i = 0;

	struct timeval last, now;

	gettimeofday(&last, NULL);

	for (; i < oper_count; i++)
	{
		key.format("key-%lu-%d-%d", thread->thread_id(),
			acl_fiber_id(fiber), i);
		val.format("val-%lu-%d-%d", thread->thread_id(),
			acl_fiber_id(fiber), i);

		if (cmd.set(key, val) == false)
		{
			printf("fiber-%d: set error: %s, key: %s\r\n",
				acl_fiber_id(fiber), cmd.result_error(),
				key.c_str());
			break;
		} else if (i < 5)
			printf("fiber-%d: set ok, key: %s\r\n",
				acl_fiber_id(fiber), key.c_str());
		cmd.clear();
	}

	gettimeofday(&now, NULL);
	double spent = stamp_sub(&now, &last);
	printf("---set spent %.2f ms, count %d, speed: %.2f----\r\n",
		spent, i, (i * 1000) / (spent > 0 ? spent : 1));

	gettimeofday(&last, NULL);

	for (int j = 0; j < i; j++)
	{
		key.format("key-%lu-%d-%d", thread->thread_id(),
			acl_fiber_id(fiber), j);

		if (cmd.get(key, val) == false)
		{
			printf("fiber-%d: get error: %s, key: %s\r\n",
				acl_fiber_id(fiber), cmd.result_error(),
				key.c_str());
			break;
		}
		val.clear();
		cmd.clear();
	}

	gettimeofday(&now, NULL);
	spent = stamp_sub(&now, &last);
	printf("---get spent %.2f ms, count %d, speed: %.2f----\r\n",
		spent, i, (i * 1000) / (spent > 0 ? spent : 1));

	gettimeofday(&last, NULL);

	for (int j = 0; j < i; j++)
	{
		key.format("key-%lu-%d-%d", thread->thread_id(),
			acl_fiber_id(fiber), j);

		if (cmd.del_one(key) < 0)
		{
			printf("fiber-%d: del error: %s, key: %s\r\n",
				acl_fiber_id(fiber), cmd.result_error(),
				key.c_str());
			break;
		}
		cmd.clear();
	}

	gettimeofday(&now, NULL);
	spent = stamp_sub(&now, &last);
	printf("---del spent %.2f ms, count %d, speed: %.2f----\r\n",
		spent, i, (i * 1000) / (spent > 0 ? spent : 1));

	thread->fiber_dec(i);
}

redis_thread::redis_thread(const char* addr, int conn_timeout, int rw_timeout,
	int fibers_max, int stack_size, int oper_count)
	: addr_(addr)
	, conn_timeout_(conn_timeout)
	, rw_timeout_(rw_timeout)
	, fibers_max_(fibers_max)
	, fibers_cnt_(fibers_max)
	, stack_size_(stack_size)
	, oper_count_(oper_count)
{
}

void* redis_thread::run(void)
{
	printf("addr: %s\r\n", addr_.c_str());
	cluster_.set(addr_.c_str(), 0, conn_timeout_, rw_timeout_);

	gettimeofday(&begin_, NULL);

	for (int i = 0; i < fibers_max_; i++)
		acl_fiber_create(fiber_redis, this, stack_size_);

	acl_fiber_schedule();

	return NULL;
}

void redis_thread::fiber_dec(int cnt)
{
	if (--fibers_cnt_ > 0)
		return;

	struct timeval end;
	long long total = fibers_max_ * cnt * 3;

	gettimeofday(&end, NULL);
	double spent = stamp_sub(&end, &begin_);
	printf("fibers: %d, count: %lld, spent: %.2f, speed: %.2f\r\n",
		fibers_max_, total, spent,
		(total * 1000) / (spent > 0 ? spent : 1));
		
	acl_fiber_stop();
}
