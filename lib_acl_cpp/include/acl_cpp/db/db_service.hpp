#pragma once
#include "../acl_cpp_define.hpp"
#include <list>
#include "../ipc/ipc_service.hpp"
#include "../stdlib/string.hpp"

#if !defined(ACL_DB_DISABLE)

namespace acl {

typedef enum
{
	DB_OK,
	DB_ERR_OPEN,
	DB_ERR_EXEC_SQL,
} db_status;

//////////////////////////////////////////////////////////////////////////

class db_rows;

class ACL_CPP_API db_query
{
public:
	db_query(void) {}
	virtual ~db_query(void) {}

	virtual void on_error(db_status status) = 0;
	virtual void on_ok(const db_rows* rows, int affected) = 0;

	/**
	 * 当任务处理完毕或出错时，内部处理过程会自动调用 destroy 接口，
	 * 子类可以在该接口内进行一些释放过程，尤其当该对象是动态创建时，
	 * 子类应该在该函数内 delete this 以删除自己，因为该函数最终肯定
	 * 会被调用，所以子类不应在其它地方进行析构操作
	 */
	virtual void destroy(void) {}
protected:
private:
};

//////////////////////////////////////////////////////////////////////////

class db_handle;
class aio_socket_stream;

/**
 * 数据库服务类，该类是一个异步数据库操作管理类，该类对象所在的线程必须是
 * 一个非阻塞的线程过程
 */
class ACL_CPP_API db_service : public ipc_service
{
public:
	/**
	 * 当为 sqlite 数据库时的构造函数
	 * @param dblimit {size_t} 数据库连接池的个数限制
	 * @param nthread {int} 子线程池的最大线程数
	 * @param win32_gui {bool} 是否是窗口类的消息，如果是，则内部的
	 *  通讯模式自动设置为基于 _WIN32 的消息，否则依然采用通用的套接
	 *  口通讯方式
	 */
	db_service(size_t dblimit = 100, int nthread = 2, bool win32_gui = false);
	virtual ~db_service(void);

	/**
	 * 异步执行 SQL 查询语句
	 * @param sql {const char*} 要执行的标准 SQL 语句
	 * @param query {db_query*} 用来接收执行结果的类对象
	 */
	void sql_select(const char* sql, db_query* query);

	/**
	 * 异步执行 SQL 更新语句
	 * @param sql {const char*} 要执行的标准 SQL 语句
	 * @param query {db_query*} 用来接收执行结果的类对象
	 */
	void sql_update(const char* sql, db_query* query);

	/**
	 * 向数据库连接池中添加连接对象
	 * @param db {db_handle*} 数据库连接对象
	 */
	void push_back(db_handle* db);
protected:
	/**
	 * 需要子类实现此函数用来创建数据库对象
	 * @return {db_handle*}
	 */
	virtual db_handle* db_create() = 0;

	/**
	 * 基类虚函数，当有新连接到达时基类回调此函数
	 * @param client {aio_socket_stream*} 接收到的新的客户端连接
	 */
	virtual void on_accept(aio_socket_stream* client);

#if defined(_WIN32) || defined(_WIN64)
	/**
	 * 基类虚函数，当收到来自于子线程的 win32 消息时的回调函数
	 * @param hWnd {HWND} 窗口句柄
	 * @param msg {UINT} 用户自定义消息号
	 * @param wParam {WPARAM} 参数
	 * @param lParam {LPARAM} 参数
	 */
	virtual void win32_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

private:
	// 数据库引擎池
	std::list<db_handle*> dbpool_;

	// 数据库连接池的个数限制
	size_t dblimit_;

	// 当前数据库连接池的个数
	size_t dbsize_;
};

} // namespace acl

#endif // !defined(ACL_DB_DISABLE)
