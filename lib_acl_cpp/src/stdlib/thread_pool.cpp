#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/thread.hpp"
#include "acl_cpp/stdlib/thread_pool.hpp"
#endif

namespace acl
{

thread_pool::thread_pool()
: stack_size_(0)
, threads_limit_(100)
, thread_idle_(0)
, thr_pool_(NULL)
{
	thr_attr_ = (acl_pthread_pool_attr_t*)
		acl_mycalloc(1, sizeof(acl_pthread_pool_attr_t));
}

thread_pool::~thread_pool()
{
	if (thr_pool_ != NULL)
		stop();

	acl_myfree(thr_attr_);
}

thread_pool& thread_pool::set_stacksize(size_t size)
{
	stack_size_ = size;
	return *this;
}

thread_pool& thread_pool::set_limit(size_t n)
{
	threads_limit_ = n;
	return *this;
}

thread_pool& thread_pool::set_idle(int ttl)
{
	thread_idle_ = ttl;
	return *this;
}

void thread_pool::start()
{
	if (thr_pool_)
		return;

	acl_pthread_pool_attr_set_stacksize(thr_attr_, stack_size_);
	acl_pthread_pool_attr_set_threads_limit(thr_attr_, (int) threads_limit_);
	acl_pthread_pool_attr_set_idle_timeout(thr_attr_, thread_idle_);

	thr_pool_ = acl_pthread_pool_create(thr_attr_);
	acl_pthread_pool_atinit(thr_pool_, thread_init, this);
	acl_pthread_pool_atfree(thr_pool_, thread_exit, this);
}

void thread_pool::stop()
{
	if (thr_pool_)
	{
		acl_pthread_pool_destroy(thr_pool_);
		thr_pool_ = NULL;
	}
}

void thread_pool::wait()
{
	if (thr_pool_)
		acl_pthread_pool_stop(thr_pool_);
	else
		logger_warn("no thread working, call start first!");
}

bool thread_pool::run(thread_job* job)
{
	if (job == NULL)
	{
		logger_error("thr null!");
		return false;
	}

	if (thr_pool_ == NULL)
	{
		logger_error("start() not called yet!");
		return false;
	}

	acl_pthread_pool_add(thr_pool_, thread_run, job);
	return true;
}

bool thread_pool::execute(thread_job* job)
{
	return run(job);
}

int  thread_pool::threads_count() const
{
	if (thr_pool_ == NULL)
	{
		logger_error("start() not called yet!");
		return -1;
	}

	return acl_pthread_pool_size(thr_pool_);
}

int  thread_pool::task_qlen() const
{
	if (thr_pool_ == NULL)
	{
		logger_error("start() not called yet!");
		return -1;
	}
	return acl_pthread_pool_qlen(thr_pool_);
}

void thread_pool::thread_run(void* arg)
{
	thread_job* job = (thread_job*) arg;
	job->run();
}

int  thread_pool::thread_init(void* arg)
{
	thread_pool* threads = (thread_pool*) arg;
	return threads->thread_on_init() ? 0 : -1;
}

void thread_pool::thread_exit(void* arg)
{
	thread_pool* threads = (thread_pool*) arg;
	threads->thread_on_exit();
}

} // namespace acl
