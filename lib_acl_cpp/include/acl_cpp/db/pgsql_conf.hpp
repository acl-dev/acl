#pragma once
#include "acl_cpp/acl_cpp_define.hpp"

namespace acl {

class ACL_CPP_API pgsql_conf
{
public:
	pgsql_conf(const char* dbaddr, const char* dbname);
	pgsql_conf(const pgsql_conf& conf);
	~pgsql_conf(void);

	pgsql_conf& set_dbuser(const char* dbuser);
	pgsql_conf& set_dbpass(const char* dbpass);
	pgsql_conf& set_dblimit(size_t dblimit);
	pgsql_conf& set_conn_timeout(int timeout);
	pgsql_conf& set_rw_timeout(int timeout);
	pgsql_conf& set_charset(const char* charset);

	const char* get_dbaddr() const
	{
		return dbaddr_;
	}

	const char* get_dbname() const
	{
		return dbname_;
	}

	size_t get_dblimit() const
	{
		return dblimit_;
	}

	const char* get_dbkey() const
	{
		return dbkey_;
	}

	const char* get_dbuser() const
	{
		return dbuser_;
	}

	const char* get_dbpass() const
	{
		return dbpass_;
	}

	int get_conn_timeout() const
	{
		return conn_timeout_;
	}

	int get_rw_timeout() const
	{
		return rw_timeout_;
	}

	const char* get_charset() const
	{
		return charset_;
	}

private:
	char* dbaddr_;
	char* dbname_;
	char* dbkey_;
	char* dbuser_;
	char* dbpass_;
	char* charset_;
	size_t dblimit_;
	int   conn_timeout_;
	int   rw_timeout_;
};

} // namespace acl
