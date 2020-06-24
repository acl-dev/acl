#pragma once
#include "fiber_cpp_define.hpp"
#include <list>
#include <assert.h>

struct ACL_FIBER_SEM;

namespace acl {

class FIBER_CPP_API fiber_sem
{
public:
	fiber_sem(int max);
	~fiber_sem(void);

	int wait(void);
	int trywait(void);
	int post(void);

private:
	ACL_FIBER_SEM* sem_;
	fiber_sem(const fiber_sem&);
	const fiber_sem& operator=(const fiber_sem&);
};

class FIBER_CPP_API fiber_sem_guard
{
public:
	fiber_sem_guard(fiber_sem& sem) : sem_(sem)
	{
		(void) sem_.wait();
	}

	~fiber_sem_guard(void)
	{
		sem_.post();
	}

private:
	fiber_sem& sem_;

	fiber_sem_guard(const fiber_sem_guard&);
	void operator=(const fiber_sem_guard&);
};

template<typename T>
class fiber_sbox
{
public:
	fiber_sbox(bool free_obj = true)
	: sem_(0), size_(0), free_obj_(free_obj) {}

	~fiber_sbox(void) { clear(free_obj_); }

	void clear(bool free_obj = false)
	{
		if (free_obj) {
			for (typename std::list<T*>::iterator it =
				sbox_.begin(); it != sbox_.end(); ++it) {

				delete *it;
			}
		}
		sbox_.clear();
	}

	void push(T* t)
	{
		sbox_.push_back(t);
		sem_.post();
	}

	T* pop(bool* found = NULL)
	{
		sem_.wait();
		bool found_flag;
		T* t = peek(found_flag);
		assert(found_flag);
		if (found) {
			*found = true;
		}
		return t;
	}

private:
	fiber_sem     sem_;
	std::list<T*> sbox_;
	size_t        size_;
	bool          free_obj_;

	fiber_sbox(const fiber_sbox&);
	void operator=(const fiber_sbox&);

	T* peek(bool& found_flag)
	{
		typename std::list<T*>::iterator it = sbox_.begin();
		if (it == sbox_.end()) {
			found_flag = false;
			return NULL;
		}
		found_flag = true;
		size_--;
		T* t = *it;
		sbox_.erase(it);
		return t;
	}
};

} // namespace acl
