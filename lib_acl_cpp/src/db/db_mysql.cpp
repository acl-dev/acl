#include "acl_stdafx.hpp"
#include <assert.h>
#include "mysql.h"
#include "errmsg.h"
#include "mysqld_error.h"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/db/db_mysql.hpp"

//////////////////////////////////////////////////////////////////////////

#ifdef HAS_MYSQL

#if defined(ACL_CPP_DLL) || defined(USE_DYNAMIC)
typedef unsigned long (*mysql_libversion_fn)(void);
typedef const char* (*mysql_client_info_fn)(void);
typedef MYSQL* (*mysql_init_fn)(MYSQL*);
typedef MYSQL* (*mysql_open_fn)(MYSQL*, const char*, const char*,
    const char*, const char*, unsigned int,
    const char*, unsigned long);
typedef void (*mysql_close_fn)(MYSQL*);
typedef int  (*mysql_options_fn)(MYSQL*,enum mysql_option option, const void*);
typedef my_bool (*mysql_autocommit_fn)(MYSQL*, my_bool);
typedef unsigned int (*mysql_errno_fn)(MYSQL*);
typedef const char* (*mysql_error_fn)(MYSQL*);
typedef int (*mysql_query_fn)(MYSQL*, const char*);
typedef unsigned int (*mysql_num_fields_fn)(MYSQL_RES*);
typedef MYSQL_FIELD* (*mysql_fetch_fields_fn)(MYSQL_RES*);
typedef MYSQL_ROW (*mysql_fetch_row_fn)(MYSQL_RES*);
typedef MYSQL_RES* (*mysql_store_result_fn)(MYSQL*);
typedef my_ulonglong (*mysql_num_rows_fn)(MYSQL_RES*);
typedef void (*mysql_free_result_fn)(MYSQL_RES*);
typedef my_ulonglong (*mysql_affected_rows_fn)(MYSQL*);
typedef int (*mysql_set_character_set_fn)(MYSQL*, const char*);
typedef const char* (*mysql_character_set_name_fn)(MYSQL*);

static mysql_libversion_fn __mysql_libversion = NULL;
static mysql_client_info_fn __mysql_client_info = NULL;
static mysql_init_fn __mysql_init = NULL;
static mysql_open_fn __mysql_open = NULL;
static mysql_close_fn __mysql_close = NULL;
static mysql_options_fn __mysql_options = NULL;
static mysql_autocommit_fn __mysql_autocommit = NULL;
static mysql_errno_fn __mysql_errno = NULL;
static mysql_error_fn __mysql_error = NULL;
static mysql_query_fn __mysql_query = NULL;
static mysql_num_fields_fn __mysql_num_fields = NULL;
static mysql_fetch_fields_fn __mysql_fetch_fields = NULL;
static mysql_fetch_row_fn __mysql_fetch_row = NULL;
static mysql_store_result_fn __mysql_store_result = NULL;
static mysql_num_rows_fn __mysql_num_rows = NULL;
static mysql_free_result_fn __mysql_free_result = NULL;
static mysql_affected_rows_fn __mysql_affected_rows = NULL;
static mysql_set_character_set_fn __mysql_set_character_set = NULL;
static mysql_character_set_name_fn __mysql_character_set_name = NULL;

static acl_pthread_once_t __mysql_once = ACL_PTHREAD_ONCE_INIT;
static ACL_DLL_HANDLE __mysql_dll = NULL;

// 程序退出释放动态加载的库
static void __mysql_dll_unload(void)
{
	if (__mysql_dll != NULL)
	{
		acl_dlclose(__mysql_dll);
		__mysql_dll = NULL;
		logger("libmysql.dll unload ok");
	}
}

