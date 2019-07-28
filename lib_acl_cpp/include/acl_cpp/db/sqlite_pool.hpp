#pragma once
#include "../acl_cpp_define.hpp"
#include "../db/db_pool.hpp"

#if !defined(ACL_DB_DISABLE)

namespace acl {

class db_handle;

class ACL_CPP_API sqlite_pool : public db_pool
{
public:
	/**
	 * 构造函数
	 * @param dbfile {const char*} sqlite 数据库的数据文件
	 * @param dblimit {size_t} 数据库连接池最大连接数限制
	 * @param charset {const char*} 数据文件名字符集
	 */
	sqlite_pool(const char* dbfile, size_t dblimit = 64,
		const char* charset = "utf-8");
	~sqlite_pool();

protected:
	// 基类 connect_pool 纯虚函数：创建数据库连接句柄
	connect_client* create_connect();

private:
	// sqlite 数据文件名
	char* dbfile_;
	// sqlite 数据文件名的字符集
	char* charset_;
};

} // namespace acl

#endif // !defined(ACL_DB_DISABLE)
