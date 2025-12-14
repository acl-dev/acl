#pragma once
#include "../acl_cpp_define.hpp"
#include "aio_istream.hpp"
#include "aio_ostream.hpp"

namespace acl {

class fstream;

/**
 * Asynchronous file read/write stream. Objects of this class can only be used in UNIX systems
 */
class ACL_CPP_API aio_fstream : public aio_istream , public aio_ostream {
public:
	/**
	 * Constructor
	 * @param handle {aio_handle*} Asynchronous event handle
	 */
	explicit aio_fstream(aio_handle* handle);

#if defined(_WIN32) || defined(_WIN64)
	aio_fstream(aio_handle* handle, HANDLE fd, unsigned int oflags = 0600);
#else
	aio_fstream(aio_handle* handle, int fd, unsigned int oflags = 0600);
#endif

	/**
	 * Open file stream based on file path. This is the most basic way to open a file
	 * @param path {const char*} Filename
	 * @param oflags {unsigned int} Flag bits. We're assuming that O_RDONLY: 0x0000,
	 *  O_WRONLY: 0x0001, O_RDWR: 0x0002, O_APPEND: 0x0008, O_CREAT: 0x0100,
	 *  O_TRUNC: 0x0200, O_EXCL: 0x0400; just for win32, O_TEXT: 0x4000,
	 *  O_BINARY: 0x8000, O_RAW: O_BINARY, O_SEQUENTIAL: 0x0020, O_RANDOM: 0x0010.
	 * @param mode {int} Mode when opening file handle (e.g.: 0600)
	 * @return {bool} Whether opening file was successful
	 */
	bool open(const char* path, unsigned int oflags, unsigned int mode);

	/**
	 * Open file stream in read/write mode. When file does not exist, create new file. When file exists, then
	 * clear file content. File attributes are 0700
	 * @param path {const char*} Filename
	 * @param mode {int} Mode when opening file handle (e.g.: 0600)
	 * @return {bool} Whether opening file was successful
	 */
	bool open_trunc(const char* path, unsigned int mode = 0600);

	/**
	 * Create new file in read/write mode. File attributes are 0600. If file does not exist, create new file. If exists, then
	 * open old file
	 * @param path {const char*} Full file path
	 * @param mode {int} Mode when opening file handle (e.g.: 0600)
	 * @return {bool} Whether file creation was successful
	 */
	bool create(const char* path, unsigned int mode = 0600);

	/**
	 * Open existing file in read-only mode
	 * @param path {const char*} Filename
	 * @return {bool} Whether opening file was successful
	 */
	bool open_read(const char* path);

	/**
	 * Open file in write-only mode. If file does not exist, create new file. If file
	 * exists, then clear file content
	 * @param path {const char*} Filename
	 * @return {bool} Whether successful
	 */
	bool open_write(const char* path);

	/**
	 * Open file in append mode. If file does not exist, create new file
	 * @param path {const char*} Filename
	 * @return {bool} Whether successful
	 */
	bool open_append(const char* path);

protected:
	~aio_fstream();
	/**
	 * Dynamically release asynchronous stream class objects that can only be allocated on heap through this function
	 */
	virtual void destroy();

private:
};

}  // namespace acl

