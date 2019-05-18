#pragma once
#include "../acl_cpp_define.hpp"
#include "../db/db_pool.hpp"

#ifndef ACL_CLIENT_ONLY

namespace acl {

class db_handle;
class pgsql_conf;

class ACL_CPP_API pgsql_pool : public db_pool
{
public:
	pgsql_pool(const pgsql_conf& conf);
	~pgsql_pool(void);

protected:
	// @override
	connect_client* create_connect(void);

private:
	pgsql_conf* conf_;
};

} // namespace acl

#endif // ACL_CLIENT_ONLY
