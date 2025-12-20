#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"
#include <list>

#ifndef ACL_CLIENT_ONLY

struct ACL_HTABLE;

namespace acl {

class hsclient;
class locker;

class ACL_CPP_API hspool : public noncopyable {
public:
	/**
	 * Constructor
	 * @param addr_rw {const char*} Listening address of handlersocket plugin on
	 * MySQL,
	 * format: ip:port. Note: This address is the read-write address of
	 * handlersocket
	 * @param addr_rd {const char*} Listening address of handlersocket plugin on
	 * MySQL,
	 * format: ip:port. Note: This address is the read-only address of
	 * handlersocket
	 * @param cache_enable {bool} Whether to automatically enable row object
	 * caching mechanism internally
	 * @param retry_enable {bool} Whether to retry when error occurs due to network
	 * reasons
	 */
	hspool(const char* addr_rw, const char* addr_rd = NULL,
		bool cache_enable = true, bool retry_enable = true);

	~hspool();

	/**
	 * Get connection object from connection pool
	 * @param dbn {const char*} Database name
	 * @param tbl {const char*} Database table name
	 * @param idx {const char*} Index field name
	 * @param flds {const char*} Set of data field names to open, format is
	 * field names separated by delimiters ",; \t", e.g.:
	 * user_id,user_name,user_mail
	 * @param readonly {bool} Whether to open in read-only mode only
	 */
	hsclient* peek(const char* dbn, const char* tbl,
		const char* idx, const char* flds, bool readonly = false);

	/**
	 * Return connection object
	 * @param client {hsclient*}
	 */
	void put(hsclient* client);

private:
	char* addr_rw_;
	char* addr_rd_;
	bool cache_enable_;
	bool retry_enable_;
	std::list<hsclient*> pool_;
	locker* locker_;
};

}  // namespace acl

#endif // ACL_CLIENT_ONLY

