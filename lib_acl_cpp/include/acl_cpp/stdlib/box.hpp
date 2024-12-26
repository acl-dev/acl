#pragma once
#include "../acl_cpp_define.hpp"
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

	virtual bool push(T* o, bool notify_first = true) = 0;
	virtual T* pop(int wait_ms = -1, bool* bound = NULL) = 0;
	virtual bool has_null() const = 0;
};

} // namespace acl
