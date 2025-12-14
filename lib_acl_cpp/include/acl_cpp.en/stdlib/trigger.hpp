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
 * Collection of timer tasks with the same timestamp
 */
template <typename T>
class trigger_item : public noncopyable {
public:
	typedef std::map<long long, trigger_item<T>*> trigger_items_t;

	trigger_item(trigger_items_t& items) : items_(items) {}
	~trigger_item() {}

	/**
	 * Add a timer task
	 * @pararm o {T*}
	 */
	void add(T* o) {
		objs_.push_back(o);
	}

	/**
	 * Delete a timer task with the same timestamp
	 * @pararm o {T*}
	 * @return {int} Return value >= 0 indicates remaining number of timer tasks with the same timestamp,
	 *  returns -1 indicates this timer task does not exist
	 */
	int del(T* o) {
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
	 * Get all timer task collection with the same timestamp
	 * @return {std::vector<T*>&}
	 */
	std::vector<T*>& get_objs() {
		return objs_;
	}

private:
	std::vector<T*> objs_;
	trigger_items_t& items_;
};

/**
 * Timer task trigger manager. Add timer tasks through this class, and this class will trigger expired tasks.
 * Each timer task object T needs to implement the following methods so that it can be triggered by this trigger:
 *
 * bool on_trigger();		// Callback method when timer expires. Return value indicates
 * 					// whether this timer task needs to be triggered again
 * int get_ttl() const;		// Time interval (milliseconds) when timer task arrives
 * void set_key(long long key);		// Trigger sets key associated with this timer task
 * long long get_key() const;	// Get key set by set_key
 *
 * Example of a T instance class declaration:
 * class mytask
 * {
 * public:
 *     mytask() {}
 *     ~mytask() {}
 *
 *     // @override
 *     bool on_trigger() {
 *         return true;
 *     }
 *
 *     // @override
 *     int get_ttl() const {
 *         return 1000;
 *     }
 *
 *     // @override
 *     void set_key(long long key) {
 *         key_ = key;
 *     }
 *
 *     // @override
 *     long long get_key() const {
 *         return key_;
 *     }
 *
 * private:
 *     long long key_;
 * };
 */
template <typename T>
class timer_trigger : public noncopyable {
public:
	typedef std::map<long long, trigger_item<T>*> trigger_items_t;
	typedef typename trigger_items_t::iterator trigger_iter_t;

	timer_trigger() {}
	~timer_trigger() {}

	/**
	 * Add a task object
	 * @pararm o {T*}
	 */
	void add(T* o) {
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
	 * Delete a task object. Internally calls o->get_key() method to get the key of this task object
	 * @pararm o {T*} Specified task object to be deleted
	 * @return {int} >= 0 indicates remaining task objects, -1 indicates this task object does not exist
	 */
	int del(T* o) {
		long long key     = o->get_key();
		trigger_iter_t it = items_.find(key);

		if (it == items_.end()) {
			return -1;
		}
		if (it->second->del(o) == 0) {
			delete it->second;
			items_.erase(it);
		}
		return (int) items_.size();
	}

	/**
	 * Trigger all expired timer tasks
	 * @return {long long} Returns timestamp of the next timer task to be triggered. Returns -1
	 *  indicates there are no timer tasks
	 */
	long long trigger() {
		long long key = get_curr_stamp();
		std::vector<trigger_item<T>*> items;
		trigger_iter_t iter;
		for (iter = items_.begin(); iter != items_.end();) {
			if (iter->first > key) {
				break;
			}

			items.push_back(iter->second);
			items_.erase(iter++);
		}

		for (typename std::vector<trigger_item<T>*>::iterator
			it = items.begin(); it != items.end(); ++it) {

			trigger(*it);
			delete *it;
		}

		iter = items_.begin();
		if (iter == items_.end()) {
			return -1;
		}
		return iter->first;
	}

private:
	trigger_items_t items_;

	/**
	 * Trigger all tasks with the same timer timestamp
	 * @pararm item {trigger_item<T>*}
	 */
	void trigger(trigger_item<T>* item) {
		std::vector<T*>& objs = item->get_objs();
		for (typename std::vector<T*>::iterator it = objs.begin();
			it != objs.end(); ++it) {

			if (!(*it)->on_trigger()) {
				continue;
			}

			int ttl       = (*it)->get_ttl();
			long long key = get_curr_stamp() + ttl;

			trigger_iter_t iter = items_.find(key);
			if (iter == items_.end()) {
				item        = new trigger_item<T>(items_);
				items_[key] = item;
			} else {
				item = iter->second;
			}

			item->add(*it);
			(*it)->set_key(key);
		}
	}
};

/**
 * Timer management thread. This thread gets timer tasks from mbox, adds them to timer task trigger, then
 * periodically extracts expired tasks from trigger and triggers them
 */
template <typename T>
class thread_trigger : public thread {
public:
	thread_trigger()
	: delay_(100)  // Timeout wait time during initialization (milliseconds)
	, stop_(false) // Whether to stop thread
	{
	}

	virtual ~thread_trigger() {}

	/**
	 * Add a timer task object
	 * @pararm o {T*}
	 */
	void add(T* o) {
		mbox_.push(o);
	}

	/**
	 * Add timer task object to be deleted to temporary queue, then delete it from timer
	 * @pararm o {T*}
	 */
	void del(T* o) {
		lock_.lock();
		timer_del_.push_back(o);
		lock_.unlock();
	}

	timer_trigger<T>& get_trigger() {
		return timer_;
	}

private:
	// @override
	void* run() {
		while (!stop_) {
			T* o = mbox_.pop(delay_);
			if (o) {
				timer_.add(o);
			}

			long long next = timer_.trigger();
			long long curr = get_curr_stamp();
			if (next == -1) {
				delay_ = 100;
			} else {
				delay_ = next - curr;
				if (delay_ < 0) {
					delay_ = 1;
				}
			}

			lock_.lock();
			typename std::vector<T*>::iterator it;
			for (it = timer_del_.begin(); it != timer_del_.end(); ++it) {
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

