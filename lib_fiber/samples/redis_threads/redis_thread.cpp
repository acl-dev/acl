#include "stdafx.h"
#include "redis_thread.h"

static double stamp_sub(const struct timeval *from,
	const struct timeval *sub_by)
{
	struct timeval res;

	memcpy(&res, from, sizeof(struct timeval));

	res.tv_usec -= sub_by->tv_usec;
	if (res.tv_usec < 0) {
		--res.tv_sec;
		res.tv_usec += 1000000;
	}
	res.tv_sec -= sub_by->tv_sec;

	return (res.tv_sec * 1000.0 + res.tv_usec/1000.0);
}

static int redis_set(acl::redis& cmd, const struct timeval& begin, int count)
{
	acl::string key, val;
	int i = 0;

	for (; i < count; i++) {
		key.format("key-%lu-%d-%d", acl::thread::self(),
			acl_fiber_self(), i);
		val.format("val-%lu-%d-%d", acl::thread::self(),
			acl_fiber_self(), i);

		if (cmd.set(key, val) == false) {
			printf("fiber-%d: set error: %s, key: %s\r\n",
				acl_fiber_self(), cmd.result_error(),
				key.c_str());
			break;
		} else if (i < 5) {
			printf("fiber-%d: set ok, key: %s\r\n",
				acl_fiber_self(), key.c_str());
		}

		cmd.clear();
	}

	struct timeval now;
	gettimeofday(&now, NULL);
	double spent = stamp_sub(&now, &begin);
	printf("---set spent %.2f ms, count %d, speed: %.2f----\r\n",
		spent, i, (i * 1000) / (spent > 0 ? spent : 1));
	return i;
}

static int redis_get(acl::redis& cmd, const struct timeval& begin, int count)
{
	acl::string key, val;
	int i = 0;
	for (; i < count; i++) {
		key.format("key-%lu-%d-%d", acl::thread::self(),
			acl_fiber_self(), i);

		if (cmd.get(key, val) == false) {
			printf("fiber-%d: get error: %s, key: %s\r\n",
				acl_fiber_self(), cmd.result_error(),
				key.c_str());
			break;
		}
		val.clear();
		cmd.clear();
	}

	struct timeval now;
	gettimeofday(&now, NULL);
	double spent = stamp_sub(&now, &begin);
	printf("---get spent %.2f ms, count %d, speed: %.2f----\r\n",
		spent, i, (i * 1000) / (spent > 0 ? spent : 1));
	return i;
}

static int redis_del(acl::redis& cmd, const struct timeval& begin, int count)
{
	acl::string key;
	int i = 0;
	for (; i < count; i++) {
		key.format("key-%lu-%d-%d", acl::thread::self(),
			acl_fiber_self(), i);

		if (cmd.del_one(key) < 0) {
			printf("fiber-%d: del error: %s, key: %s\r\n",
				acl_fiber_self(), cmd.result_error(),
				key.c_str());
			break;
		}
		cmd.clear();
	}

	struct timeval now;
	gettimeofday(&now, NULL);
	double spent = stamp_sub(&now, &begin);
	printf("---del spent %.2f ms, count %d, speed: %.2f----\r\n",
		spent, i, (i * 1000) / (spent > 0 ? spent : 1));

	return i;
}

void redis_thread::fiber_redis(ACL_FIBER *, void *ctx)
{
	redis_thread* thread = (redis_thread*) ctx;
	acl::redis_client_cluster *cluster = &thread->get_cluster();
	acl::redis redis(cluster);
	int oper_count = thread->get_oper_count();
	const acl::string& cmd = thread->get_cmd();

	int n = 0;
	struct timeval begin;

	gettimeofday(&begin, NULL);
	if (cmd == "set" || cmd == "all") {
		n += redis_set(redis, begin, oper_count);
	}

	if (cmd == "get" || cmd == "all") {
		gettimeofday(&begin, NULL);
		n += redis_get(redis, begin, oper_count);
	}

	if (cmd == "del" || cmd == "all") {
		gettimeofday(&begin, NULL);
		n += redis_del(redis, begin, oper_count);
	}

	thread->fiber_dec(n);
}

redis_thread::redis_thread(const char* addr, const char* passwd,
	int conn_timeout, int rw_timeout, int fibers_max, int stack_size,
	int oper_count, const char* cmd)
: addr_(addr)
, passwd_(passwd)
, conn_timeout_(conn_timeout)
, rw_timeout_(rw_timeout)
, fibers_max_(fibers_max)
, fibers_cnt_(fibers_max)
, stack_size_(stack_size)
, oper_count_(oper_count)
, cmd_(cmd)
{
	printf("addr: %s\r\n", addr_.c_str());
	cluster_internal_ = new acl::redis_client_cluster;
	cluster_ = cluster_internal_;

	cluster_->set(addr_.c_str(), 0, conn_timeout_, rw_timeout_);
	cluster_->set_password("default", passwd_);
}

redis_thread::redis_thread(acl::redis_client_cluster& cluster, int fibers_max,
	int stack_size, int oper_count, const char* cmd)
: fibers_max_(fibers_max)
, fibers_cnt_(fibers_max)
, stack_size_(stack_size)
, oper_count_(oper_count)
, cmd_(cmd)
, cluster_(&cluster)
, cluster_internal_(NULL)
{
}

redis_thread::~redis_thread(void)
{
	delete cluster_internal_;
}

void* redis_thread::run(void)
{
	gettimeofday(&begin_, NULL);

	for (int i = 0; i < fibers_max_; i++) {
		acl_fiber_create(fiber_redis, this, stack_size_);
	}

	acl_fiber_schedule();

	return NULL;
}

void redis_thread::fiber_dec(int cnt)
{
	if (--fibers_cnt_ > 0) {
		return;
	}

	struct timeval end;
	long long total = fibers_max_ * cnt;

	gettimeofday(&end, NULL);
	double spent = stamp_sub(&end, &begin_);
	printf("fibers: %d, count: %lld, spent: %.2f, speed: %.2f\r\n",
		fibers_max_, total, spent,
		(total * 1000) / (spent > 0 ? spent : 1));
}
