#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stream/aio_handle.hpp"

namespace acl
{

aio_delay_free::aio_delay_free()
{
	locked_ = false;
	locked_fixed_ = false;
}

aio_delay_free::~aio_delay_free()
{

}

bool aio_delay_free::locked() const
{
	return locked_ || locked_fixed_;
}

void aio_delay_free::set_locked()
{
	locked_fixed_ = true;
}

void aio_delay_free::unset_locked()
{
	locked_fixed_ = false;
}

//////////////////////////////////////////////////////////////////////////

#define DELAY_TIMER_ID	0

class aio_timer_delay_free : public aio_timer_callback
{
public:
	aio_timer_delay_free()
	{
		handle_ = NULL;
	}

	~aio_timer_delay_free()
	{
		std::set<aio_delay_free*>::iterator it;
		it = gc_set_.begin();
		for (; it != gc_set_.end(); ++it)
		{
			if (!(*it)->locked())
				(*it)->destroy();
			else
				logger_error("one timer locked yet!");
		}

		gc_set_.clear();
	}

	virtual void timer_callback(unsigned int id acl_unused)
	{
		std::set<aio_delay_free*>::iterator it, next;
		for (it = gc_set_.begin(); it != gc_set_.end(); it = next)
		{
			next = it;
			++next;
			if (!(*it)->locked())
			{
				(*it)->destroy();
				gc_set_.erase(it);
			}
		}

		acl_assert(handle_);

		// 不管事件引擎是否设置了重复定时器过程，重置本定时器任务
		if (!gc_set_.empty())
			handle_->set_timer(this, 100000, DELAY_TIMER_ID);
		else
			handle_->del_timer(this, DELAY_TIMER_ID);
	}

	bool add(aio_delay_free* callback)
	{
		std::set<aio_delay_free*>::iterator it =
			gc_set_.find(callback);
		if (it != gc_set_.end())
			return false;
		gc_set_.insert(callback);
		return true;
	}

	bool del(aio_delay_free* callback)
	{
		std::set<aio_delay_free*>::iterator it =
			gc_set_.find(callback);
		if (it != gc_set_.end())
		{
			gc_set_.erase(it);
			return true;
		}
		else
			return false;
	}

	aio_handle* handle_;
protected:
private:
	std::set<aio_delay_free*> gc_set_;
};

//////////////////////////////////////////////////////////////////////////

class aio_timer_task
{
public:
	aio_timer_task() {}
	~aio_timer_task() {}
protected:
private:
	friend class aio_timer_callback;

