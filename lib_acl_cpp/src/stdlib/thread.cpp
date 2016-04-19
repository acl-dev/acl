#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/thread.hpp"
#endif

#ifdef	ACL_FREEBSD
#include <pthread_np.h>
#endif

namespace acl
{

thread::thread()
: detachable_(true)
, stack_size_(0)
, thread_id_(0)
{
#ifdef ACL_WINDOWS
	thread_ = (acl_pthread_t*) acl_mycalloc(1, sizeof(acl_pthread_t));
#endif
	return_arg_ = NULL;
}

thread::~thread()
{
#ifdef ACL_WINDOWS
	acl_myfree(thread_);
#endif
}

thread& thread::set_detachable(bool yes /* = true */)
{
	detachable_ = yes;
	return *this;
}

thread& thread::set_stacksize(size_t size)
{
	stack_size_ = size;
	return *this;
}

void* thread::thread_run(void* arg)
{
	thread* thr = (thread*) arg;
#ifdef	ACL_WINDOWS
	thr->thread_id_ = GetCurrentThreadId();
#elif	defined(ACL_FREEBSD)
	thr->thread_id_ = pthread_getthreadid_np();
#else
	thr->thread_id_ = (unsigned long) pthread_self();
#endif

	// 如果线程创建时为分离模式，则当 run 运行时用户有可能
	// 将线程对象销毁了，所以不能再将 thr->return_arg_ 进行
	// 赋值，否则就有可能出现内存非法访问
	if (thr->detachable_)
		return thr->run();

	thr->return_arg_ = thr->run();
	return thr->return_arg_;
}

bool thread::start()
{
	acl_pthread_attr_t attr;
	acl_pthread_attr_init(&attr);

	if (detachable_)
		acl_pthread_attr_setdetachstate(&attr, 1);
	if (stack_size_ > 0)
		acl_pthread_attr_setstacksize(&attr, stack_size_);

#ifdef	ACL_WINDOWS
	int   ret = acl_pthread_create((acl_pthread_t*) thread_,
			&attr, thread_run, this);
#else
	int   ret = acl_pthread_create((pthread_t*) &thread_, &attr,
			thread_run, this);
#endif
	if (ret != 0)
	{
		acl_set_error(ret);
		logger_error("create thread error %s", last_serror());
		return false;
	}

	// 如果线程创建足够快，在 thread_run 中有可能用户将线程对象释放，
	// 则下面的代码就会造成内存非法访问
#if 0
#ifdef	ACL_WINDOWS
	thread_id_ = ((acl_pthread_t*) thread_)->id;
#elif	defined(LINUX2)
	thread_id_ = (unsigned long int) thread_;
#endif
#endif
	return true;
}

bool thread::wait(void** out /* = NULL */)
{
	if (detachable_)
	{
		logger_error("detachable thread can't be wait!");
		return false;
	}

	// 尝试等待线程创建成功
	for (int i = 0; i < 10; i++)
	{
		if (thread_id_ != 0)
			break;
		sleep(1);
	}

	if (thread_id_ == 0)
	{
		logger_error("thread not running!");
		return false;
	}

	void* ptr;

#ifdef ACL_WINDOWS
	int   ret = acl_pthread_join(*((acl_pthread_t*) thread_), &ptr);
#else
	int   ret = acl_pthread_join(thread_, &ptr);
#endif

	if (ret != 0)
	{
		acl_set_error(ret);
		logger_error("pthread_join error: %s", last_serror());
		return false;
	}

	// 比较通过在 thread_run 中截获的参数与 pthread_join 获得的参数是否相同
	if (ptr != return_arg_)
		logger_warn("pthread_josin's arg invalid?");

	if (out)
		*out = ptr;
	return true;
}

unsigned long thread::thread_id() const
{
	// 尝试等待线程创建成功
	for (int i = 0; i < 10; i++)
	{
		if (thread_id_ != 0)
			break;
		sleep(1);
	}

	if (thread_id_ == 0)
	{
		logger_error("thread not running!");
		return 0;
	}

	return thread_id_;
}

unsigned long thread::thread_self()
{
#ifdef	ACL_FREEBSD
	return (unsigned long) pthread_getthreadid_np();
#else
	return (unsigned long) acl_pthread_self();
#endif
}

} // namespace acl
