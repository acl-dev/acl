#pragma once
#include "../acl_cpp_define.hpp"
#include "../connpool/connect_manager.hpp"

#if !defined(ACL_DB_DISABLE)

namespace acl {

class ACL_CPP_API sqlite_manager : public connect_manager {
public:
	/**
	 * Constructor.
	 * @param charset {const char*} Database file character set.
	 */
	sqlite_manager(const char* charset = "utf-8");
	~sqlite_manager();

	/**
	* @param dbfile {const char*} sqlite database file name.
	* @param dblimit {size_t} Database connection pool maximum count limit.
	* @return {sqlite_manager&}
	 */
	sqlite_manager& add(const char* dbfile, size_t dblimit);

protected:
	/**
	 * Implementation of connect_manager virtual function.
	 * @param addr {const char*} Database connection address, format: ip:port.
	 * @param count {size_t} Connection pool size limit. When value is 0, there is
	 * no limit.
	 * @param idx {size_t} This connection pool object's subscript position in
	 * collection (starting from 0).
	 * @return {connect_pool*} Returns newly created connection pool object.
	 */
	connect_pool* create_pool(const char* addr, size_t count, size_t idx);

private:
	// sqlite database file name.
	char*  dbfile_;
	char*  charset_;
	size_t dblimit_;
};

} // namespace acl

#endif // !defined(ACL_DB_DISABLE)
