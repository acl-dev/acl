#pragma once
#include "../acl_cpp_define.hpp"
#include "istream.hpp"
#include "ostream.hpp"

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
	 * @param oflags {unsigned int} 标志位, We're assuming that
	 *  O_RDONLY: 0x0000, O_WRONLY: 0x0001, O_RDWR: 0x0002,
	 *  O_APPEND: 0x0008, O_CREAT: 0x0100, O_TRUNC: 0x0200,
	 *  O_EXCL: 0x0400; just for win32, O_TEXT: 0x4000,
	 *  O_BINARY: 0x8000, O_RAW: O_BINARY, O_SEQUENTIAL: 0x0020,
	 *  O_RANDOM: 0x0010.
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
	 * 以读/写方式建新文件，文件属性为 0700, 若文件不存在则创建新文件，
	 * 若存在则打开旧文件
	 * @return {bool} 文件创建是否成功
	 */
	bool create(const char* path);

	/**
	 * 将本类对象对应的文件从磁盘上删除，该函数只有当内部知道文件路径
	 * 时才能正确删除文件，否则无法删除
	 * @return {bool} 删除文件是否成功
	 */
	bool remove(void);

	/**
	 * 将当前文件重命名为指定文件名，针对 WINDOWS 平台，需要先关闭当前文件
	 * 句柄，当重命名成功后再重新打开新的目标文件
	 * @param from_path {const char*} 源文件名
	 * @param to_path {const char*} 目标文件名
	 * @return {bool} 命名是否成功
	 */
	bool rename(const char* from_path, const char* to_path);

#if defined(_WIN32) || defined(_WIN64)
	/**
	 * 根据系统的文件句柄打开 fstream 文件流对象
	 * @param fh 系统文件句柄
	 * @param oflags 打开标志位
	 * @param path {const char*} 非 NULL 时当被作为该文件句柄的文件路径
	 *  来存储，以便于 file_path, remove 使用
	 */
	void open(void* fh, unsigned int oflags, const char* path = NULL);

	/**
	 * 移动文件指针位置
	 * @param offset {__int64} 偏移量
	 * @param whence {int} 移动方向：SEEK_SET（从文件起始位置后移动）,
	 *  SEEK_CUR(从当前文件指针位置向后移动), SEEK_END(从文件尾向前移动)
	 * @return {acl_off_t} 正常时返回值 >= 0，出错时返回 -1
	 */
	__int64 fseek(__int64 offset, int whence);

	/**
	 * 获得当前系统文件指针在文件中的偏移位置
	 * @return {acl_off_t} 正常时返回值 >= 0，出错时返回 -1
	 */
	__int64 ftell(void);

	/**
	 * 将文件尺寸截断至指定大小
	 * @param length {acl_off_t} 文件截断后的大小尺寸
	 * @return {bool} 是否成功
	 */
	bool ftruncate(__int64 length);

	/**
	 * 获得当前文件的大小
	 * @return {acl_off_t} 正常时返回值 >= 0，出错返回 -1
	 */
	__int64 fsize(void) const;

	/**
	 * 静态方法用于获取指定文件的文件大小
	 * @param path {const char*} 非空字符串指定文件路径
	 * @return {__int64} 正常时返回值 >= 0，出错时返回 -1
	 */
	static __int64 fsize(const char* path);

	/**
	 * 返回系统文件句柄
	 * @return 系统文件句柄，出错时返回 ACL_FILE_INVALID
	 */
	void* file_handle(void) const;
#else
	void open(int fh, unsigned int oflags, const char* path = NULL);
	long long int fseek(long long int offset, int whence);
	long long int ftell(void);
	bool ftruncate(long long int length);
	long long int fsize(void) const;
	static long long int fsize(const char* path);
	int file_handle(void) const;
#endif
	/**
	 * 获得文件的全路径
	 * @return {const char*} 若返回空则表示文件还未打开或出错
	 */
	const char* file_path(void) const;

	/**
	 * 当文件打开后，该方法用来对文件加锁
	 * @param exclude {bool} 加锁方式是否是独占锁，默认是独占锁，如果该值
	 * 为 false, 则为共享锁
	 * @return {bool} 返回 false 表示加锁失败，可通过 acl::last_serror 查看
	 *  出错原因
	 */
	bool lock(bool exclude = true);

	/**
	 * 当文件打开后，该方法用来尝试对文件加锁，如果加锁成功则返回 true，如
	 * 果该文件已经被其它进程加锁，则返回 false
	 * @param exclude {bool} 加锁方式是否是独占锁，默认是独占锁，如果该值
	 * 为 false, 则为共享锁
	 * @return {bool} 返回 false 表示加锁失败，可通过 acl::last_serror 查看
	 *  出错原因
	 */
	bool try_lock(bool exclude = true);

	/**
	 * 当调用 lock 或 try_lock 成功后可以调用本方法对文件解锁
	 * @return {bool} 解锁是否成功
	 */
	bool unlock(void);
};

} // namespace acl
