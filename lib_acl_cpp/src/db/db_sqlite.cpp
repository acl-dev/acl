#include "acl_stdafx.hpp"
#ifdef ACL_WINDOWS
#include <io.h>
#endif
#include "sqlite3.h"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/charset_conv.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/db/db_sqlite.hpp"
#endif

#if !defined(ACL_DB_DISABLE)

#if defined(HAS_SQLITE) || defined(HAS_SQLITE_DLL)

# ifdef HAS_SQLITE_DLL

#  ifndef STDCALL
#   define STDCALL
#  endif // STDCALL

 typedef char* (STDCALL *sqlite3_libversion_fn)(void);
 typedef int   (STDCALL *sqlite3_open_fn)(const char*, sqlite3**);
 typedef int   (STDCALL *sqlite3_close_fn)(sqlite3*);
 typedef int   (STDCALL *sqlite3_get_table_fn)(sqlite3*, const char*,
	 char***, int*, int*, char**);
 typedef void  (STDCALL *sqlite3_free_table_fn)(char**);
 typedef int   (STDCALL *sqlite3_busy_handler_fn)(sqlite3*,
	 int(*)(void*,int), void*);
 typedef const char* (STDCALL *sqlite3_errmsg_fn)(sqlite3*);
 typedef int   (STDCALL *sqlite3_errcode_fn)(sqlite3*);
 typedef int   (STDCALL *sqlite3_changes_fn)(sqlite3*);
 typedef int   (STDCALL *sqlite3_total_changes_fn)(sqlite3*);
 typedef int   (STDCALL *sqlite3_busy_timeout_fn)(sqlite3*,int);
 typedef int   (STDCALL *sqlite3_prepare_v2_fn)(sqlite3*, const char*, int,
	 sqlite3_stmt**, const char**);
 typedef int   (STDCALL *sqlite3_step_fn)(sqlite3_stmt*);
 typedef int   (STDCALL *sqlite3_exec_fn)(sqlite3*, const char *sql,
         int(*)(void*,int,char**,char**), void*, char**);
 typedef int   (STDCALL *sqlite3_reset_fn)(sqlite3_stmt*);
 typedef int   (STDCALL *sqlite3_bind_blob_fn)(sqlite3_stmt*, int,
         const void *, int, void(*)(void*));
 typedef int   (STDCALL *sqlite3_bind_int_fn)(sqlite3_stmt*, int, int);
 typedef int   (STDCALL *sqlite3_bind_int64_fn)(sqlite3_stmt*, int, sqlite3_int64);
 typedef int   (STDCALL *sqlite3_bind_text_fn)(sqlite3_stmt*, int,
        const char*, int, void(*)(void*));
 typedef int   (STDCALL *sqlite3_column_count_fn)(sqlite3_stmt*);
 typedef int   (STDCALL *sqlite3_column_type_fn)(sqlite3_stmt*, int);
 typedef int   (STDCALL *sqlite3_column_bytes_fn)(sqlite3_stmt*, int);
 typedef const char* (STDCALL *sqlite3_column_name_fn)(sqlite3_stmt*, int);
 typedef int   (STDCALL *sqlite3_data_count_fn)(sqlite3_stmt*);
 typedef int   (STDCALL *sqlite3_column_int_fn)(sqlite3_stmt*, int);
 typedef int64_t   (STDCALL *sqlite3_column_int64_fn)(sqlite3_stmt*, int);
 typedef int   (STDCALL *sqlite3_column_double_fn)(sqlite3_stmt*, int);
 typedef const void*   (STDCALL *sqlite3_column_blob_fn)(sqlite3_stmt*, int);
 typedef const unsigned char* (STDCALL *sqlite3_column_text_fn)(sqlite3_stmt*, int);
 typedef int   (STDCALL *sqlite3_finalize_fn)(sqlite3_stmt*);
 typedef void   (STDCALL *sqlite3_free_fn)(void*);

 static sqlite3_libversion_fn __sqlite3_libversion = NULL;
 static sqlite3_open_fn __sqlite3_open = NULL;
 static sqlite3_close_fn __sqlite3_close = NULL;
 static sqlite3_get_table_fn __sqlite3_get_table = NULL;
 static sqlite3_free_table_fn __sqlite3_free_table = NULL;
 static sqlite3_busy_handler_fn __sqlite3_busy_handler = NULL;
 static sqlite3_errmsg_fn __sqlite3_errmsg = NULL;
 static sqlite3_errcode_fn __sqlite3_errcode = NULL;
 static sqlite3_changes_fn __sqlite3_changes = NULL;
 static sqlite3_total_changes_fn __sqlite3_total_changes = NULL;
 static sqlite3_busy_timeout_fn __sqlite3_busy_timeout = NULL;
 static sqlite3_prepare_v2_fn __sqlite3_prepare_v2 = NULL;
 static sqlite3_step_fn __sqlite3_step = NULL;
 static sqlite3_exec_fn __sqlite3_exec = NULL;
 static sqlite3_reset_fn __sqlite3_reset = NULL;
 static sqlite3_bind_blob_fn __sqlite3_bind_blob = NULL;
 static sqlite3_bind_int_fn __sqlite3_bind_int = NULL;
 static sqlite3_bind_int64_fn __sqlite3_bind_int64 = NULL;
 static sqlite3_bind_text_fn __sqlite3_bind_text = NULL;
 static sqlite3_column_count_fn __sqlite3_column_count = NULL;
 static sqlite3_column_type_fn __sqlite3_column_type = NULL;
 static sqlite3_column_bytes_fn __sqlite3_column_bytes = NULL;
 static sqlite3_column_name_fn __sqlite3_column_name = NULL;
 static sqlite3_data_count_fn __sqlite3_data_count = NULL;
 static sqlite3_column_int_fn __sqlite3_column_int = NULL;
 static sqlite3_column_int64_fn __sqlite3_column_int64 = NULL;
 static sqlite3_column_double_fn __sqlite3_column_double = NULL;
 static sqlite3_column_blob_fn __sqlite3_column_blob = NULL;
 static sqlite3_column_text_fn __sqlite3_column_text = NULL;
 static sqlite3_finalize_fn __sqlite3_finalize = NULL;
 static sqlite3_free_fn __sqlite3_free = NULL;

 static acl_pthread_once_t __sqlite_once = ACL_PTHREAD_ONCE_INIT;
 static ACL_DLL_HANDLE __sqlite_dll = NULL;
 static acl::string __sqlite_path;

 // 程序退出释放动态加载的库
