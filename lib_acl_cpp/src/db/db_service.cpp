#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include <assert.h>
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/ipc/ipc_client.hpp"
#include "acl_cpp/db/db_handle.hpp"
#include "acl_cpp/db/db_sqlite.hpp"
#include "acl_cpp/db/db_service.hpp"
#endif

#if !defined(ACL_DB_DISABLE)

namespace acl
{

struct DB_IPC_DAT 
{
	db_handle* db;
	const db_rows* rows;
	int  affected_rows;
	db_query* query;
};

//////////////////////////////////////////////////////////////////////////
// 工作子线程的处理类

class db_ipc_request : public ipc_request
{
public:
	db_ipc_request(db_handle* db, const char* sql, db_query* query, bool has_res)
	: db_(db)
	, sql_(sql)
	, query_(query)
	, has_res_(has_res)
	{
	}

	~db_ipc_request(void)
	{
	}

protected:
	// 基类 ipc_request 会自动调用此回调处理请求过程
	virtual void run(ipc_client* ipc)
	{
		DB_IPC_DAT data;

		data.db = db_;
		data.query = query_;
		data.rows = NULL;
		data.affected_rows = 0;

		if (!db_->dbopen()) {
			ipc->send_message(DB_ERR_OPEN, &data, sizeof(data));
		} else if (has_res_) {
			if (!db_->sql_select(sql_.c_str())) {
				ipc->send_message(DB_ERR_EXEC_SQL, &data, sizeof(data));
			} else {
				data.rows = db_->get_result();
				ipc->send_message(DB_OK, &data, sizeof(data));
			}
		} else if (!db_->sql_update(sql_.c_str())) {
			ipc->send_message(DB_ERR_EXEC_SQL, &data, sizeof(data));
		} else {
			data.rows = db_->get_result();
			// 修改操作，需要取一下 SQL 操作影响的行数
			data.affected_rows = db_->affect_count();
			ipc->send_message(DB_OK, &data, sizeof(data));
		}
		// 因为本请求对象是动态创建的，所以需要释放
		delete this;
	}

#ifdef ACL_WINDOWS

	// 基类虚接口，使子线程可以在执行完任务后向主线程发送 ACL_WINDOWS 窗口消息

	virtual void run(HWND hWnd)
	{
		DB_IPC_DAT* data = (DB_IPC_DAT*) acl_mymalloc(sizeof(DB_IPC_DAT));

		data->db = db_;
		data->query = query_;
		data->rows = NULL;
		data->affected_rows = 0;

		if (!db_->dbopen()) {
			::PostMessage(hWnd, DB_ERR_OPEN + WM_USER, 0, (LPARAM) data);
		} else if (has_res_) {
			if (!db_->sql_select(sql_.c_str())) {
				::PostMessage(hWnd, DB_ERR_EXEC_SQL + WM_USER, 0, (LPARAM) data);
			} else {
				data->rows = db_->get_result();
				::PostMessage(hWnd, DB_OK + WM_USER, 0, (LPARAM) data);
			}
		} else if (!db_->sql_update(sql_.c_str())) {
			::PostMessage(hWnd, DB_ERR_EXEC_SQL + WM_USER, 0, (LPARAM) data);
		} else {
			data->rows = db_->get_result();
			// 修改操作，需要取一下 SQL 操作影响的行数
			data->affected_rows = db_->affect_count();
			::PostMessage(hWnd, DB_OK + WM_USER, 0, (LPARAM) data);
			//::SendMessage(hWnd, DB_OK + WM_USER, 0, (LPARAM) data);
		}
		// 因为本请求对象是动态创建的，所以需要释放
		delete this;
	}
#endif
private:
	db_handle* db_;
	acl::string sql_;
	db_query* query_;
	bool has_res_;
};

//////////////////////////////////////////////////////////////////////////
// 服务线程与子线程池之间的 IPC 通道类定义

class db_ipc : public ipc_client
{
public:
	db_ipc(db_service* dbs, acl_int64 magic)
	: ipc_client(magic)
	, dbservice_(dbs)
	{
	}

	~db_ipc(void)
	{
	}

