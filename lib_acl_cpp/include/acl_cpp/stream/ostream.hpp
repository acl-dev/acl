#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/pipe_stream.hpp"
#include "stream.hpp"

struct iovec;

namespace acl {

class string;

/**
 * 输入流处理过程类，调用者想确切知道输出流是否出错或是否关闭，
 * 应该调用 stream->eof() 来进行判断
 */

class ACL_CPP_API ostream
	: virtual public stream
	, public pipe_stream
{
public:
	ostream(void) {}
	virtual ~ostream(void) {}

	/**
	 * 写数据至输出流中
	 * @param data {const void*} 数据指针地址
	 * @param size {size_t} data 数据长度(字节)
	 * @param loop {bool} 是否保证数据全部输出才返回，如果为 true,
	 *  则该函数直至数据全部输出或出错才会返回；否则仅写一次便返回，
	 *  但并不保证数据全部写完
	 * @param buffed {bool} 是否先缓存待写的数据
	 * @return {int} 真实写入的数据量, 返回 -1 表示出错
	 */
	int write(const void* data, size_t size, bool loop = true,
		bool buffed = false);

	/**
	 * 如果采用写缓冲方式，则最后需要调用本函数刷写缓冲区
	 * @return {bool} 返回 false 表示写失败，有可能是连接关闭
	 */
	bool fflush(void);

	/**
	 * 写数据至输出流中
	 * @param v {const struct iovec*}
	 * @param count {int} 数组 v 的元素个数
	 * @param loop {bool} 是否保证数据全部输出才返回，如果为 true，
	 *  则该函数直至数据全部输出或出错才会返回；否则仅写一次便返回，
	 *  但并不保证数据全部写完
	 * @return {int} 真实写入的数据量, 返回 -1 表示出错
	 */
	int writev(const struct iovec *v, int count, bool loop = true);

	/**
	 * 带格式方式写数据，类似于 vfprintf，保证数据全部写入
	 * @param fmt {const char*} 格式字符串
	 * @param ap {va_list} 变参列表
	 * @return {int} 真实写入的数据长度，返回 -1 表示出错
	 */
	int vformat(const char* fmt, va_list ap);

	/**
	 * 带格式方式写数据，类似于 fprintf，保证数据全部写入
	 * @param fmt {const char*} 变参格式字符串
	 * @return {int} 真实写入的数据长度，返回 -1 表示出错
	 */
	int format(const char* fmt, ...) ACL_CPP_PRINTF(2, 3);

	/**
	 * 写入一个 64 位整数
	 * @param n {acl_int64} 64 位数据
	 * @return {int} 写入的数据长度，返回 -1 表示出错
	 */
#if defined(_WIN32) || defined(_WIN64)
	int write(__int64 n);
#else
	int write(long long int n);
#endif

	/**
	 * 写入一个 32 位整数
	 * @param n {int} 32 位整数
	 * @return {int} 写入的数据长度，返回 -1 表示出错
	 */
	int write(int n);

	/**
	 * 写入一个 16 位短整数
	 * @param n {int} 16 位整数
	 * @return {int} 写入的数据长度，返回 -1 表示出错
	 */
	int write(short n);

	/**
	 * 写一个字节
	 * @param ch {char}
	 * @return {int} 写入的数据长度，返回 -1 表示出错
	 */
	int write(char ch);

	/**
	 * 输出缓冲区中的数据
	 * @param s {const string&}
	 * @param loop {bool} 是否要求全部输出完才返回
	 * @return {int} 输出数据的长度，返回 -1 表示出错
	 */
	int write(const string& s, bool loop = true);

	/**
	 * 输出一行字符串数据，在所给字符串后添加 "\r\n"
	 * @param s {const char*} 字符串指针，必须以 '\0' 结尾
	 * @return {int} 输出数据的长度，返回 -1 表示出错
	 */
	int puts(const char* s);

	/**
	 * 以下几个函数为输出操作符重载函数，且都是阻塞输出过程，
	 * 如果想判断输出流是否出错或关闭应该调用 stream->eof()
	 * 来进行判断
	 */

	ostream& operator<<(const string& s);
	ostream& operator<<(const char* s);
#if defined(_WIN32) || defined(_WIN64)
	ostream& operator<<(__int64 n);
#else
	ostream& operator<<(long long int n);
#endif
	ostream& operator<<(int n);
	ostream& operator<<(short n);
	ostream& operator<<(char ch);

	// pipe_stream 几个虚函数
	// 因为是输出流，所以仅实现一个
	virtual int push_pop(const char* in, size_t len,
		string* out = NULL, size_t max = 0);
	virtual int pop_end(string* out, size_t max = 0)
	{
		(void) out;
		(void) max;
		return (0);
	}

protected:
private:
};

} // namespace acl