	unsigned int id;
	acl_int64 delay;
	acl_int64 when;
};

//////////////////////////////////////////////////////////////////////////

aio_timer_callback::aio_timer_callback(bool keep /* = false */)
{
	keep_ = keep;
	length_ = 0;
}

aio_timer_callback::~aio_timer_callback(void)
{
	// 如果正在触发定时器的回调过程中析构过程被调用则会发生严重问题
	if (locked())
	{
		logger_error("In trigger proccess, you delete me now!");
		acl_assert(0);
	}

	clear();
}

int aio_timer_callback::clear()
{
	int  n = 0;
	std::list<aio_timer_task*>::iterator it = tasks_.begin();
	for (; it != tasks_.end(); ++it)
	{
		delete (*it);
		n++;
	}
	tasks_.clear();
	length_ = 0;
	return n;
}

bool aio_timer_callback::empty() const
{
	return tasks_.empty();
}

size_t aio_timer_callback::length() const
{
	return length_;
}

void aio_timer_callback::keep_timer(bool on)
{
	keep_ = on;
}

bool aio_timer_callback::keep_timer(void) const
{
	return keep_;
}

void aio_timer_callback::set_time(void)
{
	struct timeval now;
	gettimeofday(&now, NULL);
	present_ = ((unsigned long) now.tv_sec) * 1000000
		+ ((unsigned long) now.tv_usec);
}

#define TIMER_EMPTY		-1

acl_int64 aio_timer_callback::del_task(unsigned int id)
{
	bool ok = false;
	std::list<aio_timer_task*>::iterator it = tasks_.begin();
	for (; it != tasks_.end(); ++it)
	{
		if ((*it)->id == id)
		{
			delete (*it);
			tasks_.erase(it);
			length_--;
			ok = true;
			break;
		}
	}

	if (!ok)
		logger_warn("timer id: %u not found", id);

	if (tasks_.empty())
		return TIMER_EMPTY;

	set_time();

	acl_int64 delay = tasks_.front()->when - present_;
	return delay < 0 ? 0 : delay;
}

acl_int64 aio_timer_callback::set_task(unsigned int id, acl_int64 delay)
{
	aio_timer_task* task = NULL;
	std::list<aio_timer_task*>::iterator it = tasks_.begin();
	for (; it != tasks_.end(); ++it)
	{
		if ((*it)->id == id)
		{
			task = (*it);
			tasks_.erase(it);
			length_--;
			break;
		}
	}

	if (task == NULL)
	{
		task = NEW aio_timer_task();
		task->delay = delay;
		task->id = id;
	}
	else
		task->delay = delay;

	return set_task(task);
}

acl_int64 aio_timer_callback::set_task(aio_timer_task* task)
{
	set_time();
	task->when = present_ + task->delay;

	std::list<aio_timer_task*>::iterator it = tasks_.begin();
	for (; it != tasks_.end(); ++it)
	{
		if (task->when < (*it)->when)
		{
			tasks_.insert(it, task);
			break;
		}
	}

	if (it == tasks_.end())
		tasks_.push_back(task);

	length_++;

	aio_timer_task* first = tasks_.front();
	acl_int64 delay = first->when - present_;
	return delay < 0 ? 0 : delay;
}

acl_int64 aio_timer_callback::trigger(void)
{
	// sanity check
	if (tasks_.empty())
		return TIMER_EMPTY;

	acl_assert(length_ > 0);

	set_time();

	std::list<aio_timer_task*>::iterator it, next;
	std::list<aio_timer_task*> tasks;
	aio_timer_task* task;

	// 从定时器中取出到达的定时任务
	for (it = tasks_.begin(); it != tasks_.end(); it = next)
	{
		if ((*it)->when > present_)
			break;
		next = it;
		++next;
		task = *it;
		tasks_.erase(it);
		length_--;
		tasks.push_back(task);
	}

	// 有可能这些到达的定时任务已经被用户提前删除了
	if (tasks.empty())
	{
		acl_assert(!tasks_.empty());

		aio_timer_task* first = tasks_.front();
		acl_int64 delay = first->when - present_;
		return delay < 0 ? 0 : delay;
	}

	// 将到达的定时任务重新放回至定时器的任务列表中，
	// 并开始触发所有的到达的定时任务

	// 必须先设置触发器的忙状态，以防止子类在回调过程
	// 中调用了该类对象的析构过程
	set_locked();

	// 设置解锁后销毁标志为 false，因为当前该定时器处于
	// 锁定状态，所以其它类对象不能直接在锁定时销毁本类
	// 对象，当解锁后，如果该标识被置为 true，则本类对象
	// 应该自动销毁
	destroy_on_unlock_ = false;

	for (it = tasks.begin(); it != tasks.end(); ++it)
	{
		set_task(*it);
		timer_callback((*it)->id);
	}

	tasks.clear();

	// 允许之后的操作中被子类调用析构过程
	unset_locked();

	// 子类有可能会在 timer_callback 中删除了所有的定时任务
	if (tasks_.empty())
		return TIMER_EMPTY;

	aio_timer_task* first = tasks_.front();
	acl_int64 delay = first->when - present_;

	// 如果在加锁期间外部程序要求释放该对象，则在此处释放
	if (destroy_on_unlock_)
		destroy();
	return delay < 0 ? 0 : delay;
}

//////////////////////////////////////////////////////////////////////////

aio_handle::aio_handle(aio_handle_type engine_type /* = ENGINE_SELECT */
	, unsigned int nMsg /* = 0 */)
	: stop_(false)
	, nstream_(0)
	, engine_type_(engine_type)
{
	int   event_type;

	if (engine_type == ENGINE_SELECT)
		event_type = ACL_EVENT_SELECT;
	else if (engine_type == ENGINE_POLL)
		event_type = ACL_EVENT_POLL;
	else if (engine_type == ENGINE_KERNEL)
		event_type = ACL_EVENT_KERNEL;
#ifdef WIN32
	else if (engine_type == ENGINE_WINMSG)
		event_type = ACL_EVENT_WMSG;
#endif
	else
		acl_assert(0);

	aio_ = acl_aio_create2(event_type, nMsg);
	inner_alloc_ = true;

	// 创建延迟关闭的定时器对象
	delay_free_timer_ = NEW aio_timer_delay_free();

	// 在异步引擎对象的生命周期内锁定该延迟释放定时
	// 器，以防止异步引擎自动销毁该定时器
	delay_free_timer_->set_locked();
}

aio_handle::aio_handle(ACL_AIO* aio)
	: aio_(aio)
	, stop_(false)
	, nstream_(0)
{
	acl_assert(aio_);
	int event_type = acl_aio_event_mode(aio);

	if (event_type == ACL_EVENT_SELECT)
		engine_type_ = ENGINE_SELECT;
	else if (event_type == ACL_EVENT_POLL)
		engine_type_ = ENGINE_POLL;
	else if (event_type == ACL_EVENT_KERNEL)
		engine_type_ = ENGINE_KERNEL;
	else if (event_type == ACL_EVENT_WMSG)
		engine_type_ = ENGINE_WINMSG;
	else
		acl_assert(0);

	inner_alloc_ = false;

	// 创建延迟关闭的定时器对象
	delay_free_timer_ = NEW aio_timer_delay_free();
	// 在异步引擎对象的生命周期内锁定该延迟释放定时
	// 器，以防止异步引擎自动销毁该定时器
	delay_free_timer_->set_locked();
}

aio_handle::~aio_handle()
{
	if (inner_alloc_)
		acl_aio_free(aio_);

	delay_free_timer_->unset_locked();
	delete delay_free_timer_;
}

void aio_handle::keep_read(bool onoff)
{
	acl_aio_set_keep_read(aio_, onoff ? 1 : 0);
}

bool aio_handle::keep_read() const
{
	return acl_aio_get_keep_read(aio_) == 0 ? false : true;
}

acl_int64 aio_handle::set_timer(aio_timer_callback* callback,
	acl_int64 delay, unsigned int id /* = 0 */)
{
	acl_assert(aio_);
	callback->handle_ = this;
	delay = callback->set_task(id, delay);
	return acl_aio_request_timer(aio_,
		(void (*)(int, ACL_EVENT*, void*)) on_timer_callback,
		callback, delay < 0 ? 0 : delay,
		callback->keep_ ? 1 : 0);
}

void aio_handle::on_timer_callback(int, ACL_EVENT*, aio_timer_callback *callback)
{
	acl_assert(callback);
	acl_assert(callback->handle_);
	aio_handle* handle = callback->handle_;

	// 触发定时器中的所有定时任务
	acl_int64 next_delay = callback->trigger();

	// 如果定时器中的任务为空或未设置定时器的重复使用，则删除定时器
	if (callback->empty() || !callback->keep_timer())
	{
		handle->del_timer(callback);
		return;
	}

	// 如果允许重复使用定时器且定时器中的任务非空，则再次设置该定时器

	//  需要重置定时器的到达时间截
	acl_aio_request_timer(handle->aio_,
		(void (*)(int, ACL_EVENT*, void*)) on_timer_callback,
		callback, next_delay < 0 ? 0 : next_delay,
		callback->keep_timer() ? 1 : 0);
}

acl_int64 aio_handle::del_timer(aio_timer_callback* callback, unsigned int id)
{
	acl_assert(aio_);
	acl_int64 next_delay = callback->del_task(id);

	// 如果定时器中的任务为空，则删除该定时器
	if (callback->empty())
		return del_timer(callback);

	return acl_aio_request_timer(callback->handle_->aio_,
		(void (*)(int, ACL_EVENT*, void*)) on_timer_callback,
		callback, next_delay < 0 ? 0 : next_delay,
		callback->keep_timer() ? 1 : 0);
}

acl_int64 aio_handle::del_timer(aio_timer_callback* callback)
{
	acl_assert(aio_);
	acl_int64 next_delay = acl_aio_cancel_timer(aio_,
		(void (*)(int, ACL_EVENT*, void*)) on_timer_callback, callback);
	callback->clear();
	destroy_timer(callback);

	return next_delay;
}

void aio_handle::delay_free(aio_delay_free* callback)
{
	// 添加延迟释放定时器的定时任务
	if (delay_free_timer_->add(callback))
	{
		delay_free_timer_->handle_ = this;
		set_timer(delay_free_timer_, 100000, 0);
	}
}

void aio_handle::destroy_timer(aio_timer_callback* callback)
{
	delay_free_timer_->del(callback);

	// 如果该定时器已经在 trigger 中被锁定，则只需要
	// 设置在解锁后需要将其销毁的标识即可，由其本身
	// 自行销毁
	if (callback->locked())
		callback->destroy_on_unlock_ = true;

	// 直接销毁未锁定的定时器
	else
		callback->destroy();
}

ACL_AIO* aio_handle::get_handle() const
{
	return aio_;
}

aio_handle_type aio_handle::get_engine_type() const
{
	return engine_type_;
}

bool aio_handle::check()
{
	acl_aio_loop(aio_);
	if (stop_)
		return false;
	return true;
}

void aio_handle::stop()
{
	stop_ = true;
}

void aio_handle::reset()
{
	stop_ = false;
}

void aio_handle::increase()
{
	nstream_++;
	on_increase();
}

void aio_handle::decrease()
{
	nstream_--;
	acl_assert(nstream_ >= 0);
	on_decrease();
}

int aio_handle::length() const
{
	return nstream_;
}

}  // namespace acl
