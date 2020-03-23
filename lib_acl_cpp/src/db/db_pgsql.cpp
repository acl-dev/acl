#include "acl_stdafx.hpp"
#include "libpq-fe.h"
#ifndef ACL_PREPARE_COMPILE
#include <assert.h>
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/db/pgsql_conf.hpp"
#include "acl_cpp/db/db_pgsql.hpp"
#endif

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)

//////////////////////////////////////////////////////////////////////////

#if defined(HAS_PGSQL) || defined(HAS_PGSQL_DLL)

# ifdef HAS_PGSQL_DLL

#  ifndef STDCALL
#   ifdef ACL_WINDOWS
#    define STDCALL WINAPI
#   else
#    define STDCALL
#   endif // ACL_WINDOWS
#  endif // STDCALL

typedef PGconn* (*PQconnectdb_fn)(const char *conninfo);
typedef ConnStatusType (*PQstatus_fn)(const PGconn *conn);
typedef PGresult *(*PQexec_fn)(PGconn *conn, const char *query);
typedef ExecStatusType (*PQresultStatus_fn)(const PGresult *res);
typedef char *(*PQerrorMessage_fn)(const PGconn *conn);
typedef void  (*PQfinish_fn)(PGconn *conn);
typedef void  (*PQclear_fn)(PGresult *res);
typedef int   (*PQnfields_fn)(const PGresult *res);
typedef char *(*PQfname_fn)(const PGresult *res, int field_num);
typedef int   (*PQntuples_fn)(const PGresult *res);
typedef char *(*PQgetvalue_fn)(const PGresult *res, int tup_num, int field_num);
typedef int   (*PQgetlength_fn)(const PGresult *res, int tup_num, int field_num);
typedef char *(*PQcmdTuples_fn)(PGresult *res);

static PQconnectdb_fn __dbconnect = NULL;
static PQstatus_fn __dbstatus = NULL;
static PQexec_fn __dbexec = NULL;
static PQresultStatus_fn __dbresult_status = NULL;
static PQerrorMessage_fn __dberror_message = NULL;
static PQfinish_fn __dbfinish = NULL;
static PQclear_fn __dbclear = NULL;
static PQnfields_fn __dbnfields = NULL;
static PQfname_fn __dbfname = NULL;
static PQntuples_fn __dbntuples = NULL;
static PQgetvalue_fn __dbget_value = NULL;
static PQgetlength_fn __dbget_length = NULL;
static PQcmdTuples_fn __dbcmd_tuples = NULL;

static acl_pthread_once_t __pgsql_once = ACL_PTHREAD_ONCE_INIT;
static ACL_DLL_HANDLE __pgsql_dll = NULL;

static acl::string __pgsql_path;

// 程序退出释放动态加载的库
#ifndef HAVE_NO_ATEXIT
static void __pgsql_dll_unload(void)
{
	if (__pgsql_dll != NULL) {
		acl_dlclose(__pgsql_dll);
		__pgsql_dll = NULL;
		logger("%s unload ok", __pgsql_path.c_str());
	}
}
#endif

