#pragma once
#include "../acl_cpp_define.hpp"
#include "ostream.hpp"

namespace acl {

/**
 * Standard output stream, objects of this class can only perform write operations
 */
class ACL_CPP_API stdout_stream : public ostream {
public:
	stdout_stream();
	~stdout_stream();
};

} // namespace acl