	virtual void on_message(int nMsg, void* data, int dlen acl_unused)
	{
		DB_IPC_DAT* dat = (DB_IPC_DAT*) data;
		db_query* query = dat->query;

		switch (nMsg) {
		case DB_OK:
			query->on_ok(dat->rows, dat->affected_rows);
			break;
		case DB_ERR_OPEN:
			query->on_error(DB_ERR_OPEN);
			break;
		case DB_ERR_EXEC_SQL:
			query->on_error(DB_ERR_EXEC_SQL);
			break;
		default:
			break;
		}

		dat->db->free_result();
		dbservice_->push_back(dat->db);
		query->destroy();
	}
protected:
	virtual void on_close(void)
	{
		delete this;
	}
private:
	db_service* dbservice_;
};

//////////////////////////////////////////////////////////////////////////
#ifdef ACL_WINDOWS
#include <process.h>
#endif

db_service::db_service(size_t dblimit /* = 100 */, int nthread /* = 2 */,
	bool win32_gui /* = false */)
: ipc_service(nthread, win32_gui)
, dbsize_(0)
{
	// 当采用线程池方式，则数据库连接池的最大值不应超过线程数
	if (nthread > 1) {
		dblimit_ = (int) dblimit > nthread ? nthread : dblimit;
	} else {
		dblimit_ = dblimit;
	}
#ifdef ACL_WINDOWS
	magic_ = _getpid() + time(NULL);
#else
	magic_ = getpid() + time(NULL);
#endif
}

db_service::~db_service(void)
{
	std::list<db_handle*>::iterator it = dbpool_.begin();
	for (; it != dbpool_.end(); ++it) {
		delete (*it);
	}
}

void db_service::on_accept(acl::aio_socket_stream* client)
{
	ACL_SOCKET fd = client->get_socket();
	// 在此处设置服务端接收到的套接口的 SO_LINGER 选项，
	// 以保证在套接口关闭后其资源能得到立刻释放，虽然一般
	// 而言设置此选项会有危害，但考虑到服务端只有接收到
	// 完整的客户端数据后才会调用关闭操作，所以应该不会
	// 造成数据发送不全的问题，切记，不应在客户端的关闭
	// 操作中设置 SO_LINGER 选项，以防数据未完整发送
	// 在服务端设置接收连接的 SO_LINGER 选项，有助于操作
	// 系统快速回收套接口资源
	acl_tcp_so_linger(fd, 1, 0);

	ipc_client* ipc = NEW db_ipc(this, magic_);
	ipc->open(client);

	// 添加服务线程的消息处理

	ipc->append_message(DB_OK);
	ipc->append_message(DB_ERR_OPEN);
	ipc->append_message(DB_ERR_EXEC_SQL);

	// 异步等待消息
	ipc->wait();
}

#ifdef ACL_WINDOWS

void db_service::win32_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	DB_IPC_DAT* dat = NULL;
	db_query* query = NULL;

	switch (msg - WM_USER) {
	case DB_OK:
		dat = (DB_IPC_DAT*) lParam;
		query = dat->query;
		query->on_ok(dat->rows, dat->affected_rows);
		break;
	case DB_ERR_OPEN:
		dat = (DB_IPC_DAT*) lParam;
		query = dat->query;
		query->on_error(DB_ERR_OPEN);
		break;
	case DB_ERR_EXEC_SQL:
		dat = (DB_IPC_DAT*) lParam;
		query = dat->query;
		query->on_error(DB_ERR_EXEC_SQL);
		break;
	default:
		break;
	}

	if (dat) {
		dat->db->free_result();
		push_back(dat->db);
		query->destroy();

		// 在采用 ACL_WINDOWS 消息时该对象空间是动态分配的，所以需要释放
		acl_myfree(dat);
	}
}

#endif

void db_service::sql_select(const char* sql, db_query* query)
{
	assert(sql && *sql);

	db_handle* db;

	std::list<db_handle*>::iterator it = dbpool_.begin();
	if (it == dbpool_.end()) {
		db = db_create();
	} else {
		db = *it;
		dbpool_.erase(it);
	}

	// 创建子线程的请求对象
	db_ipc_request* ipc_req = NEW db_ipc_request(db, sql, query, true);

	// 调用基类 ipc_service 请求过程
	request(ipc_req);
}

void db_service::sql_update(const char* sql, db_query* query)
{
	assert(sql && *sql);

	db_handle* db;

	std::list<db_handle*>::iterator it = dbpool_.begin();
	if (it == dbpool_.end()) {
		db = db_create();
	} else {
		db = *it;
		dbpool_.erase(it);
	}

	// 创建子线程的请求对象
	db_ipc_request* ipc_req = NEW db_ipc_request(db, sql, query, false);

	// 调用基类 ipc_service 请求过程
	request(ipc_req);
}

void db_service::push_back(db_handle* db)
{
	if (dbsize_ >= dblimit_) {
		delete db;
	} else {
		dbsize_++;
		dbpool_.push_back(db);
	}
}

} // namespace acl

#endif // !defined(ACL_DB_DISABLE)