#ifndef HAVE_NO_ATEXIT
 static void __sqlite_dll_unload(void)
 {
	 if (__sqlite_dll != NULL) {
		 acl_dlclose(__sqlite_dll);
		 __sqlite_dll = NULL;
		 logger("%s unload ok", __sqlite_path.c_str());
	 }
 }
#endif

 // 动态加载 sqlite3.dll 库
 static void __sqlite_dll_load(void)
 {
	if (__sqlite_dll != NULL) {
		logger_warn("sqlite(%s) to be loaded again!",
			__sqlite_path.c_str());
		return;
	}

	const char* path;
	const char* ptr = acl::db_handle::get_loadpath();
	if (ptr) {
		path = ptr;
	} else {
#ifdef ACL_WINDOWS
		path = "sqlite3.dll";
#else
		path = "sqlite3.so";
#endif
	}

	__sqlite_dll = acl_dlopen(path);
	if (__sqlite_dll == NULL) {
		logger_fatal("load %s error: %s", path, acl_last_serror());
	}

	__sqlite_path = path;

	__sqlite3_libversion = (sqlite3_libversion_fn)
		acl_dlsym(__sqlite_dll, "sqlite3_libversion");
	if (__sqlite3_libversion == NULL) {
		logger_fatal("load sqlite3_libversion from %s error: %s",
			path, acl_last_serror());
	}

	__sqlite3_open = (sqlite3_open_fn)
		acl_dlsym(__sqlite_dll, "sqlite3_open");
	if (__sqlite3_open == NULL) {
		logger_fatal("load sqlite3_open from %s error: %s",
			path, acl_last_serror());
	}

	__sqlite3_close = (sqlite3_close_fn)
		acl_dlsym(__sqlite_dll, "sqlite3_close");
	if (__sqlite3_close == NULL) {
		logger_fatal("load sqlite3_close from %s error: %s",
			path, acl_last_serror());
	}

	__sqlite3_get_table = (sqlite3_get_table_fn)
		acl_dlsym(__sqlite_dll, "sqlite3_get_table");
	if (__sqlite3_get_table == NULL) {
		logger_fatal("load sqlite3_get_table from %s error: %s",
			path, acl_last_serror());
	}

	__sqlite3_free_table = (sqlite3_free_table_fn)
		acl_dlsym(__sqlite_dll, "sqlite3_free_table");
	if (__sqlite3_free_table == NULL) {
		logger_fatal("load sqlite3_free_table from %s error: %s",
			path, acl_last_serror());
	}

	__sqlite3_busy_handler = (sqlite3_busy_handler_fn)
		acl_dlsym(__sqlite_dll, "sqlite3_busy_handler");
	if (__sqlite3_busy_handler == NULL) {
		logger_fatal("load sqlite3_busy_handler from %s error: %s",
			path, acl_last_serror());
	}

	__sqlite3_errmsg = (sqlite3_errmsg_fn)
		acl_dlsym(__sqlite_dll, "sqlite3_errmsg");
	if (__sqlite3_errmsg == NULL) {
		logger_fatal("load sqlite3_errmsg from %s error: %s",
			path, acl_last_serror());
	}

	__sqlite3_errcode = (sqlite3_errcode_fn)
		acl_dlsym(__sqlite_dll, "sqlite3_errcode");
	if (__sqlite3_errcode == NULL) {
		logger_fatal("load sqlite3_errcode from %s error: %s",
			path, acl_last_serror());
	}

	__sqlite3_changes = (sqlite3_changes_fn)
		acl_dlsym(__sqlite_dll, "sqlite3_changes");
	if (__sqlite3_changes == NULL) {
		logger_fatal("load sqlite3_changes from %s error: %s",
			path, acl_last_serror());
	}

	__sqlite3_total_changes = (sqlite3_total_changes_fn)
		acl_dlsym(__sqlite_dll, "sqlite3_total_changes");
	if (__sqlite3_total_changes == NULL) {
		logger_fatal("load sqlite3_total_changes from %s error: %s",
			path, acl_last_serror());
	}

	__sqlite3_busy_timeout = (sqlite3_busy_timeout_fn)
		acl_dlsym(__sqlite_dll, "sqlite3_busy_timeout");
	if (__sqlite3_busy_timeout == NULL) {
		logger_fatal("load sqlite3_busy_timeout from %s error: %s",
			path, acl_last_serror());
	}

	__sqlite3_prepare_v2 = (sqlite3_prepare_v2_fn)
		acl_dlsym(__sqlite_dll, "sqlite3_prepare_v2");
	if (__sqlite3_prepare_v2 == NULL) {
		logger_fatal("load sqlite3_prepare_v2 from %s error: %s",
			path, acl_last_serror());
	}

	__sqlite3_step = (sqlite3_step_fn)
		acl_dlsym(__sqlite_dll, "sqlite3_step");
	if (__sqlite3_step == NULL) {
		logger_fatal("load sqlite3_step from %s error: %s",
			path, acl_last_serror());
	}

	__sqlite3_exec = (sqlite3_exec_fn)
		acl_dlsym(__sqlite_dll, "sqlite3_exec");
	if (__sqlite3_exec == NULL) {
		logger_fatal("load sqlite3_exec from %s error: %s",
			path, acl_last_serror());
	}
	__sqlite3_reset = (sqlite3_reset_fn)
		acl_dlsym(__sqlite_dll, "sqlite3_reset");
	if (__sqlite3_reset == NULL) {
		logger_fatal("load sqlite3_reset from %s error: %s",
			path, acl_last_serror());
	}

	__sqlite3_bind_blob = (sqlite3_bind_blob_fn)
		acl_dlsym(__sqlite_dll, "sqlite3_bind_blob");
	if (__sqlite3_bind_blob == NULL) {
		logger_fatal("load sqlite3_bind_blob from %s error: %s",
			path, acl_last_serror());
	}

	__sqlite3_bind_int = (sqlite3_bind_int_fn)
		acl_dlsym(__sqlite_dll, "sqlite3_bind_int");
	if (__sqlite3_bind_int == NULL) {
		logger_fatal("load sqlite3_bind_int from %s error: %s",
			path, acl_last_serror());
	}

	__sqlite3_bind_int64 = (sqlite3_bind_int64_fn)
		acl_dlsym(__sqlite_dll, "sqlite3_bind_int64");
	if (__sqlite3_bind_int64 == NULL) {
		logger_fatal("load sqlite3_bind_int64 from %s error: %s",
			path, acl_last_serror());
	}

	__sqlite3_bind_text = (sqlite3_bind_text_fn)
		acl_dlsym(__sqlite_dll, "sqlite3_bind_text");
	if (__sqlite3_bind_text == NULL) {
		logger_fatal("load sqlite3_bind_text from %s error: %s",
			path, acl_last_serror());
	}

	__sqlite3_column_count = (sqlite3_column_count_fn)
		acl_dlsym(__sqlite_dll, "sqlite3_column_count");
	if (__sqlite3_column_count == NULL) {
		logger_fatal("load sqlite3_column_count from %s error: %s",
			path, acl_last_serror());
	}

	__sqlite3_column_type = (sqlite3_column_type_fn)
		acl_dlsym(__sqlite_dll, "sqlite3_column_type");
	if (__sqlite3_column_type == NULL) {
		logger_fatal("load sqlite3_column_type from %s error: %s",
			path, acl_last_serror());
	}

	__sqlite3_column_bytes = (sqlite3_column_bytes_fn)
		acl_dlsym(__sqlite_dll, "sqlite3_column_bytes");
	if (__sqlite3_column_bytes == NULL) {
		logger_fatal("load sqlite3_column_bytes from %s error: %s",
			path, acl_last_serror());
	}

	__sqlite3_column_name = (sqlite3_column_name_fn)
		acl_dlsym(__sqlite_dll, "sqlite3_column_name");
	if (__sqlite3_column_name == NULL) {
		logger_fatal("load sqlite3_column_name from %s error: %s",
			path, acl_last_serror());
	}

	__sqlite3_data_count = (sqlite3_data_count_fn)
		acl_dlsym(__sqlite_dll, "sqlite3_data_count");
	if (__sqlite3_data_count == NULL) {
		logger_fatal("load sqlite3_data_count from %s error: %s",
			path, acl_last_serror());
	}

	__sqlite3_column_int = (sqlite3_column_int_fn)
		acl_dlsym(__sqlite_dll, "sqlite3_column_int");
	if (__sqlite3_column_int == NULL) {
		logger_fatal("load sqlite3_column_int from %s error: %s",
			path, acl_last_serror());
	}

	__sqlite3_column_int64 = (sqlite3_column_int64_fn)
		acl_dlsym(__sqlite_dll, "sqlite3_column_int64");
	if (__sqlite3_column_int64 == NULL) {
		logger_fatal("load sqlite3_column_int64 from %s error: %s",
			path, acl_last_serror());
	}

	__sqlite3_column_double = (sqlite3_column_double_fn)
		acl_dlsym(__sqlite_dll, "sqlite3_column_double");
	if (__sqlite3_column_double == NULL) {
		logger_fatal("load sqlite3_column_double from %s error: %s",
			path, acl_last_serror());
	}

	__sqlite3_column_blob = (sqlite3_column_blob_fn)
		acl_dlsym(__sqlite_dll, "sqlite3_column_blob");
	if (__sqlite3_column_blob == NULL) {
		logger_fatal("load sqlite3_column_blob from %s error: %s",
			path, acl_last_serror());
	}

	__sqlite3_column_text = (sqlite3_column_text_fn)
		acl_dlsym(__sqlite_dll, "sqlite3_column_text");
	if (__sqlite3_column_text == NULL) {
		logger_fatal("load sqlite3_column_text from %s error: %s",
			path, acl_last_serror());
	}

	__sqlite3_finalize = (sqlite3_finalize_fn)
		acl_dlsym(__sqlite_dll, "sqlite3_finalize");
	if (__sqlite3_finalize == NULL) {
		logger_fatal("load sqlite3_finalize from %s error: %s",
			path, acl_last_serror());
	}

	__sqlite3_free = (sqlite3_free_fn)
		acl_dlsym(__sqlite_dll, "sqlite3_free");
	if (__sqlite3_free == NULL) {
		logger_fatal("load sqlite3_free from %s error: %s",
			path, acl_last_serror());
	}

	logger("%s loaded", path);
#ifndef HAVE_NO_ATEXIT
	atexit(__sqlite_dll_unload);
#endif
 }