// 动态加载 libpg.dll 库
static void __pgsql_dll_load(void)
{
	if (__pgsql_dll != NULL) {
		logger("pgsql(%s) has been loaded!", __pgsql_path.c_str());
		return;
	}

	const char* path;
	const char* ptr = acl::db_handle::get_loadpath();
	if (ptr) {
		path = ptr;
	} else {
#ifdef ACL_WINDOWS
		path = "libpg.dll";
#else
		path = "libpg.so";
#endif
	}

	__pgsql_dll = acl_dlopen(path);

	if (__pgsql_dll == NULL) {
		logger_fatal("load %s error: %s", path, acl_dlerror());
	}

	// 记录动态库路径，以便于在动态库卸载时输出库路径名
	__pgsql_path = path;

	__dbconnect = (PQconnectdb_fn) acl_dlsym(__pgsql_dll, "PQconnectdb");
	if (__dbconnect == NULL) {
		logger_fatal("load PQconnectdb from %s error %s",
			path, acl_dlerror());
	}

	__dbstatus = (PQstatus_fn) acl_dlsym(__pgsql_dll, "PQstatus");
	if (__dbstatus == NULL) {
		logger_fatal("load PQstatus from %s error %s",
			path, acl_dlerror());
	}

	__dbexec = (PQexec_fn) acl_dlsym(__pgsql_dll, "PQexec");
	if (__dbexec == NULL) {
		logger_fatal("load PQexec from %s error %s",
			path, acl_dlerror());
	}

	__dbresult_status = (PQresultStatus_fn)
		acl_dlsym(__pgsql_dll, "PQresultStatus");
	if (__dbresult_status == NULL) {
		logger_fatal("load PQresultStatus from %s error %s",
			path, acl_dlerror());
	}

	__dberror_message = (PQerrorMessage_fn)
		acl_dlsym(__pgsql_dll, "PQerrorMessage");
	if (__dberror_message == NULL) {
		logger_fatal("load PQerrorMessage from %s error %s",
			path, acl_dlerror());
	}

	__dbfinish = (PQfinish_fn) acl_dlsym(__pgsql_dll, "PQfinish");
	if (__dbfinish == NULL) {
		logger_fatal("load PQfinish_fn from %s error %s",
			path, acl_dlerror());
	}

	__dbclear = (PQclear_fn) acl_dlsym(__pgsql_dll, "PQclear");
	if (__dbclear == NULL) {
		logger_fatal("load PQclear from %s error %s",
			path, acl_dlerror());
	}

	__dbnfields = (PQnfields_fn) acl_dlsym(__pgsql_dll, "PQnfields");
	if (__dbnfields == NULL) {
		logger_fatal("loas PQnfields from %s error %s",
			path, acl_dlerror());
	}

	__dbfname = (PQfname_fn) acl_dlsym(__pgsql_dll, "PQfname");
	if (__dbfname == NULL) {
		logger_fatal("load PQfname from %s error %s",
			path, acl_dlerror());
	}

	__dbntuples = (PQntuples_fn) acl_dlsym(__pgsql_dll, "PQntuples");
	if (__dbntuples == NULL) {
		logger_fatal("load PQntuples from %s error %s",
			path, acl_dlerror());
	}

	__dbget_value = (PQgetvalue_fn) acl_dlsym(__pgsql_dll, "PQgetvalue");
	if (__dbget_value == NULL) {
		logger_fatal("load PQgetvalue from %s error %s",
			path, acl_dlerror());
	}

	__dbget_length = (PQgetlength_fn) acl_dlsym(__pgsql_dll, "PQgetlength");
	if (__dbget_length == NULL) {
		logger_fatal("load PQgetlength from %s error %s",
			path, acl_dlerror());
	}

	__dbcmd_tuples = (PQcmdTuples_fn) acl_dlsym(__pgsql_dll, "PQcmdTuples");
	if (__dbcmd_tuples == NULL) {
		logger_fatal("load PQcmdTuples from %s error %s",
			path, acl_dlerror());
	}

	logger("%s loaded!", path);
#ifndef HAVE_NO_ATEXIT
	atexit(__pgsql_dll_unload);
#endif
}

# else  // if !HAS_PGSQL_DLL
#  define __dbconnect PQconnectdb
#  define __dbstatus PQstatus
#  define __dbexec PQexec
#  define __dbresult_status PQresultStatus
#  define __dberror_message PQerrorMessage
#  define __dbfinish PQfinish
#  define __dbclear PQclear
#  define __dbnfields PQnfields
#  define __dbfname PQfname
#  define __dbntuples PQntuples
#  define __dbget_value PQgetvalue
#  define __dbget_length PQgetlength
#  define __dbcmd_tuples PQcmdTuples
# endif

//////////////////////////////////////////////////////////////////////////

