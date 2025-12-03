#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/db/mysql_conf.hpp"
#endif

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)

namespace acl {

mysql_conf::mysql_conf(const char* addr, const char* name)
{
	acl_assert(addr && *addr);
	acl_assert(name && *name);

	// µÿ÷∑∏Ò Ω£∫[dbname@]dbaddr
	const char* ptr = strchr(addr, '@');
	if (ptr != NULL) {
		ptr++;
	} else {
		ptr = addr;
	}
	acl_assert(*ptr);

	dbaddr = ptr;
	dbname = name;

	acl::string buf;
	buf.format("%s@%s", name, ptr);
	buf.lower();
	dbkey  = buf.c_str();

	dblimit      = 64;
	dbflags      = 0;
	auto_commit  = true;
	conn_timeout = 60;
	rw_timeout   = 60;
}

mysql_conf::mysql_conf(const mysql_conf& conf)
{
	dbaddr       = conf.dbaddr;
	dbname       = conf.dbname;
	dbkey        = conf.dbkey;

	dbuser       = conf.dbuser;
	dbpass       = conf.dbpass;
	charset      = conf.charset;
	dblimit      = conf.dblimit;
	dbflags      = conf.dbflags;
	auto_commit  = conf.auto_commit;
	conn_timeout = conf.conn_timeout;
	rw_timeout   = conf.rw_timeout;
	sslcrt       = conf.sslcrt;
	sslkey       = conf.sslkey;
	sslca        = conf.sslca;
	sslcapath    = conf.sslcapath;
	sslcipher    = conf.sslcipher;
}

mysql_conf::~mysql_conf()
{
}

mysql_conf& mysql_conf::set_dbuser(const char* user)
{
	if (user && *user) {
		dbuser = user;
	}
	return *this;
}

mysql_conf& mysql_conf::set_dbpass(const char* pass)
{
	if (pass && *pass) {
		dbpass = pass;
	}
	return *this;
}

mysql_conf& mysql_conf::set_charset(const char* ptr)
{
	if (ptr && *ptr) {
		charset = ptr;
	}
	return *this;
}

mysql_conf& mysql_conf::set_dblimit(size_t n)
{
	dblimit = n;
	return *this;
}

mysql_conf& mysql_conf::set_dbflags(unsigned long flags)
{
	dbflags = flags;
	return *this;
}

mysql_conf& mysql_conf::set_auto_commit(bool on)
{
	auto_commit = on;
	return *this;
}

mysql_conf& mysql_conf::set_conn_timeout(int timeout)
{
	conn_timeout = timeout;
	return *this;
}

mysql_conf& mysql_conf::set_rw_timeout(int timeout)
{
	rw_timeout = timeout;
	return *this;
}

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)