# else
#  define __sqlite3_libversion sqlite3_libversion
#  define __sqlite3_open sqlite3_open
#  define __sqlite3_close sqlite3_close
#  define __sqlite3_get_table sqlite3_get_table
#  define __sqlite3_free_table sqlite3_free_table
#  define __sqlite3_busy_handler sqlite3_busy_handler
#  define __sqlite3_errmsg sqlite3_errmsg
#  define __sqlite3_errcode sqlite3_errcode
#  define __sqlite3_changes sqlite3_changes
#  define __sqlite3_total_changes sqlite3_total_changes
#  define __sqlite3_busy_timeout sqlite3_busy_timeout
#  define __sqlite3_prepare_v2 sqlite3_prepare_v2
#  define __sqlite3_step sqlite3_step
#  define __sqlite3_exec sqlite3_exec
#  define __sqlite3_reset sqlite3_reset
#  define __sqlite3_bind_blob sqlite3_bind_blob
#  define __sqlite3_bind_int sqlite3_bind_int
#  define __sqlite3_bind_int64 sqlite3_bind_int64
#  define __sqlite3_bind_text sqlite3_bind_text
#  define __sqlite3_column_count sqlite3_column_count
#  define __sqlite3_column_type sqlite3_column_type
#  define __sqlite3_column_bytes sqlite3_column_bytes
#  define __sqlite3_column_name sqlite3_column_name
#  define __sqlite3_data_count sqlite3_data_count
#  define __sqlite3_column_int sqlite3_column_int
#  define __sqlite3_column_int64 sqlite3_column_int64
#  define __sqlite3_column_double sqlite3_column_double
#  define __sqlite3_column_blob sqlite3_column_blob
#  define __sqlite3_column_text sqlite3_column_text
#  define __sqlite3_finalize sqlite3_finalize
#  define __sqlite3_free sqlite3_free
# endif // HAS_SQLITE && !HAS_SQLITE_DLL

