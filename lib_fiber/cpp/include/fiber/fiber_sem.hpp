#pragma once
#include "fiber_cpp_define.hpp"
#include <list>
#include <assert.h>

struct ACL_FIBER_SEM;

namespace acl {

typedef enum {
	fiber_sem_t_def   = 0,
	fiber_sem_t_async = (1 << 0),
} fiber_sem_attr_t;

class FIBER_CPP_API fiber_sem {
public:
	fiber_sem(int max, fiber_sem_attr_t attr = fiber_sem_t_def);
	~fiber_sem(void);

	int wait(void);
	int trywait(void);
	int post(void);

	size_t num(void) const;

private:
	ACL_FIBER_SEM* sem_;
	fiber_sem(const fiber_sem&);
	const fiber_sem& operator=(const fiber_sem&);
};

class FIBER_CPP_API fiber_sem_guard {
public:
	fiber_sem_guard(fiber_sem& sem) : sem_(sem) {
		(void) sem_.wait();
	}

	~fiber_sem_guard(void) {
		sem_.post();
	}

private:
	fiber_sem& sem_;

	fiber_sem_guard(const fiber_sem_guard&);
	void operator=(const fiber_sem_guard&);
};

template<typename T>
class fiber_sbox {
public:
	fiber_sbox(bool free_obj = true) : sem_(0), free_obj_(free_obj) {}

	~fiber_sbox(void) { clear(free_obj_); }

	void push(T* t) {
		sbox_.push_back(t);
		sem_.post();
	}

	T* pop(void) {
		sem_.wait();
		T* t = sbox_.front();
		sbox_.pop_front();
		return t;
	}

private:
	fiber_sem     sem_;
	std::list<T*> sbox_;
	bool          free_obj_;

	fiber_sbox(const fiber_sbox&);
	void operator=(const fiber_sbox&);

	void clear(bool free_obj = false) {
		if (free_obj) {
			for (typename std::list<T*>::iterator it =
				sbox_.begin(); it != sbox_.end(); ++it) {

				delete *it;
			}
		}
		sbox_.clear();
	}
};

template<typename T>
class fiber_sbox2 {
public:
	fiber_sbox2(void): sem_(0) {}
	~fiber_sbox2(void) {}

	void push(T t) {
		sbox_.push_back(t);
		sem_.post();
	}

	T pop(void) {
		sem_.wait();
		T t = sbox_.front();
		sbox_.pop_front();
		return t;
	}

	size_t size(void) const {
		return sem_.num();
	}

private:
	fiber_sem    sem_;
	std::list<T> sbox_;

	fiber_sbox2(const fiber_sbox2&);
	void operator=(const fiber_sbox2&);
};

} // namespace acl
