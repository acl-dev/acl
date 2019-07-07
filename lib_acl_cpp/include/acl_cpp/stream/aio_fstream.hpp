#pragma once
#include "../acl_cpp_define.hpp"
#include "aio_istream.hpp"
#include "aio_ostream.hpp"

namespace acl {

class fstream;

/**
 * 异步文件读写流，该类对象只可用在 UNIX 系统中
 */
class ACL_CPP_API aio_fstream
	: public aio_istream
	, public aio_ostream
{
public:
	/**
	 * 构造函数
	 * @param handle {aio_handle*} 异步事件句柄
	 */
	aio_fstream(aio_handle* handle);

#if defined(_WIN32) || defined(_WIN64)
	aio_fstream(aio_handle* handle, HANDLE fd, unsigned int oflags = 0600);
#else
	aio_fstream(aio_handle* handle, int fd, unsigned int oflags = 0600);
#endif

	/**
	 * 根据文件路径打开文件流, 这是最基础的打开文件的方式
	 * @param path {const char*} 文件名
	 * @param oflags {unsigned int} 标志位, We're assuming that O_RDONLY: 0x0000,
	 *  O_WRONLY: 0x0001, O_RDWR: 0x0002, O_APPEND: 0x0008, O_CREAT: 0x0100,
	 *  O_TRUNC: 0x0200, O_EXCL: 0x0400; just for win32, O_TEXT: 0x4000,
	 *  O_BINARY: 0x8000, O_RAW: O_BINARY, O_SEQUENTIAL: 0x0020, O_RANDOM: 0x0010.
	 * @param mode {int} 打开文件句柄时的模式(如: 0600)
	 * @return {bool} 打开文件是否成功
	 */
	bool open(const char* path, unsigned int oflags, unsigned int mode);

	/**
	 * 以读/写方式打开文件流，当文件不存在时则创建新文件，当文件存在时则
	 * 将文件清空, 文件属性为 0700
	 * @param path {const char*} 文件名
	 * @param mode {int} 打开文件句柄时的模式(如: 0600)
	 * @return {bool} 打开文件是否成功
	 */
	bool open_trunc(const char* path, unsigned int mode = 0600);

	/**
	 * 以读/写方式建新文件，文件属性为 0600, 若文件不存在则创建新文件，若存在则
	 * 打开旧文件
	 * @param path {const char*} 文件全路径
	 * @param mode {int} 打开文件句柄时的模式(如: 0600)
	 * @return {bool} 文件创建是否成功
	 */
	bool create(const char* path, unsigned int mode = 0600);

	/**
	 * 以只读方式打开已经存在的文件
	 * @param path {const char*} 文件名
	 * @return {bool} 打开文件是否成功
	 */
	bool open_read(const char* path);

	/**
	 * 以只写方式打开文件，如果文件不存在则创建新文件，如果文件
	 * 存在，则将文件内容清空
	 * @param path {const char*} 文件名
	 * @return {bool} 是否成功
	 */
	bool open_write(const char* path);

	/**
	 * 以尾部添加方式打开文件，如果文件不存在则创建新文件
	 * @param path {const char*} 文件名
	 * @return {bool} 是否成功
	 */
	bool open_append(const char* path);

protected:
	~aio_fstream(void);
	/**
	 * 通过此函数来动态释放只能在堆上分配的异步流类对象
	 */
	virtual void destroy(void);

private:
};

}  // namespace acl
