#pragma once
#include "../acl_cpp_define.hpp"
#include "ostream.hpp"

namespace acl {

/**
 * 标准输出流，该类对象仅能进行写操作
 */

class ACL_CPP_API stdout_stream : public ostream
{
public:
	stdout_stream(void);
	~stdout_stream(void);
};

} // namespace acl
