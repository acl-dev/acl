#pragma once
#include "../acl_cpp_define.hpp"
#include "istream.hpp"
#include "ostream.hpp"

namespace acl {

class string;

class ACL_CPP_API fstream : public istream , public ostream {
public:
	fstream();
	virtual ~fstream();

	/**
	 * Open file stream based on file path. This is the most basic way to open a file
	 * @param path {const char*} Filename
	 * @param oflags {unsigned int} Flag bits. We're assuming that
	 *  O_RDONLY: 0x0000, O_WRONLY: 0x0001, O_RDWR: 0x0002,
	 *  O_APPEND: 0x0008, O_CREAT: 0x0100, O_TRUNC: 0x0200,
	 *  O_EXCL: 0x0400; just for win32, O_TEXT: 0x4000,
	 *  O_BINARY: 0x8000, O_RAW: O_BINARY, O_SEQUENTIAL: 0x0020,
	 *  O_RANDOM: 0x0010.
	 * @param mode {int} Mode when opening file handle (e.g.: 0600)
	 * @return {bool} Whether opening file was successful
	 */
	bool open(const char* path, unsigned int oflags, int mode);

	/**
	 * Open file stream in read/write mode. When file does not exist, create new file. When file exists, then
	 * clear file content. File attributes are 0700
	 * @param path {const char*} Filename
	 * @return {bool} Whether opening file was successful
	 */
	bool open_trunc(const char* path);

	/**
	 * Create new file in read/write mode. File attributes are 0700. If file does not exist, create new file.
	 * If exists, open old file
	 * @return {bool} Whether file creation was successful
	 */
	bool create(const char* path);

	/**
	 * Delete file corresponding to this class object from disk. This function can only correctly delete file when internally knows file path,
	 * otherwise cannot delete
	 * @return {bool} Whether file deletion was successful
	 */
	bool remove();

	/**
	 * Rename current file to specified filename. For WINDOWS platform, need to close current file
	 * handle first. After rename succeeds, reopen new target file
	 * @param from_path {const char*} Source filename
	 * @param to_path {const char*} Target filename
	 * @return {bool} Whether rename was successful
	 */
	bool rename(const char* from_path, const char* to_path);

#if defined(_WIN32) || defined(_WIN64)
	/**
	 * Open fstream file stream object based on system file handle
	 * @param fh System file handle
	 * @param oflags Open flag bits
	 * @param path {const char*} When not NULL, stored as file path of this file handle
	 *  for use by file_path, remove
	 */
	void open(void* fh, unsigned int oflags, const char* path = NULL);

	/**
	 * Move file pointer position
	 * @param offset {__int64} Offset
	 * @param whence {int} Move direction: SEEK_SET (move from file start position),
	 *  SEEK_CUR (move from current file pointer position), SEEK_END (move from file end forward)
	 * @return {acl_off_t} Returns value >= 0 normally, returns -1 on error
	 */
	__int64 fseek(__int64 offset, int whence);

	/**
	 * Get offset position of current system file pointer in file
	 * @return {acl_off_t} Returns value >= 0 normally, returns -1 on error
	 */
	__int64 ftell(void);

	/**
	 * Truncate file size to specified size
	 * @param length {acl_off_t} File size after truncation
	 * @return {bool} Whether successful
	 */
	bool ftruncate(__int64 length);

	/**
	 * Get size of current file
	 * @return {acl_off_t} Returns value >= 0 normally, returns -1 on error
	 */
	__int64 fsize(void) const;

	/**
	 * Static method for getting file size of specified file
	 * @param path {const char*} Non-empty string specifying file path
	 * @return {__int64} Returns value >= 0 normally, returns -1 on error
	 */
	static __int64 fsize(const char* path);

	/**
	 * Return system file handle
	 * @return System file handle, returns ACL_FILE_INVALID on error
	 */
	void* file_handle(void) const;
#else
	void open(int fh, unsigned int oflags, const char* path = NULL);
	long long int fseek(long long int offset, int whence);
	long long int ftell();
	bool ftruncate(long long int length);
	long long int fsize() const;
	static long long int fsize(const char* path);
	int file_handle() const;
#endif
	/**
	 * Get full path of file
	 * @return {const char*} Returns NULL indicates file has not been opened yet or error occurred
	 */
	const char* file_path() const;

	/**
	 * After file is opened, this method is used to lock file
	 * @param exclude {bool} Whether locking method is exclusive lock. Default is exclusive lock. If this value
	 *  is false, it is shared lock
	 * @return {bool} Returns false indicates locking failed. Can check error reason through acl::last_serror
	 */
	bool lock(bool exclude = true);

	/**
	 * After file is opened, this method is used to try to lock file. If locking succeeds, returns true. If
	 * this file has already been locked by another process, returns false
	 * @param exclude {bool} Whether locking method is exclusive lock. Default is exclusive lock. If this value
	 *  is false, it is shared lock
	 * @return {bool} Returns false indicates locking failed. Can check error reason through acl::last_serror
	 */
	bool try_lock(bool exclude = true);

	/**
	 * After successfully calling lock or try_lock, can call this method to unlock file
	 * @return {bool} Whether unlock was successful
	 */
	bool unlock();
};

} // namespace acl