// 动态加载 libmysql.dll 库
static void __mysql_dll_load(void)
{
	if (__mysql_dll != NULL)
		logger_fatal("__mysql_dll not null");

	__mysql_dll = acl_dlopen("libmysql.dll");
	if (__mysql_dll == NULL)
		logger_fatal("load libmysql.dll error: %s", acl_last_serror());

	__mysql_libversion = (mysql_libversion_fn) acl_dlsym(__mysql_dll, "mysql_get_client_version");
	if (__mysql_libversion == NULL)
		logger_fatal("load mysql_get_client_version from libmysql.dll error: %s", acl_last_serror());

	__mysql_client_info = (mysql_client_info_fn) acl_dlsym(__mysql_dll, "mysql_get_client_info");
	if (__mysql_client_info == NULL)
		logger_fatal("load mysql_get_client_info from libmysql.dll error: %s", acl_last_serror());

	__mysql_init = (mysql_init_fn) acl_dlsym(__mysql_dll, "mysql_init");
	if (__mysql_init == NULL)
		logger_fatal("load mysql_init from libmysql.dll error: %s", acl_last_serror());

	__mysql_open = (mysql_open_fn) acl_dlsym(__mysql_dll, "mysql_real_connect");
	if (__mysql_open == NULL)
		logger_fatal("load mysql_real_connect from libmysql.dll error: %s", acl_last_serror());

	__mysql_close = (mysql_close_fn) acl_dlsym(__mysql_dll, "mysql_close");
	if (__mysql_close == NULL)
		logger_fatal("load mysql_close from libmysql.dll error: %s", acl_last_serror());

	__mysql_options = (mysql_options_fn) acl_dlsym(__mysql_dll, "mysql_options");
	if (__mysql_options == NULL)
		logger_fatal("load mysql_options from libmysql.dll error: %s", acl_last_serror());

	__mysql_autocommit = (mysql_autocommit_fn) acl_dlsym(__mysql_dll, "mysql_autocommit");
	if (__mysql_autocommit == NULL)
		logger_fatal("load mysql_autocommit from libmysql.dll error: %s", acl_last_serror());

	__mysql_errno = (mysql_errno_fn) acl_dlsym(__mysql_dll, "mysql_errno");
	if (__mysql_errno == NULL)
		logger_fatal("load mysql_errno from libmysql.dll error: %s", acl_last_serror());

	__mysql_error = (mysql_error_fn) acl_dlsym(__mysql_dll, "mysql_error");
	if (__mysql_error == NULL)
		logger_fatal("load mysql_error from libmysql.dll error: %s", acl_last_serror());

	__mysql_query = (mysql_query_fn) acl_dlsym(__mysql_dll, "mysql_query");
	if (__mysql_query == NULL)
		logger_fatal("load mysql_query from libmysql.dll error: %s", acl_last_serror());

	__mysql_num_fields = (mysql_num_fields_fn) acl_dlsym(__mysql_dll, "mysql_num_fields");
	if (__mysql_num_fields == NULL)
		logger_fatal("load mysql_num_fields from libmysql.dll error: %s", acl_last_serror());

	__mysql_fetch_fields = (mysql_fetch_fields_fn) acl_dlsym(__mysql_dll, "mysql_fetch_fields");
	if (__mysql_fetch_fields == NULL)
		logger_fatal("load mysql_fetch_fields from libmysql.dll error: %s", acl_last_serror());

	__mysql_fetch_row = (mysql_fetch_row_fn) acl_dlsym(__mysql_dll, "mysql_fetch_row");
	if (__mysql_fetch_row == NULL)
		logger_fatal("load mysql_fetch_row from libmysql.dll error: %s", acl_last_serror());

	__mysql_store_result = (mysql_store_result_fn) acl_dlsym(__mysql_dll, "mysql_store_result");
	if (__mysql_store_result == NULL)
		logger_fatal("load mysql_store_result from libmysql.dll error: %s", acl_last_serror());

	__mysql_num_rows = (mysql_num_rows_fn) acl_dlsym(__mysql_dll, "mysql_num_rows");
	if (__mysql_num_rows == NULL)
		logger_fatal("load mysql_num_rows from libmysql.dll error: %s", acl_last_serror());

	__mysql_free_result = (mysql_free_result_fn) acl_dlsym(__mysql_dll, "mysql_free_result");
	if (__mysql_free_result == NULL)
		logger_fatal("load mysql_free_result from libmysql.dll error: %s", acl_last_serror());

	__mysql_affected_rows = (mysql_affected_rows_fn) acl_dlsym(__mysql_dll, "mysql_affected_rows");
	if (__mysql_affected_rows == NULL)
		logger_fatal("load mysql_affected_rows from libmysql.dll error: %s", acl_last_serror());

	__mysql_set_character_set = (mysql_set_character_set_fn) acl_dlsym(__mysql_dll, "mysql_set_character_set");
	if (__mysql_affected_rows == NULL)
		logger_fatal("load mysql_set_character_set_fn from libmysql.dll error: %s", acl_last_serror());

	__mysql_character_set_name = (mysql_character_set_name_fn) acl_dlsym(__mysql_dll, "mysql_character_set_name");
	if (__mysql_affected_rows == NULL)
		logger_fatal("load mysql_character_set_name from libmysql.dll error: %s", acl_last_serror());

	logger("libmysql.dll loaded");
	atexit(__mysql_dll_unload);
}
#else