namespace acl
{

//////////////////////////////////////////////////////////////////////////
// pgsql 的记录行类型定义

static void pgsql_rows_free(void* ctx)
{
	PGresult* res = (PGresult *) ctx;
#ifdef HAS_PGSQL_DLL
	if (res && __pgsql_dll) {
#else
	if (res) {
#endif
		__dbclear(res);
	}
}

static void pgsql_rows_save(PGresult* res, db_rows& result)
{
	int   ncolumn = __dbnfields(res);

	// 取出变量名
	for (int j = 0; j < ncolumn; j++) {
		result.names_.push_back(__dbfname(res, j));
	}

	// 开始取出所有行数据结果，加入动态数组中
	int nrow = __dbntuples(res);
	for (int i = 0; i < nrow; i++) {
		db_row* row = NEW db_row(result.names_);
		for (int j = 0; j < ncolumn; j++) {
			char* value = __dbget_value(res, i, j);
			int len = __dbget_length(res, i, j);
			row->push_back(value, (size_t) len);
		}
		result.rows_.push_back(row);
	}

	result.result_free = pgsql_rows_free;
	result.result_tmp_ = res;
}

//////////////////////////////////////////////////////////////////////////

void db_pgsql::sane_pgsql_init(const char* dbaddr, const char* dbname,
	const char* dbuser, const char* dbpass, int conn_timeout,
	int rw_timeout, const char* charset)
{
	affect_count_ = 0;

	if (dbaddr == NULL || *dbaddr == 0) {
		logger_fatal("dbaddr null");
	}
	if (dbname == NULL || *dbname == 0) {
		logger_fatal("dbname null");
	}

	// 地址格式：[dbname@]dbaddr
	const char* ptr = strchr(dbaddr, '@');
	if (ptr) {
		ptr++;
	} else {
		ptr = dbaddr;
	}
	acl_assert(*ptr);
	dbaddr_ = acl_mystrdup(ptr);
	dbname_ = acl_mystrdup(dbname);

	if (dbuser && *dbuser) {
		dbuser_ = acl_mystrdup(dbuser);
	} else {
		dbuser_ = NULL;
	}

	if (dbpass && *dbpass) {
		dbpass_ = acl_mystrdup(dbpass);
	} else {
		dbpass_ = NULL;
	}

	if (charset && *charset) {
		charset_ = charset;
	}

	conn_timeout_ = conn_timeout;
	rw_timeout_   = rw_timeout;

#ifdef HAS_PGSQL_DLL
	acl_pthread_once(&__pgsql_once, __pgsql_dll_load);
#endif
	conn_ = NULL;
}

void db_pgsql::load(void)
{
#ifdef HAS_PGSQL_DLL
	acl_pthread_once(&__pgsql_once, __pgsql_dll_load);
#else
	logger_warn("link pgsql library in static way!");
#endif
}

db_pgsql::db_pgsql(const pgsql_conf& conf)
{
	sane_pgsql_init(conf.get_dbaddr(), conf.get_dbname(),
		conf.get_dbuser(), conf.get_dbpass(), conf.get_conn_timeout(),
		conf.get_rw_timeout(), conf.get_charset());
}

db_pgsql::~db_pgsql(void)
{
	acl_myfree(dbaddr_);
	acl_myfree(dbname_);
	if (dbuser_) {
		acl_myfree(dbuser_);
	}
	if (dbpass_) {
		acl_myfree(dbpass_);
	}
#ifdef HAS_PGSQL_DLL
	if (conn_ && __pgsql_dll) {
#else
	if (conn_) {
#endif
		__dbfinish(conn_);
	}
}

const char* db_pgsql::dbtype(void) const
{
	static const char* type = "pgsql";
	return type;
}

int db_pgsql::get_errno(void) const
{
	if (conn_) {
		return __dbstatus(conn_);
	} else {
		return -1;
	}
}

const char* db_pgsql::get_error(void) const
{
	if (conn_) {
		return __dberror_message(conn_);
	} else {
		return "pgsql not opened yet!";
	}
}

static acl_pthread_key_t __thread_key;

static void thread_free_dummy(void* ctx)
{
	if ((unsigned long) acl_pthread_self() != acl_main_thread_self()) {
		acl_myfree(ctx);
	}
}

static int* __main_dummy = NULL;
#ifndef HAVE_NO_ATEXIT
static void main_free_dummy(void)
{
	if (__main_dummy) {
		acl_myfree(__main_dummy);
		__main_dummy = NULL;
	}
}
#endif

static acl_pthread_once_t __thread_once_control = ACL_PTHREAD_ONCE_INIT;

static void thread_once(void)
{
	if (acl_pthread_key_create(&__thread_key, thread_free_dummy) != 0) {
		abort();
	}
}

bool db_pgsql::dbopen(const char* /* charset = NULL */)
{
	if (conn_) {
		return true;
	}

	char  tmpbuf[256];
	char *db_host, *db_unix;
	int   db_port;

	char* ptr = strchr(dbaddr_, '/');
	if (ptr == NULL) {
		ACL_SAFE_STRNCPY(tmpbuf, dbaddr_, sizeof(tmpbuf));
		ptr = strchr(tmpbuf, ':');
		if (ptr == NULL || *(ptr + 1) == 0) {
			logger_error("invalid db_addr=%s", dbaddr_);
			return false;
		} else {
			*ptr++ = 0;
		}
		db_host = tmpbuf;

		db_port = atoi(ptr);
		if (db_port <= 0) {
			logger_error("invalid port=%d", db_port);
			return false;
		}
		db_unix = NULL;
	} else {
		db_unix = dbaddr_;
		db_host = db_unix;
		db_port = 0;
	}

	int* dummy;

	if (acl_pthread_once(&__thread_once_control, thread_once) != 0) {
		logger_error("call thread_once error: %s", acl_last_serror());
	} else if (!(dummy = (int*) acl_pthread_getspecific(__thread_key))) {
		dummy = (int*) acl_mymalloc(sizeof(int));
		*dummy = 1;
		if  (acl_pthread_setspecific(__thread_key, dummy) != 0) {
			abort();
		}

		if ((unsigned long) acl_pthread_self()
			== acl_main_thread_self()) {

			__main_dummy = dummy;
#ifndef HAVE_NO_ATEXIT
			atexit(main_free_dummy);
#endif
		}
	}

	string info;
	info.format("host=%s dbname=%s", db_host, dbname_);
	if (db_unix == NULL) {
		info.format_append(" port=%d", db_port);
	}
	if (dbuser_) {
		info.format_append(" user=%s", dbuser_);
	}
	if (dbpass_) {
		info.format_append(" password=%s", dbpass_);
	}

	conn_ = __dbconnect(info.c_str());
	if (conn_ == NULL || __dbstatus(conn_) != CONNECTION_OK) {
		logger_error("connect pgsql error(%s), db_host=%s, db_port=%d,"
			" db_unix=%s, db_name=%s, db_user=%s, db_pass=%s",
			conn_ ? __dberror_message(conn_) : "connect error",
			db_host ? db_host : "null", db_port,
			db_unix ? db_unix : "null",
			dbname_ ? dbname_ : "null",
			dbuser_ ? dbuser_ : "null",
			dbpass_ ? dbpass_ : "null");

		if (conn_) {
			__dbfinish(conn_);
			conn_ = NULL;
		}

		return false;
	}

	return true;
}

bool db_pgsql::is_opened(void) const
{
	return conn_ ? true : false;
}

bool db_pgsql::close(void)
{
#ifdef HAS_PGSQL_DLL
	if (conn_ && __pgsql_dll) {
#else
	if (conn_) {
#endif
		__dbfinish(conn_);
		conn_ = NULL;
	}
	return true;
}

void* db_pgsql::sane_pgsql_query(const char* sql)
{
	if (conn_ == NULL) {
		logger_error("db(%s) not opened yet!", dbname_);
		return NULL;
	}

	PGresult* res = __dbexec(conn_, sql);
	if (res) {
		return res;
	}

	/* 重新打开连接进行重试 */
	close();
	if (!dbopen()) {
		logger_error("reopen db(%s) error", dbname_);
		return NULL;
	}
	res = __dbexec(conn_, sql);
	if (res) {
		return res;
	}

	logger_error("db(%s), sql(%s) error(%s)",
		dbname_, sql, __dberror_message(conn_));
	return NULL;
}

bool db_pgsql::tbl_exists(const char* tbl_name)
{
	free_result();

	db_rows rows;
	string sql;
	sql.format("select * from %s limit 1", tbl_name);

	PGresult* res = (PGresult *) sane_pgsql_query(sql);
	if (res == NULL) {
		return false;
	}

	if (__dbresult_status(res) != PGRES_TUPLES_OK) {
		__dbclear(res);
		return false;
	}

	__dbclear(res);
	return true;
}

bool db_pgsql::sql_select(const char* sql, db_rows* result /* = NULL */)
{
	// 优先调用基类方法释放上次的查询结果
	free_result();

	PGresult* res = (PGresult *) sane_pgsql_query(sql);
	if (res == NULL) {
		return false;
	}

	if (__dbresult_status(res) != PGRES_TUPLES_OK) {
		logger_error("db(%s), sql(%s) error(%s)",
			dbname_, sql, __dberror_message(conn_));
		__dbclear(res);
		return false;
	}

	if (__dbntuples(res) <= 0) {
		__dbclear(res);
		result_ = NULL;
		return true;
	}

	if (result != NULL) {
		pgsql_rows_save(res, *result);
	} else {
		result_ = NEW db_rows();
		pgsql_rows_save(res, *result_);
	}

	return true;
}

bool db_pgsql::sql_update(const char* sql)
{
	free_result();

	PGresult* res = (PGresult *) sane_pgsql_query(sql);
	if (res == NULL) {
		return false;
	}

	if (__dbresult_status(res) != PGRES_COMMAND_OK) {
		logger_error("db(%s), sql(%s) error(%s)",
			dbname_, sql, __dberror_message(conn_));
		__dbclear(res);
		return false;
	}

	const char* ptr = __dbcmd_tuples(res);
	if (ptr == NULL || *ptr == 0) {
		__dbclear(res);
		return true;
	}

	affect_count_ = atoi(ptr);
	__dbclear(res);
	return true;
}

int db_pgsql::affect_count(void) const
{
	return affect_count_;
}

bool db_pgsql::begin_transaction(void)
{
	const char* sql = "start transaction";
	if (!sql_update(sql)) {
		logger_error("%s error: %s", sql, get_error());
		return false;
	}
	return true;
}

bool db_pgsql::commit(void)
{
	const char* sql = "commit";
	if (!sql_update(sql)) {
		logger_error("%s error: %s", sql, get_error());
		return false;
	}
	return true;
}

bool db_pgsql::rollback(void)
{
	const char* sql = "rollback";
	if (!sql_update(sql)) {
		logger_error("%s error: %s", sql, get_error());
		return false;
	}
	return true;
}

} // name acl

#else

namespace acl
{

db_pgsql::db_pgsql(const pgsql_conf&)
: affect_count_(0)
{
}

db_pgsql::~db_pgsql(void)
{
	logger_fatal("Please #define HAS_PGSQL or HAS_PGSQL_DLL first");
}

const char* db_pgsql::dbtype() const
{
	return NULL;
}

bool db_pgsql::dbopen(const char*)
{
	return false;
}

bool db_pgsql::is_opened() const
{
	return false;
}

bool db_pgsql::close(void)
{
	return false;
}

bool db_pgsql::tbl_exists(const char*)
{
	return false;
}

bool db_pgsql::sql_select(const char*, db_rows*)
{
	return false;
}

bool db_pgsql::sql_update(const char*)
{
	return false;
}

bool db_pgsql::begin_transaction()
{
	return false;
}

bool db_pgsql::commit()
{
	return false;
}

bool db_pgsql::rollback(void)
{
	return false;
}

int db_pgsql::affect_count() const
{
	return 0;
}

int db_pgsql::get_errno() const
{
	return -1;
}

const char* db_pgsql::get_error() const
{
	return "pgsql not opened yet!";
}

} // namespace acl

#endif  // !HAS_MYSQL && !HAS_MYSQL_DLL

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)
