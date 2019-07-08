#pragma once
#include "../acl_cpp_define.hpp"
#include <stdlib.h>
#include "stream.hpp"

namespace acl {

class string;

/**
 * 输入流操作类，如果想确切知道输入流是否关闭或出错或读到了文件流的
 * 尾部，应通过调用 stream->eof() 来进行判断
 */

class ACL_CPP_API istream : virtual public stream
{
public:
	istream(void) {}
	virtual ~istream(void) {}

	/**
	 * 从输入流中读数据
	 * @param buf {void*} 用户缓冲区
	 * @param size {size_t} 用户缓冲区长度
	 * @param loop {bool} 是否读满 size 后才返回
	 * @return {int} 读操作结果, -1 表示关闭或出错, > 0 表示成功
	 */
	int read(void* buf, size_t size, bool loop = true);

	/**
	 * 从输入流读数据直至读到所要求的字符串或出错才返回
	 * @param buf {void*} 用户缓冲区
	 * @param inout {size_t*} 作为参数时 *inout 表示缓冲 buf
	 *  的空间大小，函数返回后记录存储于 buf 中的数据长度
	 * @param tag {const char*} 要求读到的字符串
	 * @param len {size_t} tag 字符串的长度
	 * @return {bool} 是否读到所要求的字符串数据
	 */

	bool readtags(void *buf, size_t* inout, const char *tag, size_t len);

	/**
	 * 从输入流中读到一行数据
	 * @param buf {void*} 用户缓冲区
	 * @param size_inout {size_t*} 作为参数时 *size_inout 表示缓冲 buf
	 *  的空间大小，函数返回后记录存储于 buf 中的数据长度
	 * @param nonl {bool} 如果为 true 则会将读到的一行数据尾部的 "\r\n"
	 *  或 "\n" 去掉，*size_inout 存储的数据长度是去掉 "\r\n" 或 "\n" 后
	 *  的长度；否则，保留数据行中的 "\r\n" 或 "\n"，同时 *size_inout 存
	 *  储的是包含 "\r\n" 或 "\n" 的数据长度
	 * @return {bool} 是否读到了一行数据, 出错则返回 false; 对文件输入流而
	 *  言，如果读到的数据是最后一部分数据且这部分数据不含 "\r\n" 或 "\n"
	 *  则也会返回 false, 调用者需要检查 *size_inout 值是否大于 0
	 *  来确定是否读到了最后一部分数据
	 */
	bool gets(void* buf, size_t* size_inout, bool nonl = true);

	/**
	 * 从输入流中读一个 64 位整数
	 * @param n {acl_int64&} 64 位整数
	 * @param loop {bool} 是否阻塞式读完8个字节
	 * @return {bool} 是否读取成功
	 */
#if defined(_WIN32) || defined(_WIN64)
	bool read(__int64& n, bool loop = true);
#else
	bool read(long long int& n, bool loop = true);
#endif

	/**
	 * 从输入流中读一个 32 位整数
	 * @param n {int&} 32 位整数
	 * @param loop {bool} 是否阻塞式读完4个字节
	 * @return {bool} 是否读取成功
	 */
	bool read(int& n, bool loop = true);

	/**
	 * 从输入流中读一个 16 位整数
	 * @param n {short&} 16 位整数
	 * @param loop {bool} 是否阻塞式读完2个字节
	 * @return {bool} 是否读取成功
	 */
	bool read(short& n, bool loop = true);

	/**
	 * 从输入流中读取一个字节
	 * @param ch {char&}
	 * @return {bool} 读取是否成功
	 */
	bool read(char& ch);

	/**
	 * 从输入流中读数据至缓冲区中
	 * @param s {string*} 缓冲区，内部会首先自动清空该缓冲区
	 * @param loop {bool} 是否阻塞式读满整个缓冲，缓冲区
	 *  的容量为 s.capacity()
	 * @return {bool} 读数据是否成功
	 */
	bool read(string& s, bool loop = true);
	bool read(string* s, bool loop = true);

	/**
	 * 从输入流中读数据至缓冲区中
	 * @param s {string*} 缓冲区，内部会首先自动清空该缓冲区
	 * @param max {size_t} 希望读到的数据的最大值
	 * @param loop {bool} 是否读到要求的 max 字节数为止
	 * @return {bool} 读数据是否成功
	 */
	bool read(string& s, size_t max, bool loop = true);
	bool read(string* s, size_t max, bool loop = true);

	/**
	 * 从输入流中读一行数据至缓冲区中
	 * @param s {string&} 缓冲区，内部会首先自动清空该缓冲区
	 * @param nonl {bool} 是否保留所读行数据中的 "\r\n" 或 "\n"
	 * @param max {size_t} 当该值 > 0 时，该值限定了所读到行的最大值，当
	 *  接收到数据行长度大于该值时，则仅返回部分数据，同时内部会记录警告；
	 *  当该值 = 0 时，则不限制行数据长度
	 * @return {bool} 是否读到了一行数据
	 *  1）如果返回 true 则说明读到了完整一行数据；如果该行数据中只有
	 *     "\r\n" 或 "\n"，则 s 的内容为空，即：s.empty() == true
	 *  2）如果返回 false 则说明读关闭且未读到一行数据，此时 s 中有可能
	 *     存储着部分数据，需要用 if (s.empty() == true) 判断一下
	 */
	bool gets(string& s, bool nonl = true, size_t max = 0);
	bool gets(string* s, bool nonl = true, size_t max = 0);