# define  __mysql_libversion mysql_get_client_version
# define  __mysql_client_info mysql_get_client_info
# define  __mysql_init mysql_init
# define  __mysql_open mysql_real_connect
# define  __mysql_close mysql_close
# define  __mysql_options mysql_options
# define  __mysql_autocommit mysql_autocommit
# define  __mysql_errno mysql_errno
# define  __mysql_error mysql_error
# define  __mysql_query mysql_query
# define  __mysql_num_fields mysql_num_fields
# define  __mysql_fetch_fields mysql_fetch_fields
# define  __mysql_fetch_row mysql_fetch_row
# define  __mysql_store_result mysql_store_result
# define  __mysql_num_rows mysql_num_rows
# define  __mysql_free_result mysql_free_result
# define  __mysql_affected_rows mysql_affected_rows
# define  __mysql_set_character_set mysql_set_character_set
# define  __mysql_character_set_name mysql_character_set_name

#endif

//////////////////////////////////////////////////////////////////////////

namespace acl
{

//////////////////////////////////////////////////////////////////////////
// mysql 的记录行类型定义

class db_mysql_rows : public db_rows
{
public:
	db_mysql_rows(MYSQL_RES *my_res)
	{
		int   ncolumn = __mysql_num_fields(my_res);
		MYSQL_FIELD *fields = __mysql_fetch_fields(my_res);

		// 取出变量名
		for (int j = 0; j < ncolumn; j++)
			names_.push_back(fields[j].name);

		// 开始取出所有行数据结果，加入动态数组中
		while (true)
		{
			MYSQL_ROW my_row = __mysql_fetch_row(my_res);
			if (my_row == NULL)
				break;
			db_row* row = NEW db_row(names_);
			for (int j = 0; j < ncolumn; j++)
				row->push_back(my_row[j]);
			rows_.push_back(row);
		}

		my_res_ = my_res;
	}

