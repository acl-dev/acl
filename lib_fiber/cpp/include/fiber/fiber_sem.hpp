#pragma once
#include "fiber_cpp_define.hpp"
#include <list>
#include <cassert>

struct ACL_FIBER_SEM;

namespace acl {

typedef enum {
	fiber_sem_t_sync  = 0,
	fiber_sem_t_async = (1 << 0),
} fiber_sem_attr_t;

class FIBER_CPP_API fiber_sem {
public:
	explicit fiber_sem(int max, fiber_sem_attr_t attr = fiber_sem_t_async);
	~fiber_sem();

	int wait(int milliseconds = -1);
	int trywait();
	int post();

	size_t num() const;

private:
	ACL_FIBER_SEM* sem_;
	fiber_sem(const fiber_sem&);
	const fiber_sem& operator=(const fiber_sem&);
};

class FIBER_CPP_API fiber_sem_guard {
public:
	explicit fiber_sem_guard(fiber_sem& sem) : sem_(sem) {
		(void) sem_.wait();
	}

	~fiber_sem_guard() {
		sem_.post();
	}

private:
	fiber_sem& sem_;

	fiber_sem_guard(const fiber_sem_guard&);
	void operator=(const fiber_sem_guard&);
};

// The base box<T> defined in acl_cpp/stdlib/box.hpp, so you must include
// box.hpp first before including fiber_tbox.hpp
template<typename T>
class fiber_sbox : public box<T> {
public:
	explicit fiber_sbox(bool free_obj = true, bool async = true)
	: sem_(0, async ? fiber_sem_t_async : fiber_sem_t_sync)
	, free_obj_(free_obj) {}

	~fiber_sbox() { clear(free_obj_); }

	// @override
	bool push(T* t, bool dummy = true) {
		(void) dummy;
		sbox_.push_back(t);
		sem_.post();
		return true;
	}

	// @override
	T* pop(int milliseconds, bool* found = NULL) {
		if (sem_.wait(milliseconds) < 0) {
			if (found) {
				*found = false;
			}
			return NULL;
		}

		T* t = sbox_.front();
		sbox_.pop_front();
		if (found) {
			*found = true;
		}
		return t;
	}

	// @override
	size_t pop(std::vector<T*>& out, size_t max, int milliseconds) {
		size_t n = 0;
		while (true) {
			if (sem_.wait(milliseconds) < 0) {
				return n;
			}

			T* t = sbox_.front();
			sbox_.pop_front();
			out.push_back(t);
			n++;
			if (max > 0 && n >= max) {
				return n;
			}
			milliseconds = 0;
		}
	}

	// Old interface.
	T* pop(bool* found = NULL) {
		return pop(-1, found);
	}

	// @override
	bool has_null() const {
		return true;
	}

	// @override
	size_t size() const {
		return sem_.num();
	}

private:
	fiber_sem     sem_;
	std::list<T*> sbox_;
	bool          free_obj_;

	fiber_sbox(const fiber_sbox&);
	void operator=(const fiber_sbox&);

public:
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
	explicit fiber_sbox2(bool async = true)
	: sem_(0, async ? fiber_sem_t_async : fiber_sem_t_sync) {}

	~fiber_sbox2() {}

#if __cplusplus >= 201103L      // Support c++11 ?

	void push(T t) {
		sbox_.emplace_back(std::move(t));
		sem_.post();
	}

	bool pop(T& t, int milliseconds = -1) {
		if (sem_.wait(milliseconds) < 0) {
			return false;
		}

		t = std::move(sbox_.front());
		sbox_.pop_front();
		return true;
	}

#else

	void push(T t) {
		sbox_.push_back(t);
		sem_.post();
	}

	bool pop(T& t, int milliseconds = -1) {
		if (sem_.wait(milliseconds) < 0) {
			return false;
		}

		t = sbox_.front();
		sbox_.pop_front();
		return true;
	}

#endif

	size_t size() const {
		return sem_.num();
	}

private:
	fiber_sem    sem_;
	std::list<T> sbox_;

	fiber_sbox2(const fiber_sbox2&);
	void operator=(const fiber_sbox2&);
};

} // namespace acl
