#pragma once
#include "noncopyable.hpp"

struct ACL_SCAN_DIR;

namespace acl {

class string;

class ACL_CPP_API scan_dir : public noncopyable {
public:
	scan_dir();
	virtual ~scan_dir();

	/**
	 * Open directory
	 * @param path {const char*} Directory path, non-NULL pointer
	 * @param recursive {bool} Whether to allow recursive directory scanning
	 * @param rmdir_on {bool} When directory is empty, whether to delete this empty directory
	 * @return {bool} Whether opening directory was successful
	 */
	bool open(const char* path, bool recursive = true, bool rmdir_on = false);

	/**
	 * Virtual method. When need to delete empty directory, subclasses can implement this virtual method to delete passed directory.
	 * This virtual method internally automatically calls rmdir to delete empty directory
	 * @param path {const char*} Empty directory to be deleted
	 * @return {bool} Whether deleting directory was successful
	 */
	virtual bool rmdir_callback(const char* path);

	/**
	 * Close directory, and release internal resources
	 */
	void close();

	/**
	 * Scan next file (automatically skips directories). When recursive scanning option is specified in open
	 * (i.e., recursive = true), this function will recursively scan all subdirectories of opened directory
	 * @param full {bool} Whether to return full file path
	 * @return {const char*} Non-NULL indicates scanned filename, otherwise indicates scanning complete
	 *  or directory has not been opened yet
	 */
	const char* next_file(bool full = false);

	/**
	 * Scan next directory (skips files, ".", and ".."). When recursive scanning option is specified in open
	 * (i.e., recursive = true), this function will recursively scan all subdirectories of opened directory
	 * @param full {bool} Whether to return full directory path
	 * @return {const char*} Non-NULL indicates scanned directory name, otherwise indicates scanning complete
	 *  or directory has not been opened yet
	 */
	const char* next_dir(bool full = false);

	/**
	 * Scan next directory or file. When recursive scanning option is specified in open (i.e., recursive
	 * = true), this function will recursively scan all subdirectories and files of opened directory
	 * @param full {bool} Whether to return full path of directory or file. If true, returns
	 *  full path, otherwise only returns filename or directory name, both without path
	 * @param is_file {bool*} When return result is not empty, value stored at this address indicates whether scanned
	 *  item is a file. If true, it is a file, otherwise it is a directory
	 * @return {const char*} Non-NULL indicates scanned directory name or filename, otherwise indicates
	 *  scanning complete or directory has not been opened yet
	 */
	const char* next(bool full = false, bool* is_file = NULL);

	/**
	 * Get directory path where current scanning process is located. Returned path tail does not contain path separator '/' or '\\' (win32).
	 * For example, for path: /home/zsx/, will return /home/zsx. If path is root path: /, then this '/' will be retained.
	 * On _WIN32, returns path like C:\Users\zsx
	 * @return {const char*} When directory is open, this function returns non-NULL pointer, otherwise returns NULL
	 */
	const char* curr_path();

	/**
	 * Get filename scanned by current program scanning process
	 * @param full {bool} Whether to also return full file path
	 * @return {bool} Returns NULL if directory is not opened or current scan is not a file
	 */
	const char* curr_file(bool full = false);

	/**
	 * Return number of directories already scanned
	 * @return {size_t}
	 */
	size_t dir_count() const;

	/**
	 * Return number of files already scanned
	 * @return {size_t}
	 */
	size_t file_count() const;

#if defined(_WIN32) || defined(_WIN64)
	/**
	 * Get total size of files and directories already scanned
	 * @return {acl_uint64}
	 */
	unsigned __int64 scaned_size() const;

	/**
	 * After open succeeds, call this function to count total size of all files and directories under root directory
	 * @param nfiles {int*} When not empty, stores number of deleted files
	 * @param ndirs {int*} When not empty, stores number of deleted directories
	 * @return {acl_uint64} Total size of all files and directories under root directory
	 */
	unsigned __int64 all_size(int* nfiles = NULL, int* ndirs = NULL) const;

	/**
	 * Total size of all files and directories under root directory
	 * @param path {const char*} Root directory to be deleted
	 * @param recursive {bool} Whether to recursively scan all directories
	 * @param nfiles {int*} When not empty, stores number of deleted files
	 * @param ndirs {int*} When not empty, stores number of deleted directories
	 * @return {acl_uint64} Total size of all files and directories under root directory
	 */
	static unsigned __int64 all_size(const char* path, bool recursive = true,
		int* nfiles = NULL, int* ndirs = NULL);

	/**
	 * After open succeeds, call this function to delete all files and directories under root directory
	 * @param nfiles {int*} When not empty, stores number of deleted files
	 * @param ndirs {int*} When not empty, stores number of deleted directories
	 * @return {acl_unint64} Total size of all deleted files and directories (in bytes)
	 */
	unsigned __int64 remove_all(int* nfiles = NULL, int* ndirs = NULL) const;

	/**
	 * Delete all files and directories under root directory
	 * @param path {const char*} Root directory to be deleted
	 * @param recursive {bool} Whether to recursively scan all directories
	 * @param nfiles {int*} When not empty, stores number of deleted files
	 * @param ndirs {int*} When not empty, stores number of deleted directories
	 * @return {acl_unint64} Total size of all deleted files and directories (in bytes)
	 */
	static unsigned __int64 remove_all(const char* path, bool recursive = true,
		int* nfiles = NULL, int* ndirs = NULL);
#else
	unsigned long long scaned_size() const;
	unsigned long long all_size(int* nfiles = NULL, int* ndirs = NULL) const;
	static unsigned long long all_size(const char* path, bool recursive = true,
		int* nfiles = NULL, int* ndirs = NULL);
	unsigned long long remove_all(int* nfiles = NULL, int* ndirs = NULL) const;
	static unsigned long long remove_all(const char* path, bool recursive = true,
		int* nfiles = NULL, int* ndirs = NULL);
#endif

	/**
	 * Get current program's running path
	 * @param out {string&} Store result
	 * @return {bool} Whether successfully obtained current program's running path
	 */
	static bool get_cwd(string& out);

public:
	ACL_SCAN_DIR* get_scan_dir() const {
		return scan_;
	}

	/**
	 * Set callback method for deleting empty directories
	 * @param fn {int (*)(ACL_SCAN_DIR*, const char*, void*)}
	 * @param ctx {void*}
	 */
	void set_rmdir_callback(int (*fn)(ACL_SCAN_DIR*, const char*, void*), void* ctx);

private:
	char* path_;
	ACL_SCAN_DIR* scan_;
	string* path_buf_;
	string* file_buf_;

	static int rmdir_def(ACL_SCAN_DIR* scan, const char* path, void* ctx);
};

}  // namespace acl

