#pragma once
#include "../acl_cpp_define.hpp"
#include "../db/db_pool.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)

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

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)
