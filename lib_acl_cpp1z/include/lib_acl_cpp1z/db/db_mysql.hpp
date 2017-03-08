#pragma once
#include "acl_cpp/db/db_mysql.hpp"
#include "lib_acl_cpp1z/reflection/reflection.hpp"
namespace acl
{
	namespace lz
	{
		class db_mysql : public acl::db_mysql
		{
		public:
			/**
			* 构造函数方式一
			* @param dbaddr {const char*} 数据库监听地址，可以为 TCP 套接口或在 UNIX
			*  平台下的域套接口，格式如：127.0.0.1:3306，或 /tmp/mysql.sock
			* @param dbname {const char*} 数据库名称，非 NULL
			* @param dbuser {const char*} 连接数据库时的用户名
			* @param dbpass {const char*} 连接数据库时的用户密码
			* @param dbflags {unsigned long} 连接 MYSQL 时的标志位
			* @param auto_commit {bool} 当对数据库进行修改时是否自动提交事务
			* @param conn_timeout {int} 连接数据库的超时时间（秒）
			* @param rw_timeout {int} 进行数据库操作时的超时时间（秒）
			* @param charset {const char*} 连接数据库时的本地字符集（gbk, utf8, ...）
			*/

			db_mysql(const char* dbaddr, const char* dbname,
				const char* dbuser, const char* dbpass,
				unsigned long dbflags = 0, bool auto_commit = true,
				int conn_timeout = 60, int rw_timeout = 60,
				const char* charset = "utf8")
				:acl::db_mysql(
					dbaddr, 
					dbname, 
					dbuser, 
					dbpass, 
					dbflags, 
					auto_commit, 
					conn_timeout, 
					rw_timeout, 
					charset)
			{

			}

			/**
			* 构造函数方式二：使用参数配置类对象进行构造
			* @param conf {const mysql_conf&} mysql 数据库连接配置类对象
			*/
			db_mysql(const mysql_conf& conf)
				:acl::db_mysql(conf)
			{

			}

			template<typename T, typename... Args>
			std::enable_if_t<(sizeof...(Args) == 0), std::pair<bool, std::vector<T>>> select(const char *sql)
			{
				std::vector<T> v;

				if (!sql_select(sql))
					return{ false, {} };
				for (size_t i = 0; i < length(); i++)
				{
					T t;
					const acl::db_row* row = (*this)[i];
					for_each(t, [&](auto& item, size_t I, bool is_last)
					{
						const char * row_name = get_name<T>(I);
						const char * value = (*row)[row_name];
						if (!value)
							return;
						assign(item, value);
					});
					v.push_back(t);
				}
				return{ true, v };
			}
		private:
			void assign(int &item, const char *value)
			{
				item = std::strtol(value, 0, 10);
			}
			void assign(unsigned int &item, const char *value)
			{
				item = std::strtoul(value, 0, 10);
			}
			void assign(float &item, const char *value)
			{
				item = std::strtof(value, 0);
			}

			void assign(double &item, const char *value)
			{
				item = std::strtod(value, 0);
			}

			void assign(acl::string &item, const char *value)
			{
				item.append(value);
			}

			void assign(std::string &item, const char *value)
			{
				item.append(value);
			}
		};
	}
}
