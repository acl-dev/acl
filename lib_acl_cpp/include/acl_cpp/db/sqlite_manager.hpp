#pragma once
#include "../acl_cpp_define.hpp"
#include "../connpool/connect_manager.hpp"

#if !defined(ACL_DB_DISABLE)

namespace acl {

class ACL_CPP_API sqlite_manager : public connect_manager
{
public:
	/**
	 * 构造函数
	 * @param charset {const char*} 数据文件名字符集
	 */
	sqlite_manager(const char* charset = "utf-8");
	~sqlite_manager();

	/**
	* @param dbfile {const char*} sqlite 数据库的数据文件
	* @param dblimit {size_t} 数据库连接池最大连接数限制
	* @return {sqlite_manager&}
	 */
	sqlite_manager& add(const char* dbfile, size_t dblimit);

protected:
	/**
	 * 基类 connect_manager 虚函数的实现
	 * @param addr {const char*} 服务器监听地址，格式：ip:port
	 * @param count {size_t} 连接池的大小限制，该值为 0 时没有限制
	 * @param idx {size_t} 该连接池对象在集合中的下标位置(从 0 开始)
	 * @return {connect_pool*} 返回创建的连接池对象
	 */
	connect_pool* create_pool(const char* addr, size_t count, size_t idx);

private:
	// sqlite 数据文件名
	char* dbfile_;
	char* charset_;
	size_t dblimit_;
};

} // namespace acl

#endif // !defined(ACL_DB_DISABLE)
