#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"

namespace acl
{

class ACL_CPP_API db_cursor : public noncopyable
{
public:
	db_cursor(void) {}
	virtual ~db_cursor(void) {}
};

}
