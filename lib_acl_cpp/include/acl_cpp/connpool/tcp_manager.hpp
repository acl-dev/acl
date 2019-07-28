#pragma once
#include "../acl_cpp_define.hpp"
#include "connect_manager.hpp"

namespace acl
{

class connect_pool;

class ACL_CPP_API tcp_manager : public connect_manager
{
public:
	tcp_manager(void);
	virtual ~tcp_manager(void);

protected:
	// @override
	virtual connect_pool* create_pool(const char*, size_t, size_t);
};

} // namespace acl
