#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/db/db_pool.hpp"

namespace acl {

class db_handle;

class ACL_CPP_API sqlite_pool : public acl::db_pool
{
public:
	/**
	 * 构造函数
	 * @param dbfile {const char*} sqlite 数据库的数据文件
	 * @param dblimit {int} 数据库连接池最大连接数限制
	 */
	sqlite_pool(const char* dbfile, int dblimit = 64);
	~sqlite_pool();
protected:
	// 基类虚函数：创建数据库连接句柄
	virtual db_handle* create();
private:
	// sqlite 数据文件名
	char* dbfile_;
};

} // namespace acl
