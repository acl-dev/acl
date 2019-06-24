#pragma once
#include "../acl_cpp_define.hpp"
#include <map>
#include <vector>
#include "noncopyable.hpp"
#include "mbox.hpp"
#include "util.hpp"
#include "thread.hpp"
#include "thread_mutex.hpp"

namespace acl {

/**
 * 具有相同时间截的定时任务的集合
 */
template <typename T>
class trigger_item : public noncopyable
{
public:
	typedef std::map<long long, trigger_item<T>*> trigger_items_t;

	trigger_item(trigger_items_t& items) : items_(items) {}
	~trigger_item(void) {}

	/**
	 * 添加一个定时任务
	 * @pararm o {T*}
	 */
	void add(T* o)
	{
		objs_.push_back(o);
	}

	/**
	 * 删除一个具有相同时间截的定时任务
	 * @pararm o {T*}
	 * @return {int} 返回值 >= 0 表示剩余的具有相同时间截的定时任务数，
	 *  返回 -1 表示该定时任务不存在
	 */
	int del(T* o)
	{
		for (typename std::vector<T*>::iterator it = objs_.begin();
			it != objs_.end(); ++it) {

			if (*it == o) {
				objs_.erase(it);
				return (int) objs_.size();
			}
		}
		return -1;
	}

	/**
	 * 获取具有相同时间截的所有定时任务集合
	 * @return {std::vector<T*>&}
	 */
	std::vector<T*>& get_objs(void)
	{
		return objs_;
	}

private:
	std::vector<T*> objs_;
	trigger_items_t& items_;
};

/**
 * 定时任务触发管理器，通过本类添加定时任务，该类会将到期的任务进行触发
 * 每个定时任务对象 T 需要实现以下方法，以便于由该触发器触发
 *
 * bool on_trigger(void);		// 定时时间到期时的回调方法，返回值表示
 * 					// 是否需要再次触发该定时任务
 * int get_ttl(void) const;		// 定时任务到达时的时间间隔（毫秒）
 * void set_key(long long key);		// 触发器设置与该定时任务关联的键
 * long long get_key(void) const;	// 获得由 set_key 设置的键
 *
 * 如一个 T 的实例类声明如下：
 * class mytask
 * {
 * public:
 *     mytask(void) {}
 *     ~mytask(void) {}
 *
 *     // @override
 *     bool on_trigger(void)
 *     {
 *         return true;
 *     }
 *
 *     // @override
 *     int get_ttl(void) const
 *     {
 *         return 1000;
 *     }
 *
 *     // @override
 *     void set_key(long long key)
 *     {
 *         key_ = key;
 *     }
 *
 *     // @override
 *     long long get_key(void) const
 *     {
 *         return key_;
 *     }
 *
 * private:
 *     long long key_;
 * };
 */
template <typename T>
class timer_trigger : public noncopyable
{
public:
	typedef std::map<long long, trigger_item<T>*> trigger_items_t;
	typedef typename trigger_items_t::iterator trigger_iter_t;

	timer_trigger(void) {}
	~timer_trigger(void) {}

	/**
	 * 添加一个任务对象
	 * @pararm o {T*}
	 */
	void add(T* o)
	{
		int ttl       = o->get_ttl();
		long long key = get_curr_stamp() + ttl;

		trigger_item<T>* item;
		trigger_iter_t it = items_.find(key);
		if (it == items_.end()) {
			item        = new trigger_item<T>(items_);
			items_[key] = item;
		} else
			item = it->second;
		item->add(o);
		o->set_key(key);
	}

	/**
	 * 删除一个任务对象，内部调用 o->get_key() 方法获得该任务对象的键
	 * @pararm o {T*} 指定将被删除的任务对象
	 * @return {int} >= 0 时表示剩余的任务对象，-1 表示该任务对象不存在
	 */
	int del(T* o)
	{
		long long key     = o->get_key();
		trigger_iter_t it = items_.find(key);

		if (it == items_.end())
			return -1;
		if (it->second->del(o) == 0) {
			delete it->second;
			items_.erase(it);
		}
		return (int) items_.size();
	}

	/**
	 * 触发所有到期的定时任务
	 * @return {long long} 返回下一个将被触发的定时任务的时间截，返回 -1
	 *  表示没有定时任务
	 */
	long long trigger(void)
	{
		long long key = get_curr_stamp();
		std::vector<trigger_item<T>*> items;
		trigger_iter_t iter;
		for (iter = items_.begin(); iter != items_.end();) {
			if (iter->first > key)
				break;

			items.push_back(iter->second);
			items_.erase(iter++);
		}

		for (typename std::vector<trigger_item<T>*>::iterator
			it = items.begin(); it != items.end(); ++it) {

			trigger(*it);
			delete *it;
		}

		iter = items_.begin();
		if (iter == items_.end())
			return -1;
		return iter->first;
	}

private:
	trigger_items_t items_;

	/**
	 * 触发具有相同定时时间截的所有任务
	 * @pararm item {trigger_item<T>*}
	 */
	void trigger(trigger_item<T>* item)
	{
		std::vector<T*>& objs = item->get_objs();
		for (typename std::vector<T*>::iterator it = objs.begin();
			it != objs.end(); ++it) {

			if (!(*it)->on_trigger())
				continue;

			int ttl       = (*it)->get_ttl();
			long long key = get_curr_stamp() + ttl;

			trigger_iter_t iter = items_.find(key);
			if (iter == items_.end()) {
				item        = new trigger_item<T>(items_);
				items_[key] = item;
			} else
				item = iter->second;

			item->add(*it);
			(*it)->set_key(key);
		}
	}
};

/**
 * 定时器管理线程，该线程从 mbox 中获得定时任务，并加入定时任务触发器中，然后
 * 定时从触发器中提取到期的任务并触发
 */
template <typename T>
class thread_trigger : public thread
{
public:
	thread_trigger(void)
	: delay_(100)  // 初始化时的超时等待时间（毫秒）
	, stop_(false) // 是否停止线程
	{
	}

	virtual ~thread_trigger(void) {}

	/**
	 * 添加一个定时任务对象
	 * @pararm o {T*}
	 */
	void add(T* o)
	{
		mbox_.push(o);
	}

	/**
	 * 添加要删除的定时任务对象到临时队列中，然后从定时器中删除之
	 * @pararm o {T*}
	 */
	void del(T* o)
	{
		lock_.lock();
		timer_del_.push_back(o);
		lock_.unlock();
	}

	timer_trigger<T>& get_trigger(void)
	{
		return timer_;
	}

private:
	// @override
	void* run(void)
	{
		while (!stop_) {
			T* o = mbox_.pop(delay_);
			if (o)
				timer_.add(o);

			long long next = timer_.trigger();
			long long curr = get_curr_stamp();
			if (next == -1)
				delay_ = 100;
			else {
				delay_ = next - curr;
				if (delay_ < 0)
					delay_ = 1;
			}

			lock_.lock();
			typename std::vector<T*>::iterator it;
			for (it = timer_del_.begin();
				it != timer_del_.end(); ++it) {

				timer_.del(*it);
			}
			timer_del_.clear();
			lock_.unlock();
		}
		return NULL;
	}

private:
	long long delay_;
	bool stop_;

	timer_trigger<T> timer_;
	mbox<T> mbox_;

	std::vector<T*> timer_del_;
	thread_mutex lock_;
};

} // namespace acl
