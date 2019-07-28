#pragma once
#include "../acl_cpp_define.hpp"

namespace acl {

class ACL_CPP_API noncopyable
{
protected:
	noncopyable() {}
	~noncopyable() {}
private:
	noncopyable( const noncopyable& );
	const noncopyable& operator=( const noncopyable& );
};

}  // namespace acl
