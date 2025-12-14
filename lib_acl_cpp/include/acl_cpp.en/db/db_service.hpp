#pragma once
#include "../acl_cpp_define.hpp"
#include <list>
#include "../ipc/ipc_service.hpp"
#include "../stdlib/string.hpp"

#if !defined(ACL_DB_DISABLE)

namespace acl {

typedef enum {
	DB_OK,
	DB_ERR_OPEN,
	DB_ERR_EXEC_SQL,
} db_status;

//////////////////////////////////////////////////////////////////////////

class db_rows;

class ACL_CPP_API db_query {
public:
	db_query(void) {}
	virtual ~db_query(void) {}

	virtual void on_error(db_status status) = 0;
	virtual void on_ok(const db_rows* rows, int affected) = 0;

	/**
	 * When task processing is complete or error occurs, internal processing will
	 * automatically call destroy interface. Subclasses can perform some release
	 * process in this interface, especially when this object is dynamically created,
	 * subclasses should delete this in this function to delete themselves, because
	 * this function will definitely be called, so subclasses should not perform
	 * destruction operations elsewhere.
	 */
	virtual void destroy(void) {}
};

//////////////////////////////////////////////////////////////////////////

class db_handle;
class aio_socket_stream;

/**
 * Database service class. This class is an asynchronous database operation
 * management class. The thread where objects of this class are located must be
 * a non-blocking thread process
 */
class ACL_CPP_API db_service : public ipc_service {
public:
	/**
	 * Constructor when using sqlite database
	 * @param dblimit {size_t} Connection pool count limit for database
	 * @param nthread {int} Maximum thread count for child thread pool
	 * @param win32_gui {bool} Whether it is window class message. If yes, then
	 * internally communication mode is automatically set to _WIN32 message based,
	 * otherwise still uses common socket communication method
	 */
	db_service(size_t dblimit = 100, int nthread = 2, bool win32_gui = false);
	virtual ~db_service(void);

	/**
	 * Asynchronously execute SQL query statement
	 * @param sql {const char*} Standard SQL statement to execute
	 * @param query {db_query*} Class object used to receive execution results
	 */
	void sql_select(const char* sql, db_query* query);

	/**
	 * Asynchronously execute SQL update statement
	 * @param sql {const char*} Standard SQL statement to execute
	 * @param query {db_query*} Class object used to receive execution results
	 */
	void sql_update(const char* sql, db_query* query);

	/**
	 * Add connection object to database connection pool
	 * @param db {db_handle*} Database connection object
	 */
	void push_back(db_handle* db);

protected:
	/**
	 * Subclasses need to implement this function to create database objects
	 * @return {db_handle*}
	 */
	virtual db_handle* db_create() = 0;

	/**
	 * Base class virtual function. Called by base class when new connection
	 * arrives
	 * @param client {aio_socket_stream*} Newly received client connection
	 */
	virtual void on_accept(aio_socket_stream* client);

#if defined(_WIN32) || defined(_WIN64)
	/**
	 * Base class virtual function. Callback function when win32 message from child
	 * thread is received
	 * @param hWnd {HWND} Window handle
	 * @param msg {UINT} User-defined message number
	 * @param wParam {WPARAM} Parameter
	 * @param lParam {LPARAM} Parameter
	 */
	virtual void win32_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

private:
	// Database engine pool
	std::list<db_handle*> dbpool_;

	// Connection pool count limit for database
	size_t dblimit_;

	// Current connection pool count for database
	size_t dbsize_;
};

} // namespace acl

#endif // !defined(ACL_DB_DISABLE)

