#pragma once
#include "../acl_cpp_define.hpp"
#include "istream.hpp"

namespace acl {

/**
 * 标准输入流，该类对象仅能进行读操作
 */

class ACL_CPP_API stdin_stream : public istream
{
public:
	stdin_stream(void);
	~stdin_stream(void);
};

} // namespace acl
