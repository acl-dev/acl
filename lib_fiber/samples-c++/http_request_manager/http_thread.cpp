#include "stdafx.h"
#include "http_thread.h"

static acl::atomic_long __count = 0;

double http_thread::stamp_sub(const struct timeval *from,
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

static bool http_request(acl::http_request_pool& pool)
{
	acl::http_guard guard(pool);
	acl::http_request* conn = (acl::http_request*) guard.peek();
	if (conn == NULL) {
		printf("peek connection error\r\n");
		return false;
	}

	acl::http_header& header = conn->request_header();
	header.set_url("/")
		.set_keep_alive(true)
		.accept_gzip(true);
	header.set_host(pool.get_addr());
	if (!conn->request(NULL, 0)) {
		guard.set_keep(false);
		printf("send request failed!\r\n");
		return false;
	}

	acl::string body;
	if (!conn->get_body(body)) {
		guard.set_keep(false);
		return false;
	}

	//printf("[%s]\r\n", body.c_str());
	printf("read body=%ld\r\n", (long) body.size());
	return true;
}

void http_thread::fiber_http(ACL_FIBER *, void *ctx)
{
	http_thread* thread = (http_thread*) ctx;
	acl::connect_manager *cluster = &thread->get_cluster();
	int oper_count = thread->get_oper_count();

	int i = 0;

	struct timeval last, now;

	gettimeofday(&last, NULL);

	for (; i < oper_count; i++) {
		acl::http_request_pool* pool =
			(acl::http_request_pool*) cluster->peek();
		assert(pool);
		http_request(*pool);
	}

	gettimeofday(&now, NULL);
	double spent = stamp_sub(&now, &last);

	long long n = ++__count;
	printf("---set spent %.2f ms, count %d, speed: %.2f, count=%lld----\r\n",
		spent, i, (i * 1000) / (spent > 0 ? spent : 1), n);

	thread->fiber_dec(i);
}

http_thread::http_thread(const char* addr, int conn_timeout, int rw_timeout,
	int fibers_max, int stack_size, int oper_count)
: addr_(addr)
, conn_timeout_(conn_timeout)
, rw_timeout_(rw_timeout)
, fibers_max_(fibers_max)
, fibers_cnt_(fibers_max)
, stack_size_(stack_size)
, oper_count_(oper_count)
{
	printf("addr: %s\r\n", addr_.c_str());
	cluster_internal_ = new acl::http_request_manager;
	cluster_ = cluster_internal_;

	cluster_->set(addr_.c_str(), 0, conn_timeout_, rw_timeout_);
}

http_thread::http_thread(acl::http_request_manager& cluster,
	int fibers_max, int stack_size, int oper_count)
: fibers_max_(fibers_max)
, fibers_cnt_(fibers_max)
, stack_size_(stack_size)
, oper_count_(oper_count)
, cluster_(&cluster)
, cluster_internal_(NULL)
{
}

http_thread::~http_thread(void)
{
	delete cluster_internal_;
}

void* http_thread::run(void)
{
	gettimeofday(&begin_, NULL);

	for (int i = 0; i < fibers_max_; i++) {
		acl_fiber_create(fiber_http, this, stack_size_);
	}

	acl_fiber_schedule();

	return NULL;
}

void http_thread::fiber_dec(int cnt)
{
	if (--fibers_cnt_ > 0) {
		return;
	}

	struct timeval end;
	long long total = fibers_max_ * cnt * 3;

	gettimeofday(&end, NULL);
	double spent = stamp_sub(&end, &begin_);
	printf("fibers: %d, count: %lld, spent: %.2f, speed: %.2f\r\n",
		fibers_max_, total, spent,
		(total * 1000) / (spent > 0 ? spent : 1));
		
//	acl_fiber_schedule_stop();
}