namespace acl
{

//////////////////////////////////////////////////////////////////////////
// sqlite 的记录行类型定义

static void sqlite_rows_free(void* ctx)
{
	char** results = (char**) ctx;
#ifdef HAS_SQLITE_DLL
	if (__sqlite_dll && results) {
#else
	if (results) {
#endif
		__sqlite3_free_table(results);
	}
}

static void sqlite_rows_save(char** results, int nrow,
	int ncolumn, db_rows& result)
{
	int   n = 0;

	// 取出变量名
	for (; n < ncolumn; n++) {
		result.names_.push_back(results[n]);
	}

	// 开始取出所有行数据结果，加入动态数组中
	for (int i = 0; i < nrow; i++) {
		db_row* row = NEW db_row(result.names_);
		for (int j = 0; j < ncolumn; j++) {
			const char* value = results[n++];
			size_t len = strlen(value);
			row->push_back(value, len);
		}
		result.rows_.push_back(row);
	}
	result.result_tmp_ = results;
	result.result_free = sqlite_rows_free;
}

//////////////////////////////////////////////////////////////////////////

db_sqlite::db_sqlite(const char* dbfile, const char* charset /* ="utf-8" */)
: db_(NULL)
, dbfile_(dbfile)
{
	if (charset && strcasecmp(charset, "utf-8") !=0) {
		charset_ = charset;
		conv_ = NEW charset_conv();
	} else {
		conv_ = NULL;
	}

	acl_assert(dbfile && *dbfile);
#ifdef HAS_SQLITE_DLL
	acl_pthread_once(&__sqlite_once, __sqlite_dll_load);
#endif
}

db_sqlite::~db_sqlite(void)
{
	close();
	delete conv_;
	free_result();
}

const char* db_sqlite::version(void) const
{
	return __sqlite3_libversion();
}

static int sqlite_busy_callback(void *ctx acl_unused, int nretry acl_unused)
{
	acl_doze(10);
	return (1);
}

const char* db_sqlite::dbtype(void) const
{
	static const char* type = "sqlite";
	return type;
}

int db_sqlite::get_errno(void) const
{
	if (db_) {
		return __sqlite3_errcode(db_);
	} else {
		return -1;
	}
}

const char* db_sqlite::get_error(void) const
{
	if (db_) {
		return __sqlite3_errmsg(db_);
	} else {
		return "sqlite not opened yet!";
	}
}

bool db_sqlite::dbopen(const char* charset /* = NULL */)
{
	// 如果数据库已经打开，则直接返回 true
	if (db_ != NULL) {
		return true;
	}

	string buf;

	const char* ptr;

	if (charset != NULL && *charset != 0) {
		charset_ = charset;
	}

	// 转换成 [utf-8] 编码格式

	if (conv_ == NULL) {
		ptr = dbfile_.c_str();
	} else if (conv_->convert(charset_.c_str(), "utf-8",
		dbfile_.c_str(), dbfile_.length(), &buf) == false) {

		logger_error("charset convert(%s) from %s to utf-8 error",
			dbfile_.c_str(), charset_.c_str());
		return false;
	} else {
		ptr = buf.c_str();
	}

	string path;
	string& dir = path.dirname(dbfile_);

#ifdef ACL_WINDOWS
	if (!dir.empty() && dir != "." &&
		_access(dir.c_str(), 6) == -1 && errno == ENOENT) {
#else
	if (!dir.empty() && dir != "." &&
		access(dir.c_str(), R_OK | W_OK | X_OK) == -1 && errno == ENOENT) {
#endif
		if (acl_make_dirs(dir.c_str(), 0755) == -1) {
			logger_error("make dirs error %s, dir: %s",
				last_serror(), dir.c_str());
		}
	}

	// 打开 sqlite 数据库
	int   ret = __sqlite3_open(ptr, &db_);
	if (ret != SQLITE_OK) {
		logger_error("open %s error(%s, %d)", dbfile_.c_str(),
			__sqlite3_errmsg(db_), __sqlite3_errcode(db_));
		__sqlite3_close(db_);
		db_ = NULL;

		return false;
	}

	// 当 SQLITE 忙时的回调函数
	__sqlite3_busy_handler(db_, sqlite_busy_callback, NULL);

	// 关闭 SQLITE 实时刷新磁盘的特性，从而提高性能
	set_conf("PRAGMA synchronous = off");
	return true;
}

bool db_sqlite::is_opened(void) const
{
	return db_ != NULL ? true : false;
}

bool db_sqlite::close(void)
{
	if (db_ == NULL) {
		return false;
	}

	// 关闭 sqlite 数据库
	int   ret = __sqlite3_close(db_);
	if (ret == SQLITE_BUSY) {
		logger_error("close %s error SQLITE_BUSY", dbfile_.c_str());
		return false;
	}

	db_ = NULL;
	return true;
}

bool db_sqlite::set_conf(const char* pragma)
{
	bool ret = exec_sql(pragma);
	if (result_) {
		free_result();
	}
	return ret;
}

const char* db_sqlite::get_conf(const char* pragma, string& out)
{
	if (!exec_sql(pragma)) {
		return NULL;
	} else if (length() == 0) {
		free_result();
		return NULL;
	} else {
		const db_row* row = (*this)[(size_t) 0];
		acl_assert(row != NULL);
		const char* ptr = (*row)[(size_t) 0];
		if (ptr == NULL) {
			free_result();
			return NULL;
		}
		out = ptr;
		free_result();
		return out.c_str();
	}
}

// sqlite 的一些配置选项
const char* __pragmas[] =
{
	"PRAGMA auto_vacuum",
	"PRAGMA cache_size",
	"PRAGMA case_sensitive_like",
	"PRAGMA count_changes",
	"PRAGMA default_cache_size",
	"PRAGMA default_synchronous",
	"PRAGMA empty_result_callbacks",
	"PRAGMA encoding",
	"PRAGMA full_column_names",
	"PRAGMA fullfsync",
	"PRAGMA legacy_file_format",
	"PRAGMA locking_mode",
	"PRAGMA page_size",
	"PRAGMA max_page_count",
	"PRAGMA read_uncommitted",
	"PRAGMA short_column_names",
	"PRAGMA synchronous",
	"PRAGMA temp_store",
	"PRAGMA temp_store_directory",
	NULL
};

void db_sqlite::show_conf(const char* pragma /* = NULL */)
{
	if (db_ == NULL) {
		logger_error("db not open yet!");
		return;
	}

	string buf;

	if (pragma != NULL) {
		if (get_conf(pragma, buf) != NULL) {
			printf("%s: %s\r\n", pragma, buf.c_str());
		} else {
			printf("%s: UNKNOWN\r\n", pragma);
		}
		return;
	}

	int   i;
	for (i = 0; __pragmas[i] != NULL; i++) {
		if (get_conf(__pragmas[i], buf) != NULL) {
			printf("%s: %s\r\n", __pragmas[i], buf.c_str());
		} else {
			printf("%s: UNKNOWN\r\n", __pragmas[i]);
		}
	}
}

bool db_sqlite::tbl_exists(const char* tbl_name)
{
	if (tbl_name == NULL || *tbl_name == 0) {
		logger_error("tbl_name null");
		return false;
	}

	acl::string sql;
	sql.format("select count(*) from sqlite_master"
		" where type='table' and name='%s'", tbl_name);

	if (!exec_sql(sql.c_str())) {
		free_result();
		return false;
	}

	if (length() == 0) {
		free_result();
		return false;
	} else {
		const db_row* row = (*this)[0];
		acl_assert(row != NULL);

		int  n = row->field_int((size_t) 0, (int) 0);
		free_result();

		if (n == 0) {
			return false;
		}
		return true;
	}
}

bool db_sqlite::sql_select(const char* sql, db_rows* result /* = NULL */)
{
	return exec_sql(sql, result);
}

bool db_sqlite::sql_update(const char* sql)
{
	return exec_sql(sql);
}

bool db_sqlite::exec_sql(const char* sql, db_rows* result /* = NULL */)
{
	// 必须将上次的查询结果删除
	free_result();

	if (sql == NULL || *sql == 0) {
		logger_error("invalid params");
		return false;
	} else if (db_ == NULL) {
		logger_error("db not open yet!");
		return false;
	}

	char** results = NULL, *err;
	int    nrow, ncolumn;

	// 执行 sqlite 的查询过程
	int   ret = __sqlite3_get_table(db_, sql, &results, &nrow, &ncolumn, &err);
	if (ret != SQLITE_OK) {
		logger_error("sqlites_get_table(%s) error(%s)",
			sql, __sqlite3_errmsg(db_));
		__sqlite3_free_table(results);
		return false;
	}

	if (nrow > 0) {
		if (result != NULL) {
			sqlite_rows_save(results, nrow, ncolumn, *result);
		} else {
			result_ = NEW db_rows();
			sqlite_rows_save(results, nrow, ncolumn, *result_);
		}
	} else if (results) {
		result_ = NULL;
		__sqlite3_free_table(results);
	}

	return true;
}

int db_sqlite::affect_count(void) const
{
	if (db_ == NULL) {
		logger_error("db not opened yet!");
		return -1;
	}

	return __sqlite3_changes(db_);
}

int db_sqlite::affect_total_count(void) const
{
	return __sqlite3_total_changes(db_);
}

bool db_sqlite::begin_transaction(void)
{
	const char* sql = "begin transaction;";
	if (sql_update(sql) == false) {
		logger_error("%s error: %s", sql, get_error());
		return false;
	}
	return true;
}

bool db_sqlite::commit(void)
{
	const char* sql = "commit transaction;";
	if (sql_update(sql) == false) {
		logger_error("%s error: %s", sql, get_error());
		return false;
	}
	return true;
}

bool db_sqlite::set_busy_timeout(int nMillisecs)
{
	int   ret = __sqlite3_busy_timeout(db_,nMillisecs);
	return ret == SQLITE_OK;
}

bool db_sqlite::prepare(sqlite_cursor& cursor)
{
	const string& sql = cursor.get_sql();
	int ret = __sqlite3_prepare_v2(db_, sql.c_str(), -1,
			&cursor.stmt_, NULL);
	if (ret != SQLITE_OK) {
		logger_error("prepare error=%s, sql=%s",
			get_error(), sql.c_str());
		return false;
	}

	cursor.free_callback = __sqlite3_finalize;

	int n = __sqlite3_column_count(cursor.stmt_);
	if (n <= 0) {
		logger_error("invalid column count=%d", n);
		return false;
	}
	for (int i = 0; i < n; i++) {
		const char* name = __sqlite3_column_name(cursor.stmt_, i);
		if (name == NULL) {
			logger_error("column name null, i=%d, sql=%s",
				i, cursor.get_sql().c_str());
			return false;
		}
		cursor.add_column_name(name);
	}
	cursor.create_row();
	return true;
}

bool db_sqlite::next(sqlite_cursor& cursor, bool* done)
{
	cursor.clear();

	if (done) {
		*done = false;
	}

	int ret = __sqlite3_step(cursor.stmt_);
	switch (ret) {
	case SQLITE_DONE:
		if (done) {
			*done = true;
		}
		return true;
	case SQLITE_BUSY:
		logger_error("SQLITE_BUSY now, error=%s", get_error());
		return false;
	case SQLITE_ERROR:
		logger_error("SQLITE_BUSY now, error=%s", get_error());
		return false;
	case SQLITE_ROW:
		break;
	default:
		logger_error("unknown type=%d, error=%s", ret, get_error());
		return false;
	}

	int columns = __sqlite3_data_count(cursor.stmt_);
	if (columns != (int) cursor.names_.size()) {
		logger_error("invalid columns=%d, names count=%d",
			columns, (int) cursor.names_.size());
		return false;
	}

	for (int i = 0; i < columns; i++) {
		int type = __sqlite3_column_type(cursor.stmt_, i);
		long long i64;
		double ifloat;
		const unsigned char* itxt;

		switch (type) {
		case SQLITE_INTEGER:
			i64 = __sqlite3_column_int64(cursor.stmt_, i);
			cursor.add_column_value(i64);
			break;
		case SQLITE_FLOAT:
			ifloat = __sqlite3_column_double(cursor.stmt_, i);
			cursor.add_column_value(ifloat);
			break;
		case SQLITE_TEXT:
			itxt = __sqlite3_column_text(cursor.stmt_, i);
			cursor.add_column_value((const char*) itxt);
			break;
		default:
			logger_warn("not support type=%d", type);
			break;
		}
	}
	return true;
}

int db_sqlite::sqlite3_prepare_v2(const char *zSql,
	int nByte, sqlite3_stmt **ppStmt, const char **pzTail)
{
	return __sqlite3_prepare_v2(db_, zSql, nByte, ppStmt, pzTail);
}

int db_sqlite::sqlite3_step(sqlite3_stmt *stmt)
{
	return __sqlite3_step(stmt);
}

int db_sqlite::sqlite3_reset(sqlite3_stmt *pStmt)
{
	return __sqlite3_reset(pStmt);
}

int db_sqlite::sqlite3_finalize(sqlite3_stmt *pStmt)
{
	return __sqlite3_finalize(pStmt);
}

int db_sqlite::sqlite3_bind_blob(sqlite3_stmt *stmt, int iCol,
	const void *value, int n, void(*destory)(void*))
{
	return __sqlite3_bind_blob(stmt, iCol, value, n, destory);
}

int db_sqlite::sqlite3_bind_int(sqlite3_stmt *stmt, int iCol, int value)
{
	return __sqlite3_bind_int(stmt, iCol, value);
}

int db_sqlite::sqlite3_bind_int64(sqlite3_stmt *stmt, int iCol, int64_t value)
{
	return __sqlite3_bind_int64(stmt, iCol, value);
}

int db_sqlite::sqlite3_bind_text(sqlite3_stmt *stmt, int iCol,
	const char *value, int n, void(*destory)(void*))
{
	return __sqlite3_bind_text(stmt, iCol, value, n, destory);
}

int db_sqlite::sqlite3_column_count(sqlite3_stmt *pStmt)
{
	return __sqlite3_column_count(pStmt);
}

const void* db_sqlite::sqlite3_column_blob(sqlite3_stmt *stmt, int iCol)
{
	return __sqlite3_column_blob(stmt, iCol);
}

int db_sqlite::sqlite3_column_int(sqlite3_stmt *stmt, int iCol)
{
	return __sqlite3_column_int(stmt, iCol);
}

int64_t db_sqlite::sqlite3_column_int64(sqlite3_stmt *stmt, int iCol)
{
	return __sqlite3_column_int64(stmt, iCol);
}

const unsigned char* db_sqlite::sqlite3_column_text( sqlite3_stmt *stmt, int iCol)
{
	return __sqlite3_column_text(stmt, iCol);
}

int db_sqlite::sqlite3_column_bytes(sqlite3_stmt *stmt, int iCol)
{
	return __sqlite3_column_bytes(stmt, iCol);
}

const char* db_sqlite::sqlite3_column_name(sqlite3_stmt *stmt, int iCol)
{
	return __sqlite3_column_name(stmt, iCol);
}

int db_sqlite::sqlite3_exec(const char *sql,
	int(*callback)(void*,int,char**,char**), void *arg, char **errmsg)
{
	return __sqlite3_exec(db_, sql, callback, arg, errmsg);
}

void db_sqlite::sqlite3_free(void* ptr)
{
	__sqlite3_free(ptr);
}

} // namespace acl

#else

namespace acl
{

db_sqlite::db_sqlite(const char*, const char*) {}
db_sqlite::~db_sqlite(void) {}
const char* db_sqlite::version(void) const { return NULL; }
bool db_sqlite::set_conf(const char*) { return false; }
const char* db_sqlite::get_conf(const char*, string&) { return NULL; }
void db_sqlite::show_conf(const char*) {}
int db_sqlite::affect_total_count() const { return 0; }
bool db_sqlite::begin_transaction(void) { return false; }
bool db_sqlite::commit(void) { return false; }
const char* db_sqlite::dbtype() const { return NULL; }
bool db_sqlite::dbopen(const char*) { return false; }
bool db_sqlite::is_opened() const { return false; }
bool db_sqlite::close(void) { return false; }
bool db_sqlite::tbl_exists(const char*) { return false; }
bool db_sqlite::sql_select(const char*, db_rows*) { return false; }
bool db_sqlite::sql_update(const char*) { return false; }
int db_sqlite::affect_count() const { return 0; }
int db_sqlite::get_errno() const { return -1; }
const char* db_sqlite::get_error() const { return "unknown"; }
bool db_sqlite::prepare(sqlite_cursor&) { return false; }
bool db_sqlite::next(sqlite_cursor&, bool*) { return false; }
int db_sqlite::sqlite3_prepare_v2(const char*, int,
        sqlite3_stmt**, const char**){ return SQLITE_ERROR; }
int db_sqlite::sqlite3_step(sqlite3_stmt*) { return SQLITE_ERROR; }
int db_sqlite::sqlite3_reset(sqlite3_stmt*) { return SQLITE_ERROR;  }
int db_sqlite::sqlite3_finalize(sqlite3_stmt*){ return SQLITE_ERROR; }
int db_sqlite::sqlite3_bind_blob(sqlite3_stmt*, int,
        const void*, int, void(*)(void*)) { return SQLITE_ERROR; }
int db_sqlite::sqlite3_bind_int(sqlite3_stmt*, int, int) {
    return SQLITE_ERROR; }
int db_sqlite::sqlite3_bind_int64(sqlite3_stmt*, int, int64_t){
    return SQLITE_ERROR; }
int db_sqlite::sqlite3_bind_text(sqlite3_stmt*, int,
        const char*, int, void(*)(void*)) { return SQLITE_ERROR; }
int db_sqlite::sqlite3_column_count(sqlite3_stmt *pStmt) { return 0; }
const void* db_sqlite::sqlite3_column_blob(sqlite3_stmt*, int) { return NULL; }
int db_sqlite::sqlite3_column_int(sqlite3_stmt *stmt, int iCol) { return -1; }
int64_t db_sqlite::sqlite3_column_int64(sqlite3_stmt*, int) { return -1; }
const unsigned char* db_sqlite::sqlite3_column_text(
        sqlite3_stmt *, int) { return NULL; }
int db_sqlite::sqlite3_column_bytes(sqlite3_stmt *, int) { return 0; }
const char* db_sqlite::sqlite3_column_name(sqlite3_stmt*, int) { return NULL; }
int db_sqlite::sqlite3_exec(const char*,int(*)(void*,int,char**,char**),
        void *, char **) { return SQLITE_ERROR; }
void db_sqlite::sqlite3_free(void* ptr) {  }
}  // namespace acl

#endif // !HAS_SQLITE && !HAS_SQLITE_DLL
#endif // !defined(ACL_DB_DISABLE)
