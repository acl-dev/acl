#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include "noncopyable.hpp"

namespace acl {

typedef enum {
	BOX_TYPE_MBOX,
	BOX_TYPE_TBOX,
	BOX_TYPE_TBOX_ARRAY,
} box_type_t;

template <typename T>
class box : public noncopyable {
public:
	box() {}
	virtual ~box() {}

	// Pus one message to the queue and notify the waiter.
	virtual bool push(T* o, bool notify_first) = 0;

	// Try to get one message with timed wait.
	virtual T* pop(int wait_m, bool* bound) = 0;

	// Try to get more messages with timed wait.
	virtual size_t pop(std::vector<T*>& out, size_t max, int ms) = 0;

	// If supports transferring null message.
	virtual bool has_null() const = 0;
};

template <typename T>
class box2 : public noncopyable {
public:
	box2() {}
	virtual ~box2() {}

	// Pus one message to the queue and notify the waiter.
	virtual bool push(T o, bool notify_first) = 0;

	// Try to get one message with timed wait.
	virtual bool pop(T& t, int ms) = 0;

	// Try to get more messages with timed wait.
	virtual size_t pop(std::vector<T>& out, size_t max, int ms) = 0;

	// Get the messages' count in queue.
	virtual size_t size() const = 0;

	// If supports transferring null message.
	virtual bool has_null() const = 0;
};

} // namespace acl
