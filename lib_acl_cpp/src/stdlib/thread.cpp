#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/thread.hpp"
#include "acl_cpp/stdlib/tbox.hpp"
#include "acl_cpp/stdlib/atomic.hpp"
#endif

#ifdef	ACL_FREEBSD
#include <pthread_np.h>
#endif

namespace acl
{

thread::thread(void)
: detachable_(false)
, stack_size_(0)
, thread_id_(0)
{
#ifdef ACL_WINDOWS
	thread_ = (acl_pthread_t*) acl_mycalloc(1, sizeof(acl_pthread_t));
#endif
	return_arg_ = NULL;
	sync_ = NEW tbox<int>;
	lock_ = NEW atomic_long;
}

thread::~thread(void)
{
#ifdef ACL_WINDOWS
	acl_myfree(thread_);
#endif
	delete sync_;
	delete lock_;
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

void thread::wait_for_running(void)
{
	if (lock_->cas(0, 1) == 0) {
		if (thread_id_ == 0) {
			(void) sync_->pop();
		}
	} else if (thread_id_ == 0) {
		sleep(1);
	}
}

void* thread::thread_run(void* arg)
{
	thread* thr = (thread*) arg;
#ifdef	ACL_WINDOWS
	thr->thread_id_ = GetCurrentThreadId();
#elif	defined(ACL_FREEBSD)
#if defined(__FreeBSD__) && (__FreeBSD__ >= 9)
	thr->thread_id_ = pthread_getthreadid_np();
#else
	thr->thread_id_ = (unsigned long) pthread_self();
#endif
#elif	defined(ACL_MACOSX)
	unsigned long long n;
	(void) pthread_threadid_np(NULL, &n);
	thr->thread_id_ = (unsigned long) n;
#else
	thr->thread_id_ = (unsigned long) pthread_self();
#endif

	// 先将之前 push 到队列里可能还未清除的消息清除，以免该线程对象
	// 再被重复使用启动线程时造成运行时内存泄露
	while (true) {
		bool found;
		thr->sync_->pop(0, &found);
		if (!found) {
			break;
		}
	}

	thr->init();

	thr->sync_->push(NULL);

	// 如果线程创建时为分离模式，则当 run 运行时用户有可能
	// 将线程对象销毁了，所以不能再将 thr->return_arg_ 进行
	// 赋值，否则就有可能出现内存非法访问
	if (thr->detachable_) {
		return thr->run();
	}

	thr->return_arg_ = thr->run();
	return thr->return_arg_;
}

bool thread::start(bool sync /* = false */)
{
	acl_pthread_attr_t attr;
	acl_pthread_attr_init(&attr);

	// 当一个线程对象被重复使用时，为了防止 wait(void** out /* = NULL */)
	// 执行时 logger_warn("pthread_josin's arg invalid?") 报错
	// --- by 562351190@qq.com 
	thread_id_ = 0;

	if (detachable_) {
		acl_pthread_attr_setdetachstate(&attr, 1);
	}
	if (stack_size_ > 0) {
		acl_pthread_attr_setstacksize(&attr, stack_size_);
	}

#ifdef	ACL_WINDOWS
	int   ret = acl_pthread_create((acl_pthread_t*) thread_,
			&attr, thread_run, this);
#else
	int   ret = acl_pthread_create((pthread_t*) &thread_, &attr,
			thread_run, this);
#endif
	if (ret != 0) {
		acl_set_error(ret);
		logger_error("create thread error %s", last_serror());
		return false;
	}

	if (sync) {
		wait_for_running();
	}

	// 如果线程创建足够快，在 thread_run 中有可能用户将线程对象释放，
	// 则下面的代码就会造成内存非法访问
#if 0
#ifdef	ACL_WINDOWS
	thread_id_ = ((acl_pthread_t*) thread_)->id;
#elif	defined(ACL_LINUX)
	thread_id_ = (unsigned long int) thread_;
#endif
#endif
	return true;
}

bool thread::wait(void** out /* = NULL */)
{
	if (detachable_) {
		logger_error("detachable thread can't be wait!");
		return false;
	}

	wait_for_running();

	if (thread_id_ == 0) {
		logger_error("thread not running!");
		return false;
	}

	void* ptr;

#ifdef ACL_WINDOWS
	int   ret = acl_pthread_join(*((acl_pthread_t*) thread_), &ptr);
#else
	int   ret = acl_pthread_join(thread_, &ptr);
#endif

	if (ret != 0) {
		acl_set_error(ret);
		logger_error("pthread_join error: %s", last_serror());
		return false;
	}

	// 比较通过在 thread_run 中截获的参数与 pthread_join 获得的参数是否相同
	if (ptr != return_arg_) {
		logger_warn("pthread_josin's arg invalid?");
	}

	if (out) {
		*out = ptr;
	}
	return true;
}

unsigned long thread::thread_id(void) const
{
	const_cast<thread*>(this)->wait_for_running();

	if (thread_id_ == 0) {
		logger_error("thread not running!");
		return 0;
	}

	return thread_id_;
}

unsigned long thread::thread_self(void)
{
#ifdef	ACL_FREEBSD
#if defined(__FreeBSD__) && (__FreeBSD__ >= 9)
	return (unsigned long) pthread_getthreadid_np();
#else
	return (unsigned long) acl_pthread_self();
#endif
#elif	defined(ACL_MACOSX)
	unsigned long long n;
	(void) pthread_threadid_np(NULL, &n);
	return (unsigned long) n;
#else
	return (unsigned long) acl_pthread_self();
#endif
}

} // namespace acl
