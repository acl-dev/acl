#pragma once
#include "../acl_cpp_define.hpp"
#include "istream.hpp"

namespace acl {

/**
 * Standard input stream, objects of this class can only perform read operations
 */

class ACL_CPP_API stdin_stream : public istream {
public:
	stdin_stream();
	~stdin_stream();
};

} // namespace acl

