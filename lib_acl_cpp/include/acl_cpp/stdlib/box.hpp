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
	virtual bool push(T* o, bool notify_first = true) = 0;

	// Try to get one message with timed wait.
	virtual T* pop(int wait_ms = -1, bool* bound = NULL) = 0;

	// Try to get more messages with timed wait.
	virtual size_t pop(std::vector<T*>& out, size_t max, int wait_ms = -1) = 0;

	// If supports transferring null message.
	virtual bool has_null() const = 0;
};

} // namespace acl
