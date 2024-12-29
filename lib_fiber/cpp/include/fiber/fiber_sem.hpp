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
	explicit fiber_sem(int max, int buf);
	~fiber_sem();

	int wait(int ms = -1);
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
	const fiber_sem_guard& operator=(const fiber_sem_guard&);
};

// The base box<T> defined in acl_cpp/stdlib/box.hpp, so you must include
// box.hpp first before including fiber_tbox.hpp
template<typename T>
class fiber_sbox : public box<T> {
public:
	explicit fiber_sbox(bool free_obj = true, bool async = true)
	: sem_(0, async ? fiber_sem_t_async : fiber_sem_t_sync)
	, free_obj_(free_obj) {}

	explicit fiber_sbox(int buf, bool free_obj = true)
	: sem_(0, buf)
	, free_obj_(free_obj) {}

	~fiber_sbox() { clear(free_obj_); }

	// @override
	bool push(T* t, bool dummy = false) {
		(void) dummy;
		sbox_.push_back(t);
		sem_.post();
		return true;
	}

	// @override
	T* pop(int ms, bool* found = NULL) {
		if (sem_.wait(ms) < 0) {
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
	size_t pop(std::vector<T*>& out, size_t max, int ms) {
		size_t n = 0;
		while (true) {
			if (sem_.wait(ms) < 0) {
				return n;
			}

			T* t = sbox_.front();
			sbox_.pop_front();
			out.push_back(t);
			n++;
			if (max > 0 && n >= max) {
				return n;
			}
			ms = 0;
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
	const fiber_sbox& operator=(const fiber_sbox&);

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
class fiber_sbox2 : public box2<T> {
public:
	explicit fiber_sbox2(bool async = true)
	: sem_(0, async ? fiber_sem_t_async : fiber_sem_t_sync) {}

	explicit fiber_sbox2(int buf)
	: sem_(0, buf) {}

	~fiber_sbox2() {}

	// @override
	bool push(T t, bool dummy = false) {
		(void) dummy;
#if __cplusplus >= 201103L || defined(USE_CPP11)     // Support c++11 ?
		sbox_.emplace_back(std::move(t));
#else
		sbox_.push_back(t);
#endif
		sem_.post();
		return true;
	}

	// @override
	bool pop(T& t, int ms = -1) {
		if (sem_.wait(ms) < 0) {
			return false;
		}

#if __cplusplus >= 201103L || defined(USE_CPP11)     // Support c++11 ?
		t = std::move(sbox_.front());
#else
		t = sbox_.front();
#endif
		sbox_.pop_front();
		return true;
	}

	// @override
	size_t pop(std::vector<T>& out, size_t max, int ms) {
		size_t n = 0;
		while (true) {
			if (sem_.wait(ms) < 0) {
				return n;
			}

			T t = sbox_.front();
#if __cplusplus >= 201103L || defined(USE_CPP11)     // Support c++11 ?
			out.emplace_back(std::move(t));
#else
			out.push_back(t);
#endif
			n++;
			if (max > 0 && n >= max) {
				return n;
			}
			ms = 0;
		}
	}

	// @override
	size_t size() const {
		return sem_.num();
	}

	// @override
	bool has_null() const {
		return true;
	}

private:
	fiber_sem    sem_;
	std::list<T> sbox_;

	fiber_sbox2(const fiber_sbox2&);
	const fiber_sbox2& operator=(const fiber_sbox2&);
};

} // namespace acl
