#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"
#include "../db/db_service.hpp"

#if !defined(ACL_DB_DISABLE)

namespace acl {

class ACL_CPP_API db_service_sqlite : public db_service
{
public:
	/**
	 * 当为 sqlite 数据库时的构造函数
	 * @param dbname {const char*} 数据库名
	 * @param dbfile {const char*} 数据库文件名(对于一些内嵌数据库有用)
	 * @param dblimit {size_t} 数据库连接池的个数限制
	 * @param nthread {int} 子线程池的最大线程数
	 * @param win32_gui {bool} 是否是窗口类的消息，如果是，则内部的
	 *  通讯模式自动设置为基于 _WIN32 的消息，否则依然采用通用的套接
	 *  口通讯方式
	 */
	db_service_sqlite(const char* dbname, const char* dbfile,
		size_t dblimit = 100, int nthread = 2, bool win32_gui = false);
	~db_service_sqlite();

private:
	// 数据库名称
	string dbname_;
	// sqlite 数据库文件名
	string dbfile_;

	// 基类纯虚函数
	virtual db_handle* db_create(void);
};

}

#endif // !defined(ACL_DB_DISABLE)
