#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"
#include "../db/db_handle.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)

typedef struct pg_conn PGconn;

namespace acl {

class pgsql_conf;

class ACL_CPP_API db_pgsql : public db_handle {
public:
	db_pgsql(const pgsql_conf& conf);
	~db_pgsql(void);

	static bool load(void);

	/********************************************************************/
	/*         The following are virtual interfaces of base class db_handle                            */
	/********************************************************************/

	const char* dbtype(void) const;

	/**
	 * @override
	 */
	int get_errno(void) const;

	/**
	 * @override
	 */
	const char* get_error(void) const;

	/**
	 * @override
	 */
	bool dbopen(const char* charset = NULL);

	/**
	 * @override
	 */
	bool is_opened(void) const;

	/**
	 * @override
	 */
	bool close(void);

	/**
	 * @override
	 */
	bool tbl_exists(const char* tbl_name);

	/**
	 * @override
	 */
	bool sql_select(const char* sql, db_rows* result = NULL);

	/**
	 * @override
	 */
	bool sql_update(const char* sql);

	/**
	 * @override
	 */
	int affect_count(void) const;

	/**
	 * @override
	 */
	bool begin_transaction(void);

	/**
	 * @override
	 */
	bool commit(void);

	/**
	 * @override
	 */
	bool rollback(void);

private:
	char*   dbaddr_;  // Database listening address
	char*   dbname_;  // Database name
	char*   dbuser_;  // Database account
	char*   dbpass_;  // Database account password
	string  charset_; // Character set used when connecting to database

	int     conn_timeout_;
	int     rw_timeout_;

	PGconn* conn_;
	int     affect_count_;

	void sane_pgsql_init(const char* dbaddr, const char* dbname,
		const char* dbuser, const char* dbpass, int conn_timeout,
		int rw_timeout, const char* charset);
	void* sane_pgsql_query(const char* sql);
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)

