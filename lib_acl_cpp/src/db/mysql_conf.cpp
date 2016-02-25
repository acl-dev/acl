#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/db/mysql_conf.hpp"
#endif

namespace acl {

mysql_conf::mysql_conf(const char* dbaddr, const char* dbname)
{
	acl_assert(dbaddr && *dbaddr);
	acl_assert(dbname && *dbname);

	// µÿ÷∑∏Ò Ω£∫[dbname@]dbaddr
	const char* ptr = strchr(dbaddr, '@');
	if (ptr != NULL)
		ptr++;
	else
		ptr = dbaddr;
	acl_assert(*ptr);

	dbaddr_ = acl_mystrdup(ptr);
	dbname_ = acl_mystrdup(dbname);
	dbkey_  = acl_concatenate(dbname, "@", ptr, NULL);
	acl_lowercase(dbkey_);

	dbuser_ = NULL;
	dbpass_ = NULL;
	charset_ = NULL;
	dblimit_ = 64;
	dbflags_ = 0;
	auto_commit_ = true;
	conn_timeout_ = 60;
	rw_timeout_ = 60;
}

mysql_conf::mysql_conf(const mysql_conf& conf)
{
	dbaddr_ = acl_mystrdup(conf.get_dbaddr());
	dbname_ = acl_mystrdup(conf.get_dbname());
	dbkey_  = acl_mystrdup(conf.get_dbkey());

	const char* ptr = conf.get_dbuser();
	if (ptr && *ptr)
		dbuser_ = acl_mystrdup(ptr);
	else
		dbuser_ = NULL;
	ptr = conf.get_dbpass();
	if (ptr && *ptr)
		dbpass_ = acl_mystrdup(ptr);
	else
		dbpass_ = NULL;
	ptr = conf.get_charset();
	if (ptr && *ptr)
		charset_ = acl_mystrdup(ptr);
	else
		charset_ = NULL;
	dblimit_ = conf.get_dblimit();
	dbflags_ = conf.get_dbflags();
	auto_commit_ = conf.get_auto_commit();
	conn_timeout_ = conf.get_conn_timeout();
	rw_timeout_ = conf.get_rw_timeout();
}

mysql_conf::~mysql_conf()
{
	acl_myfree(dbaddr_);
	acl_myfree(dbname_);
	acl_myfree(dbkey_);
	if (dbuser_)
		acl_myfree(dbuser_);
	if (dbpass_)
		acl_myfree(dbpass_);
	if (charset_)
		acl_myfree(charset_);
}

mysql_conf& mysql_conf::set_dbuser(const char* dbuser)
{
	if (dbuser == NULL || *dbuser == 0)
		return *this;
	if (dbuser_)
		acl_myfree(dbuser_);
	dbuser_ = acl_mystrdup(dbuser);
	return *this;
}

mysql_conf& mysql_conf::set_dbpass(const char* dbpass)
{
	if (dbpass == NULL || *dbpass == 0)
		return *this;
	if (dbpass_)
		acl_myfree(dbpass_);
	dbpass_ = acl_mystrdup(dbpass);
	return *this;
}

mysql_conf& mysql_conf::set_charset(const char* charset)
{
	if (charset == NULL || *charset == 0)
		return *this;
	if (charset_)
		acl_myfree(charset_);
	charset_ = acl_mystrdup(charset);
	return *this;
}

mysql_conf& mysql_conf::set_dblimit(size_t dblimit)
{
	dblimit_ = dblimit;
	return *this;
}

mysql_conf& mysql_conf::set_dbflags(unsigned long dbflags)
{
	dbflags_ = dbflags;
	return *this;
}

mysql_conf& mysql_conf::set_auto_commit(bool on)
{
	auto_commit_ = on;
	return *this;
}

mysql_conf& mysql_conf::set_conn_timeout(int timeout)
{
	conn_timeout_ = timeout;
	return *this;
}

mysql_conf& mysql_conf::set_rw_timeout(int timeout)
{
	rw_timeout_ = timeout;
	return *this;
}

} // namespace acl
