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

static int redis_set(int tid, int fid, acl::redis& cmd,
	const struct timeval& begin, int count)
{
	acl::string key, val;
	int i = 0;

	for (; i < count; i++) {
		key.format("key-%d-%d-%d", tid, fid, i);
		val.format("val-%d-%d-%d", tid, fid, i);

		if (cmd.set(key, val) == false) {
			printf("fiber-%d: set error: %s, key: %s\r\n",
				acl::fiber::self(), cmd.result_error(),
				key.c_str());
			break;
		} else if (i < 5) {
			printf("fiber-%d: set ok, key: %s\r\n",
				acl::fiber::self(), key.c_str());
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

static int redis_get(int tid, int fid, acl::redis& cmd,
	const struct timeval& begin, int count)
{
	acl::string key, val;
	int i = 0;
	for (; i < count; i++) {
		key.format("key-%d-%d-%d", tid, fid, i);

		if (cmd.get(key, val) == false) {
			printf("fiber-%d: get error: %s, key: %s\r\n",
				acl::fiber::self(), cmd.result_error(),
				key.c_str());
			break;
		} else if (i < 5) {
			printf("fiber-%d: get ok, key: %s, val: %s\r\n",
				acl::fiber::self(), key.c_str(), val.c_str());
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

static int redis_del(int tid, int fid, acl::redis& cmd,
	const struct timeval& begin, int count)
{
	acl::string key;
	int i = 0;
	for (; i < count; i++) {
		key.format("key-%d-%d-%d", tid, fid, i);

		if (cmd.del_one(key) < 0) {
			printf("fiber-%d: del error: %s, key: %s\r\n",
				acl::fiber::self(), cmd.result_error(),
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

///////////////////////////////////////////////////////////////////////////////

class fiber_redis : public acl::fiber {
public:
	fiber_redis(int fid, redis_thread& thread)
	: fid_(fid), thread_(thread)
	{
	}

private:
	~fiber_redis() {}

protected:
	// @override
	void run() {
		acl::redis_client_cluster *cluster = &thread_.get_cluster();
		acl::redis redis(cluster);
		int oper_count = thread_.get_oper_count();
		const acl::string& cmd = thread_.get_cmd();

		int n = 0;
		struct timeval begin;

		if (cmd == "set" || cmd == "all") {
			gettimeofday(&begin, NULL);
			n += redis_set(thread_.get_tid(), fid_, redis,
				begin, oper_count);
		}

		if (cmd == "get" || cmd == "all") {
			gettimeofday(&begin, NULL);
			n += redis_get(thread_.get_tid(), fid_, redis,
				begin, oper_count);
		}

		if (cmd == "del" || cmd == "all") {
			gettimeofday(&begin, NULL);
			n += redis_del(thread_.get_tid(), fid_, redis,
				begin, oper_count);
		}

		thread_.fiber_dec(n);

		delete this;
	}

private:
	int fid_;
	redis_thread& thread_;
};

///////////////////////////////////////////////////////////////////////////////

redis_thread::redis_thread(int tid, const char* addr, const char* passwd,
	int conn_timeout, int rw_timeout, int fibers_max, int stack_size,
	int oper_count, const char* cmd)
: tid_(tid)
, addr_(addr)
, passwd_(passwd)
, conn_timeout_(conn_timeout)
, rw_timeout_(rw_timeout)
, fibers_max_(fibers_max)
, fibers_cnt_(fibers_max)
, stack_size_(stack_size)
, oper_count_(oper_count)
, cmd_(cmd)
{
	(void) stack_size_;
	printf("addr: %s\r\n", addr_.c_str());
	cluster_internal_ = new acl::redis_client_cluster;
	cluster_ = cluster_internal_;

	cluster_->set(addr_.c_str(), 0, conn_timeout_, rw_timeout_);
	cluster_->set_password("default", passwd_);
}

redis_thread::redis_thread(int tid, acl::redis_client_cluster& cluster,
	int fibers_max, int stack_size, int oper_count, const char* cmd)
: tid_(tid)
, fibers_max_(fibers_max)
, fibers_cnt_(fibers_max)
, stack_size_(stack_size)
, oper_count_(oper_count)
, cmd_(cmd)
, cluster_(&cluster)
, cluster_internal_(NULL)
{
	(void) stack_size_;
}

redis_thread::~redis_thread(void)
{
	delete cluster_internal_;
}

void* redis_thread::run(void)
{
	gettimeofday(&begin_, NULL);

	for (int i = 0; i < fibers_max_; i++) {
		acl::fiber* fb = new fiber_redis(i, *this);
		fb->start();
	}

	acl::fiber::schedule();

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