	/**
	 * 从输入流中读数据直到读到要求的字符串数据作为分隔符的数据，
	 * 读取的数据的最后部分应该是该字符串
	 * @param s {string&} 缓冲区，内部会首先自动清空该缓冲区
	 * @param tag {const string&} 要求读的字符串数据
	 * @return {bool} 是否读到要求的字符串数据
	 */
	bool readtags(string& s, const string& tag);
	bool readtags(string* s, const string& tag);

	/**
	 * 从输入流中读一个字节数据
	 * @return {int} 所读字节的 ASCII 码值，如果返回值为 -1 则表示对方关闭或
	 *  读出错
	 */
	int getch(void);

	/**
	 * 向输入流中放加一个字节的数据
	 * @param ch {int} 一个字符的 ASCII 码值
	 * @return {int} 如果返回值与 ch 值相同则表示正确，否则表示出错
	 */
	int ugetch(int ch);

	/**
	 * 尝试性从输入流中读取一行数据
	 * @param buf {string&} 缓冲区
	 * @param nonl {bool} 是否保留所读行数据中的 "\r\n" 或 "\n"
	 * @param clear {bool} 是否内部自动清空 buf 缓冲区
	 * @param max {int} 当该值 > 0 时则设置所读行数据的最大长度以避免本地
	 *  缓冲区溢出
	 * @return {bool} 是否读了一行数据; 如果返回 false 并不表示输入
	 *  流结束，只是表示未读到一个完整行数据，应该通过调用 stream->eof()
	 *  来检查输入流是否关闭，另外，如果仅读到了部分数据，则 buf 会存储
	 *  这些部分数据
	 *  注意：为了防止 buf 缓冲区溢出，调用者调用该方法获得的数据即使不够
	 *  一行数据，应尽量取出 buf 中的数据然后将 buf->clear()，以防止 buf
	 *  内存过大导致缓冲区溢出
	 */
	bool gets_peek(string& buf, bool nonl = true,
		bool clear = false, int max = 0);
	bool gets_peek(string* buf, bool nonl = true,
		bool clear = false, int max = 0);

	/**
	 * 尝试性从输入流中读取数据
	 * @param buf {string&} 缓冲区
	 * @param clear {bool} 函数开始时是否内部自动清空 buf 缓冲区
	 * @return {bool} 是否读到数据, 如果返回 false 仅 表示未读完所要求
	 *  的数据长度，应该通过调用 stream->eof() 来检查输入流是否关闭
	 *  注意：为了防止 buf 缓冲区溢出，调用者调用该方法获得的数据即使不够
	 *  一行数据，应尽量取出 buf 中的数据然后将 buf->clear()，以防止 buf
	 *  内存过大导致缓冲区溢出
	 */
	bool read_peek(string& buf, bool clear = false);
	bool read_peek(string* buf, bool clear = false);

	/**
	 * 尝试性从输入流中读取数据
	 * @param buf {void*} 缓冲区
	 * @param size {size_t} buf 缓冲区大小
	 * @return {int} 返回 -1 表示读出错或关闭，> 0 表示读到的数据长度，
	 *  如果返回 0 表示本次没有读到数据，可以继续读，当返回值 < 0 时，
	 *  可通过 eof() 判断流是否应该关闭
	 */
	int read_peek(void* buf, size_t size);

	/**
	 * 尝试性从输入流中读取指定长度的数据
	 * @param buf {string&} 缓冲区
	 * @param cnt {size_t} 要求读到的数据长度
	 * @param clear {bool} 函数开始时是否内部自动清空 buf 缓冲区
	 * @return {bool} 是否读到所要求数据长度的数据, 如果返回 false 仅
	 *  表示未读完所要求的数据长度，应该通过调用 stream->eof() 来检查
	 *  输入流是否关闭
	 *  注意：为了防止 buf 缓冲区溢出，调用者调用该方法获得的数据即使不够
	 *  一行数据，应尽量取出 buf 中的数据然后将 buf->clear()，以防止 buf
	 *  内存过大导致缓冲区溢出
	 */
	bool readn_peek(string& buf, size_t cnt, bool clear = false);
	bool readn_peek(string* buf, size_t cnt, bool clear = false);

	/* 以下几个函数重载了输入操作符，它们都是阻塞式操作过程，且需要
	 * 调用 stream->eof() 来判断输入流是否关闭或是否读到了文件尾
	 */

	istream& operator>>(string& s);
#if defined(_WIN32) || defined(_WIN64)
	istream& operator>>(__int64& n);
#else
	istream& operator>>(long long int& n);
#endif
	istream& operator>>(int& n);
	istream& operator>>(short& n);
	istream& operator>>(char& ch);
};

} // namespace acl
