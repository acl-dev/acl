#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stdlib/scan_dir.hpp"
#endif

#ifdef ACL_UNIX
# include <dirent.h>
# include <unistd.h>
#elif	defined(ACL_WINDOWS)
# if	defined(ACL_BCB_COMPILER)
#  include <dirent.h>
# else
#  include <direct.h>
# endif
#endif

namespace acl
{

scan_dir::scan_dir(void)
: path_(NULL)
, scan_(NULL)
, path_buf_(NULL)
, file_buf_(NULL)
{
}

scan_dir::~scan_dir(void)
{
	close();
	delete path_buf_;
	delete file_buf_;
}

void scan_dir::close(void)
{
	if (path_) {
		acl_myfree(path_);
		path_ = NULL;
	}
	if (scan_) {
		acl_scan_dir_close(scan_);
		scan_ = NULL;
	}
}

bool scan_dir::open(const char* path, bool recursive /* = true */,
	bool rmdir_on /* = false */)
{
	if (path == NULL || *path == 0) {
		logger_error("path null");
		return false;
	}

	// 先关闭之前可能打开的句柄，以防止资源泄漏
	close();

	unsigned flags = 0;
	if (recursive) {
		flags |= ACL_SCAN_FLAG_RECURSIVE;
	}
	if (rmdir_on) {
		flags |= ACL_SCAN_FLAG_RMDIR;
	}

	path_ = acl_mystrdup(path);
	scan_ = acl_scan_dir_open2(path_, flags);
	if (scan_ == NULL) {
		logger_error("open dir: %s error: %s", path_, last_serror());
		acl_myfree(path_);
		return false;
	}

	if (rmdir_on) {
		set_rmdir_callback(rmdir_def, this);
	}

	return true;
}

void scan_dir::set_rmdir_callback(int (*fn)(ACL_SCAN_DIR*, const char*, void*),
	void* ctx)
{
	if (scan_ == NULL) {
		logger_error("please call scan_dir::open first!");
		return;
	}

	acl_scan_dir_ctl(scan_, ACL_SCAN_CTL_RMDIR_FN, fn,
		ACL_SCAN_CTL_CTX, ctx, ACL_SCAN_CTL_END);
}

int scan_dir::rmdir_def(ACL_SCAN_DIR*, const char* path, void* ctx)
{
	scan_dir* me = (scan_dir*) ctx;
	return me->rmdir_callback(path) ? 0 : -1;
}

bool scan_dir::rmdir_callback(const char* path)
{
#ifdef ACL_WINDOWS
	if (_rmdir(path) == 0) {
#else
	if (rmdir(path) == 0) {
#endif
		logger("rmdir ok, path=%s", path);
		return true;
	} else {
		logger_error("rmdir error=%s, path=%s", last_serror(), path);
		return false;
	}
}

const char* scan_dir::next_file(bool full /* = false */)
{
	if (scan_ == NULL) {
		return NULL;
	}

	const char* file =  acl_scan_dir_next_file(scan_);
	if (file == NULL || *file == 0) {
		return NULL;
	}
	if (!full) {
		return file;
	}

	const char* path = curr_path();
	if (path == NULL) {
		return NULL;
	}

	if (file_buf_ == NULL) {
		file_buf_ = NEW string(256);
	}


#ifdef	ACL_WINDOWS
	file_buf_->format("%s%c%s", path, PATH_SEP_C, file);
#else
	if (*path == '/' && *(path + 1) == 0) {
		file_buf_->format("%s%s", path, file);
	} else {
		file_buf_->format("%s%c%s", path, PATH_SEP_C, file);
	}
#endif

	return file_buf_->c_str();
}

const char* scan_dir::next_dir(bool full /* = false */)
{
	if (scan_ == NULL) {
		return NULL;
	}

	const char* dir = acl_scan_dir_next_dir(scan_);
	if (dir == NULL || *dir == 0) {
		return NULL;
	}
	if (!full) {
		return dir;
	}

	const char* path = curr_path();
	if (path == NULL) {
		return NULL;
	}

	if (file_buf_ == NULL) {
		file_buf_ = NEW string(256);
	}

#ifdef	ACL_WINDOWS
	file_buf_->format("%s%c%s", path, PATH_SEP_C, dir);
#else
	if (*path == '/' && *(path + 1) == 0) {
		file_buf_->format("%s%s", path, dir);
	} else {
		file_buf_->format("%s%c%s", path, PATH_SEP_C, dir);
	}
#endif

	return file_buf_->c_str();
}

const char* scan_dir::next(bool full /* = false */, bool* is_file /* = NULL */)
{
	if (scan_ == NULL) {
		return NULL;
	}

	int res;
	const char* name = acl_scan_dir_next_name(scan_, &res);
	if (name == NULL || *name == 0) {
		return NULL;
	}

	if (is_file) {
		*is_file = res ? true : false;
	}
	if (!full) {
		return name;
	}

	const char* path = curr_path();
	if (path == NULL) {
		return NULL;
	}

	if (file_buf_ == NULL) {
		file_buf_ = NEW string(256);
	}

	(*file_buf_) = path;
	if (!res) {
		return file_buf_->c_str();
	}

#ifdef	ACL_WINDOWS
	file_buf_->format_append("%c%s", PATH_SEP_C, name);
#else
	if (*path == '/' && *(path + 1) == 0) {
		file_buf_->format_append("%s%s", path, name);
	} else {
		file_buf_->format_append("%c%s", PATH_SEP_C, name);
	}
#endif

	return file_buf_->c_str();
}

const char* scan_dir::curr_path(void)
{
	if (scan_ == NULL) {
		return NULL;
	}

	return acl_scan_dir_path(scan_);
}

bool scan_dir::get_cwd(string& out)
{

#ifndef MAX_PATH
#define MAX_PATH	1024
#endif
	char  buf[MAX_PATH];

#ifdef ACL_WINDOWS
	if (::GetCurrentDirectory(MAX_PATH, buf) == 0) {
		logger_error("can't get process path: %s", last_serror());
		return false;
	}
#else
	if (::getcwd(buf, sizeof(buf)) == NULL) {
		logger_error("can't get process path: %s", last_serror());
		return false;
	}
#endif // ACL_WINDOWS

	// xxx: can this happen ?
	if (buf[0] == 0) {
		return false;
	}

	// 去掉尾部的 '/'
	char* end = buf + strlen(buf) - 1;
	while (end > buf) {
#ifdef ACL_WINDOWS
		if (*end == '/' || *end == '\\') {
			end--;
		}
#else
		if (*end == '/') {
			end--;
		}
#endif
		else {
			break;
		}
	}

	out = buf;
	return true;
}

const char* scan_dir::curr_file(bool full /* = false */)
{
	if (scan_ == NULL) {
		return NULL;
	}
	const char* ptr = acl_scan_dir_file(scan_);
	if (ptr == NULL || *ptr == 0) {
		return NULL;
	}
	if (!full) {
		return ptr;
	}

	const char* path = curr_path();
	if (path == NULL) {
		return NULL;
	}

	if (file_buf_ == NULL) {
		file_buf_ = NEW string(256);
	}
	file_buf_->format("%s%c%s", path, PATH_SEP_C, ptr);

	return file_buf_->c_str();
}

size_t scan_dir::dir_count(void) const
{
	if (scan_ == NULL) {
		return 0;
	}
	return acl_scan_dir_ndirs(scan_);
}

size_t scan_dir::file_count(void) const
{
	if (scan_ == NULL) {
		return 0;
	}
	return acl_scan_dir_nfiles(scan_);
}

acl_uint64 scan_dir::scaned_size(void) const
{
	if (scan_ == NULL) {
		return 0;
	}
	acl_uint64 n = (acl_uint64) acl_scan_dir_nsize(scan_);
	return n;
}

acl_uint64 scan_dir::all_size(int* nfiles /* = NULL */,
	int* ndirs /* = NULL */) const
{
	if (scan_ == NULL) {
		return 0;
	}
	return acl_scan_dir_size2(scan_, nfiles, ndirs);
}

acl_uint64 scan_dir::all_size(const char* path, bool recursive /* = true */,
	int* nfiles /* = NULL */, int* ndirs /* = NULL */)
{
	scan_dir scan;
	if (scan.open(path, recursive) == false) {
		return 0;
	}
	return scan.all_size(nfiles, ndirs);
}

acl_uint64 scan_dir::remove_all(int* nfiles /* = NULL */,
	int* ndirs /* = NULL */) const
{
	if (scan_ == NULL) {
		return 0;
	}
	return (acl_uint64) acl_scan_dir_rm2(scan_, nfiles, ndirs);
}

acl_uint64 scan_dir::remove_all(const char* path, bool recursive /* = true */,
	int* nfiles /* = NULL */, int* ndirs /* = NULL */)
{
	scan_dir scan;
	if (scan.open(path, recursive) == false) {
		return 0;
	}
	return scan.remove_all(nfiles, ndirs);
}

}  // namespace acl
