#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"

#if !defined(ACL_DB_DISABLE)

namespace acl
{

class ACL_CPP_API db_cursor : public noncopyable
{
public:
	db_cursor(void) {}
	virtual ~db_cursor(void) {}
};

}

#endif // !defined(ACL_DB_DISABLE)
