#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/stream/istream.hpp"
#include "acl_cpp/stream/ostream.hpp"

namespace acl {

class string;

class ACL_CPP_API fstream
	: public istream
	, public ostream
{
public:
	fstream(void);
	virtual ~fstream(void);

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
	bool open(const char* path, unsigned int oflags, int mode);

	/**
	 * 以读/写方式打开文件流，当文件不存在时则创建新文件，当文件存在时则
	 * 将文件清空, 文件属性为 0700
	 * @param path {const char*} 文件名
	 * @return {bool} 打开文件是否成功
	 */
	bool open_trunc(const char* path);

	/**
	 * 以读/写方式建新文件，文件属性为 0700, 若文件不存在则创建新文件，若存在则
	 * 打开旧文件
	 * @return {bool} 文件创建是否成功
	 */
	bool create(const char* path);

	/**
	 * 关闭文件流，同时关闭文件句柄
	 * @return {bool} 关闭是否成功
	 */
	bool close();

#ifdef WIN32
	void open(void* fh, unsigned int oflags);
	__int64 fseek(__int64 offset, int whence);
	bool ftruncate(__int64 length);
	__int64 fsize(void) const;
	void* file_handle() const;
#else
	void open(int fh, unsigned int oflags);
	long long int fseek(long long int offset, int whence);
	bool ftruncate(long long int length);
	long long int fsize(void) const;
	int file_handle() const;
#endif
	/**
	 * 获得文件的全路径
	 * @return {const char*} 若返回空则表示文件还未打开或出错
	 */
	const char* file_path() const;
};

} // namespace acl