	~db_mysql_rows()
	{
		__mysql_free_result(my_res_);
	}

private:
	MYSQL_RES *my_res_;
};

//////////////////////////////////////////////////////////////////////////

db_mysql::db_mysql(const char* dbaddr, const char* dbname,
	const char* dbuser, const char* dbpass,
	unsigned long dbflags /* = 0 */, bool auto_commit /* = true */,
	int conn_timeout /* = 60 */, int rw_timeout /* = 60 */)
{
	acl_assert(dbaddr && *dbaddr);
	acl_assert(dbname && *dbname);
	dbaddr_ = acl_mystrdup(dbaddr);
	dbname_ = acl_mystrdup(dbname);

	if (dbuser)
		dbuser_ = acl_mystrdup(dbuser);
	else
		dbuser_ = NULL;

	if (dbpass)
		dbpass_ = acl_mystrdup(dbpass);
	else
		dbpass_ = NULL;

	dbflags_ = dbflags;
	auto_commit_ = auto_commit;
	conn_timeout_ = conn_timeout;
	rw_timeout_ = rw_timeout;

#if defined(ACL_CPP_DLL) || defined(USE_DYNAMIC)
	acl_pthread_once(&__mysql_once, __mysql_dll_load);
#endif
	conn_ = NULL;
}

db_mysql::~db_mysql()
{
	acl_myfree(dbaddr_);
	acl_myfree(dbname_);
	if (dbuser_)
		acl_myfree(dbuser_);
	if (dbpass_)
		acl_myfree(dbpass_);
	if (conn_)
		__mysql_close(conn_);
}

unsigned long db_mysql::mysql_libversion() const
{
	return __mysql_libversion();
}

const char* db_mysql::mysql_client_info() const
{
	return __mysql_client_info();
}

const char* db_mysql::dbtype() const
{
	static const char* type = "mysql";
	return type;
}

int db_mysql::get_errno() const
{
	if (conn_)
		return __mysql_errno(conn_);
	else
		return -1;
}

const char* db_mysql::get_error() const
{
	if (conn_)
		return __mysql_error(conn_);
	else
		return "mysql not opened yet!";
}

bool db_mysql::open(const char* local_charset /* = GBK */)
{
	if (conn_)
		return true;

	char  tmpbuf[256];
	char *db_host, *db_unix;
	int   db_port;

	char* ptr = strchr(dbaddr_, '/');
	if (ptr == NULL) {
		ACL_SAFE_STRNCPY(tmpbuf, dbaddr_, sizeof(tmpbuf));
		ptr = strchr(tmpbuf, ':');
		if (ptr == NULL || *(ptr + 1) == 0)
		{
			logger_error("invalid db_addr=%s", dbaddr_);
			return false;
		}
		else
			*ptr++ = 0;
		db_host = tmpbuf;

		db_port = atoi(ptr);
		if (db_port <= 0)
		{
			logger_error("invalid port=%d", db_port);
			return false;
		}
		db_unix = NULL;
	} else {
		db_unix = dbaddr_;
		db_host = NULL;
		db_port = 0;
	}

	conn_ = __mysql_init(NULL);
	if (conn_ == NULL)
	{
		logger_error("mysql init error");
		return false;
	}

	if (conn_timeout_ > 0)
#if MYSQL_VERSION_ID >= 50500
		__mysql_options(conn_, MYSQL_OPT_CONNECT_TIMEOUT,
			(const void*) &conn_timeout_);
#else
		__mysql_options(conn_, MYSQL_OPT_CONNECT_TIMEOUT,
			(const char*) &conn_timeout_);
#endif

	if (rw_timeout_ > 0)
	{
#if MYSQL_VERSION_ID >= 50500
		__mysql_options(conn_, MYSQL_OPT_READ_TIMEOUT,
			(const void*) &rw_timeout_);
		__mysql_options(conn_, MYSQL_OPT_WRITE_TIMEOUT,
			(const void*) &rw_timeout_);
#else
		__mysql_options(conn_, MYSQL_OPT_READ_TIMEOUT,
			(const char*) &rw_timeout_);
		__mysql_options(conn_, MYSQL_OPT_WRITE_TIMEOUT,
			(const char*) &rw_timeout_);
#endif
	}

	my_bool reconnect = 1;

#if MYSQL_VERSION_ID >= 50500
	__mysql_options(conn_, MYSQL_OPT_RECONNECT, (const void*) &reconnect);
#else
	__mysql_options(conn_, MYSQL_OPT_RECONNECT, (const char*) &reconnect);
#endif

	if (__mysql_open(conn_, db_host, dbuser_ ? dbuser_ : "",
		dbpass_ ? dbpass_ : "", dbname_, db_port,
		db_unix, dbflags_) == NULL)
	{
		logger_error("connect mysql error(%s), db_host=%s, db_port=%d,"
			" db_unix=%s, db_name=%s, db_user=%s, db_pass=%s",
			__mysql_error(conn_), db_host ? db_host : "null", db_port,
			db_unix ? db_unix : "null", dbname_, dbuser_, dbpass_);

		__mysql_close(conn_);
		conn_ = NULL;
		return false;
	}

	if (local_charset)
	{
		if (!__mysql_set_character_set(conn_, local_charset))
			logger("set mysql charset to %s, %s", local_charset,
				__mysql_character_set_name(conn_));
		else
			logger_error("set mysql to %s error %s",
				local_charset, __mysql_error(conn_));
	}

#if MYSQL_VERSION_ID >= 50000
	if (__mysql_autocommit(conn_, auto_commit_ ? 1 : 0) != 0)
	{
		logger_error("mysql_autocommit error");
		__mysql_close(conn_);
		conn_ = NULL;
		return (false);
	}
#else
	auto_commit_ = false;
#endif

	return true;
}

bool db_mysql::is_opened() const
{
	return conn_ ? true : false;
}

bool db_mysql::close()
{
	if (conn_ != NULL)
	{
		__mysql_close(conn_);
		conn_ = NULL;
	}
	return true;
}

bool db_mysql::sane_mysql_query(const char* sql)
{
	if (conn_ == NULL)
	{
		logger_error("db(%s) not opened yet!", dbname_);
		return false;
	}
	if (__mysql_query(conn_, sql) == 0)
		return true;

	int errnum = __mysql_errno(conn_);
	if (errnum != CR_SERVER_LOST && errnum != CR_SERVER_GONE_ERROR)
	{
		logger_error("db(%s): sql(%s) error(%s)",
			dbname_, sql, __mysql_error(conn_));
		return false;
	}

	/* 重新打开MYSQL连接进行重试 */
	close();
	if (open("GBK") == false)
	{
		logger_error("reopen db(%s) error", dbname_);
		return false;
	}
	if (__mysql_query(conn_, sql) == 0)
		return true;
	logger_error("db(%s), sql(%s) error(%s)",
		dbname_, sql, __mysql_error(conn_));
	close();
	return false;
}

bool db_mysql::tbl_exists(const char* tbl_name)
{
	if (conn_ == NULL)
	{
		logger_error("db(%s) not opened yet", dbname_);
		return false;
	}

	char sql[256];

	snprintf(sql, sizeof(sql), "show tables like '%s'", tbl_name);
	if (sane_mysql_query(sql) == false)
		return false;
	MYSQL_RES *my_res = __mysql_store_result(conn_);
	if (my_res == NULL)
	{
		if (__mysql_errno(conn_) != 0)
		{
			logger_error("db(%s), sql(%s) error(%s)",
				dbname_, sql, __mysql_error(conn_));
			close();
		}
		return false;
	}

	bool ret;
	if (__mysql_num_rows(my_res) > 0)
		ret = true;
	else
		ret = false;
	__mysql_free_result(my_res);
	return ret;
}

bool db_mysql::sql_select(const char* sql)
{
	if (sane_mysql_query(sql) == false)
		return false;
	MYSQL_RES *my_res = __mysql_store_result(conn_);
	if (my_res == NULL)
	{
		if (__mysql_errno(conn_) != 0)
		{
			logger_error("db(%s), sql(%s) error(%s)",
				dbname_, sql, __mysql_error(conn_));
			close();
		}
		return false;
	}

	my_ulonglong nrow = __mysql_num_rows(my_res);
	if (nrow <= 0)
	{
		__mysql_free_result(my_res);
		result_ = NULL;
		return true;
	}

	result_ = NEW db_mysql_rows(my_res);
	return true;
}

bool db_mysql::sql_update(const char* sql)
{
	if (sane_mysql_query(sql) == false)
		return false;
	int ret = (int) __mysql_affected_rows(conn_);
	if (ret == -1)
		return false;
	return true;
}

int db_mysql::affect_count() const
{
	if (!is_opened())
	{
		logger_error("mysql not opened yet");
		return -1;
	}

	return (int) __mysql_affected_rows(conn_);
}

}  // namespace acl

#else

namespace acl
{

db_mysql::db_mysql(const char*, const char*,
	const char*, const char*,
	unsigned long, bool, int, int)
{
	logger_fatal("Please #define HAS_MYSQL first");
}

db_mysql::~db_mysql(void)
{
}

const char* db_mysql::dbtype() const
{
	return NULL;
}

bool db_mysql::open(const char*)
{
	return false;
}

bool db_mysql::is_opened() const
{
	return false;
}

bool db_mysql::close(void)
{
	return false;
}

bool db_mysql::tbl_exists(const char*)
{
	return false;
}

bool db_mysql::sql_select(const char*)
{
	return false;
}

bool db_mysql::sql_update(const char*)
{
	return false;
}

int db_mysql::affect_count() const
{
	return 0;
}

unsigned long db_mysql::mysql_libversion() const
{
	return 0;
}

const char* db_mysql::mysql_client_info() const
{
	return NULL;
}

int db_mysql::get_errno() const
{
	return -1;
}

const char* db_mysql::get_error() const
{
	return "mysql not opened yet!";
}

} // namespace acl

#endif  // HAS_MYSQL
