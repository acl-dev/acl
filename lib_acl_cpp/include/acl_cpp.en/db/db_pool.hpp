#pragma once
#include "../acl_cpp_define.hpp"
#include <list>
#include "../db/db_handle.hpp"
#include "../connpool/connect_pool.hpp"

#if !defined(ACL_DB_DISABLE)

namespace acl {

class db_handle;
class locker;

class ACL_CPP_API db_pool : public connect_pool {
public:
	/**
	 * Database constructor
	 * @param dbaddr {const char*} Database address
	 * @param count {size_t} Maximum connection limit for connection pool
	 * @param idx {size_t} Index position of this connection pool object in the
	 * collection (starting from 0)
	 */
	db_pool(const char* dbaddr, size_t count, size_t idx = 0);
	virtual ~db_pool() {};

	/**
	 * Get a database object from the database connection pool and require opening
	 * the database connection, i.e., users don't need to explicitly call
	 * db_handle::open again; After use, must call db_pool->put(db_handle*) to
	 * return the connection to the database connection pool.
	 * The connection handle obtained by this function cannot be deleted, otherwise
	 * it will cause errors in the connection pool's internal counter
	 * @return {db_handle*} Database connection object, returns NULL on error
	 */
	db_handle* peek_open();

	/**
	 * Get the maximum connection limit of the current database connection pool
	 * @return {size_t}
	 */
	size_t get_dblimit() const {
		return get_max();
	}

	/**
	 * Get the current connection count of the current database connection pool
	 * @return {size_t}
	 */
	size_t get_dbcount() const {
		return get_count();
	}

	/**
	 * Set the lifetime (seconds) of idle connections in the database connection
	 * pool
	 * @param ttl {int} Lifetime (seconds)
	 */
	void set_idle(int ttl) {
		set_idle_ttl(ttl);
	}
};

class ACL_CPP_API db_guard : public connect_guard
{
public:
	db_guard(db_pool& pool) : connect_guard(pool) {}
	~db_guard(void);
};

} // namespace acl

#endif // !defined(ACL_DB_DISABLE)

