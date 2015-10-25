#pragma once
#include "acl_cpp/acl_cpp_define.hpp"

namespace acl {

class ACL_CPP_API mysql_conf
{
public:
	mysql_conf(const char* dbaddr, const char* dbname);
	mysql_conf(const mysql_conf& conf);
	~mysql_conf();

	mysql_conf& set_dbuser(const char* dbuser);
	mysql_conf& set_dbpass(const char* dbpass);
	mysql_conf& set_dblimit(size_t dblimit);
	mysql_conf& set_dbflags(unsigned long dbflags);
	mysql_conf& set_auto_commit(bool on);
	mysql_conf& set_conn_timeout(int timeout);
	mysql_conf& set_rw_timeout(int timeout);
	mysql_conf& set_charset(const char* charset);

	const char* get_dbaddr() const
	{
		return dbaddr_;
	}

	const char* get_dbname() const
	{
		return dbname_;
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

	size_t get_dblimit() const
	{
		return dblimit_;
	}

	unsigned long get_dbflags() const
	{
		return dbflags_;
	}

	bool get_auto_commit() const
	{
		return auto_commit_;
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
	char* dbaddr_;          // 数据库监听地址
	char* dbname_;          // 数据库名
	char* dbkey_;           // dbname@dbaddr
	char* dbuser_;          // 数据库账号
	char* dbpass_;          // 数据库账号密码
	char* charset_;         // 连接数据库时的字符集
	size_t dblimit_;        // 数据库连接池连接数上限
	unsigned long dbflags_; // 打开数据库时的标志位
	bool  auto_commit_;     // 是否自动提交修改后的数据
	int   conn_timeout_;    // 连接数据库的超时时间
	int   rw_timeout_;      // 与数据库通信的超时时间
};

} // namespace acl
