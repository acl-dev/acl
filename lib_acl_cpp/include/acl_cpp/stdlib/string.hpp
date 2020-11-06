#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include <list>
#include <string>
#include <stdarg.h>
#include <utility>

struct ACL_VSTRING;
struct ACL_LINE_STATE;

namespace acl {

class dbuf_pool;

/**
 * 该类为字符串处理类，支持大部分 std::string 中的功能，同时支持其不支持的一些
 * 功能；该类内部自动保证最后一个字符为 \0
 */
class ACL_CPP_API string
{
public:
	/**
	 * 构造函数
	 * @param n {size_t} 初始时分配的内存大小
	 * @param bin {bool} 是否以二进制方式构建缓冲区对象，该值为 true 时，
	 *  则当调用 += int|int64|short|char 或调用 << int|int64|short|char
	 *  时，则按二进制方式处理，否则按文本方式处理
	 */
	string(size_t n, bool bin);
	explicit string(size_t n);
	string(void);

	/**
	 * 构造函数
	 * @param s {const string&} 源字符串对象，初始化后的类对象内部自动复制
	 *  该字符串
	 */
	string(const string& s);

	/**
	 * 构造函数
	 * @param s {const char*} 内部自动用该字符串初始化类对象，s 必须是
	 *  以 \0 结尾
	 */
	string(const char* s);

	/**
	 * 构造函数
	 * @param s {const char*} 源缓冲内容
	 * @param n {size_t} s 缓冲区数据长度
	 */
	string(const void* s, size_t n);

#if defined(_WIN32) || defined(_WIN64)
	/**
	 * 采用内存映射文件方式构造对象
	 * @param fd {int} 文件句柄
	 * @param max {size_t} 所映射的最大空间大小
	 * @param n {size_t} 初始化大小
	 */
	string(void* fd, size_t max, size_t n);
#else
	string(int fd, size_t max, size_t n);
#endif

	virtual ~string(void);

	/**
	 * 设置字符串类对象为二进制处理模式
	 * @param bin {bool} 当该值为 true 时，则设置字符串类对象为二进制处理
	 *  方式；否则为文本方式；为 true 时，则当调用 += int|int64|short|char
	 *  或调用 << int|int64|short|char 时，则按二进制方式处理，否则按文本
	 *  方式处理
	 * @return {string&}
	 */
	string& set_bin(bool bin);

	/**
	 * 设置缓冲区的最大长度，以避免缓冲区溢出
	 * @param max {int}
	 * @return {string&}
	 */
	string& set_max(int  max);

	/**
	 * 判断当前字符串类对象是否为二进制处理方式 
	 * @return {bool} 返回值为 true 时则表示为二进制方式
	 */
	bool get_bin() const;

	/**
	 * 返回当前缓冲区的最大长度限制，若返回值 <= 0 则表示没有限制
	 * @return {int}
	 */
	int get_max(void) const;

	/**
	 * 根据字符数组下标获得指定位置的字符，输入参数必须为合法值，否则则
	 * 内部产生断言
	 * @param n {size_t} 指定的位置（该值 >= 0 且 < 字符串长度)，如果越界
	 *  则产生断言
	 * @return {char} 返回指定位置的字符
	 */
	char operator[](size_t n) const;

	/**
	 * 根据字符数组下标获得指定位置的字符，输入参数必须为合法值，否则则
	 * 内部产生断言
	 * @param n {int} 指定的位置（该值 >= 0 且 < 字符串长度)，如果越界，
	 *  则产生断言
	 * @return {char} 返回指定位置的字符
	 */
	char operator[](int n) const;

	/**
	 * 左值赋值重载，用户可以直接使用对象的数组下标进行赋值，如果下标组值
	 * 越界，则内部会自动扩充缓冲区空间
	 * @param n {size_t} 数组下标位置
	 * @return {char&}
	 */
	char& operator[](size_t n);

	/**
	 * 左值赋值重载，用户可以直接使用对象的数组下标进行赋值，如果下标组值
	 * 越界，则内部会自动扩充缓冲区空间
	 * @param n {int} 数组下标位置，该值必须 >= 0
	 * @return {char&}
	 */
	char& operator[](int n);

	/**
	 * 对目标字符串类对象赋值
	 * @param s {const char*} 源字符串
	 * @return {string&} 返回当前对象的引用，便于对该类对象连续进行操作
	 */
	string& operator=(const char* s);

	/**
	 * 对目标字符串类对象赋值
	 * @param s {const string&} 源字符串对象
	 * @return {string&} 返回当前对象的引用，便于对该类对象连续进行操作
	 */
	string& operator=(const string& s);

	/**
	 * 对目标字符串类对象赋值
	 * @param s {const string*} 源字符串对象
	 * @return {string&} 返回当前对象的引用，便于对该类对象连续进行操作
	 */
	string& operator=(const string* s);

	/**
	 * 对目标字符串类对象赋值
	 * @param s {const std::string&} 源字符串对象
	 * @return {string&} 返回当前对象的引用，便于对该类对象连续进行操作
	 */
	string& operator=(const std::string& s);

	/**
	 * 对目标字符串类对象赋值
	 * @param s {const std::string*} 源字符串对象
	 * @return {string&} 返回当前对象的引用，便于对该类对象连续进行操作
	 */
	string& operator=(const std::string* s);

#if defined(_WIN32) || defined(_WIN64)
	/**
	 * 对目标字符串类对象赋值
	 * @param n {long long int} 源 64 位符号长整数，若当前对象的当前状态为
	 *  二进制模式，则该函数便会以二进制方式赋值给字符串对象，否则以文本方
	 *  式赋值给字符串对象；关于二进制模式还是文本方式，其含义参见
	 *  set_bin(bool)
	 * @return {string&} 返回当前字对象的引用，便于对该类对象连续进行操作
	 */
	string& operator=(__int64 n);

	/**
	 * 对目标字符串类对象赋值
	 * @param n {unsinged long long int} 源 64 位无符号长整数，若字符串对象
	 *  的当前状态为二进制模式，则该函数便会以二进制方式赋值给字符串对象，
	 *  否则以文本方式赋值给字符串对象；关于二进制模式还是文本方式，其含义
	 *  参见 set_bin(bool)
	 * @return {string&} 返回当前对象的引用，便于对该类对象连续进行操作
	 */
	string& operator=(unsigned __int64);
#else
	string& operator=(long long int);
	string& operator=(unsigned long long int);
#endif

	/**
	 * 对目标字符串类对象赋值
	 * @param n {char} 源有符号字符；若字符串对象的当前状态为二进制模式，
	 *  则该函数便会以二进制方式赋值给字符串对象，否则以文本方式赋值给字
	 *  符串对象；关于二进制模式还是文本方式，其含义参见 set_bin(bool)
	 * @return {string&} 返回当前对象的引用，便于对该类对象连续进行操作
	 */
	string& operator=(char n);

	/**
	 * 对目标字符串类对象赋值
	 * @param n {char} 源无符号字符；若对象的当前状态为二进制模式，则该函数
	 *  便会以二进制方式赋值给字符串对象，否则以文本方式赋值给字符串对象；
	 *  关于二进制模式还是文本方式，其含义参见 set_bin(bool)
	 * @return {string&} 返回当前对象的引用，便于对该类对象连续进行操作
	 */
	string& operator=(unsigned char n);

	/**
	 * 对目标字符串类对象赋值
	 * @param n {char} 源有符号长整型；若对象当前状态为二进制模式，则该函数
	 *  便会以二进制方式赋值给字符串对象，否则以文本方式赋值给字符串对象；
	 *  关于二进制模式还是文本方式，其含义参见 set_bin(bool)
	 * @return {string&} 返回当前对象的引用，便于对该类对象连续进行操作
	 */
	string& operator=(long n);

	/**
	 * 对目标字符串类对象赋值
	 * @param n {char} 源无符号长整型；若字对象的当前状态为二进制模式，
	 *  则该函数便会以二进制方式赋值给字符串对象，否则以文本方式赋值给
	 *  字符串对象；关于二进制模式还是文本方式，其含义参见 set_bin(bool)
	 * @return {string&} 返回当前对象的引用，便于对该类对象连续进行操作
	 */
	string& operator=(unsigned long n);

	/**
	 * 对目标字符串类对象赋值
	 * @param n {char} 源有符号整型；若字符串对象的当前状态为二进制模式，
	 *  则该函数便会以二进制方式赋值给字符串对象，否则以文本方式赋值给字符
	 *  串对象；关于二进制模式还是文本方式，其含义参见 set_bin(bool)
	 * @return {string&} 返回当前对象的引用，便于对该类对象连续进行操作
	 */
	string& operator=(int n);

	/**
	 * 对目标字符串类对象赋值
	 * @param n {char} 源无符号整型；若字符串对象的当前状态为二进制模式，
	 *  则该函数便会以二进制方式赋值给字符串对象，否则以文本方式赋值给字符
	 *  串对象；关于二进制模式还是文本方式，其含义参见 set_bin(bool)
	 * @return {string&} 返回当前对象的引用，便于对该类对象连续进行操作
	 */
	string& operator=(unsigned int n);

	/**
	 * 对目标字符串类对象赋值
	 * @param n {char} 源有符号短整型；若字符串对象的当前状态为二进制模式,
	 *  则该函数便会以二进制方式赋值给字符串对象，否则以文本方式赋值给字符
	 *  串对象；关于二进制模式还是文本方式，其含义参见 set_bin(bool)
	 * @return {string&} 返回当前对象的引用，便于对该类对象连续进行操作
	 */
	string& operator=(short n);

	/**
	 * 对目标字符串类对象赋值
	 * @param n {char} 源无符号短整型；若对象的当前状态为二进制模式，则该
	 *  函数便会以二进制方式赋值给字符串对象，否则以文本方式赋值给字符串
	 *  对象；关于二进制模式还是文本方式，其含义参见 set_bin(bool)
	 * @return {string&} 返回当前对象的引用，便于对该类对象连续进行操作
	 */
	string& operator=(unsigned short n);

	/**
	 * 向目标字符串对象尾部添加字符串
	 * @param s {const char*} 源字符串指针
	 * @return {string&} 目标字符串对象的引用
	 */
	string& operator+=(const char* s);

	/**
	 * 向目标字符串对象尾部添加字符串
	 * @param s {const string&} 源字符串对象引用
	 * @return {string&} 目标字符串对象的引用
	 */
	string& operator+=(const string& s);

	/**
	 * 向目标字符串对象尾部添加字符串
	 * @param s {const string*} 源字符串对象指针
	 * @return {string&} 目标字符串对象的引用
	 */
	string& operator+=(const string* s);

	/**
	 * 向目标字符串对象尾部添加字符串
	 * @param s {const std::string&} 源字符串对象引用
	 * @return {string&} 目标字符串对象的引用
	 */
	string& operator+=(const std::string& s);

	/**
	 * 向目标字符串对象尾部添加字符串
	 * @param s {const std::string*} 源字符串对象指针
	 * @return {string&} 目标字符串对象的引用
	 */
	string& operator+=(const std::string* s);

#if defined(_WIN32) || defined(_WIN64)
	/**
	 * 向目标字符串对象尾部添加有符号长整型数字，当为目标字符串对象为
	 * 二进制方式时，则按二进制数字方式添加；否则按文本方式添加
	 * @param n {long long int} 源 64 位有符号整数
	 * @return {string&} 目标字符串对象的引用
	 */
	string& operator+=(__int64 n);

	/**
	 * 向目标字符串对象尾部添加无符号长整型数字，当为目标字符串对象为
	 * 二进制方式时，则按二进制数字方式添加；否则按文本方式添加
	 * @param n {long long int} 源 64 位无符号整数
	 * @return {string&} 目标字符串对象的引用
	 */
	string& operator+=(unsigned __int64 n);
#else
	string& operator+=(long long int n);
	string& operator+=(unsigned long long int n);
#endif

	/**
	 * 向目标字符串对象尾部添加有符号长整型数字，当为目标字符串对象为
	 * 二进制方式时，则按二进制数字方式添加；否则按文本方式添加
	 * @param n {long} 源有符号长整数
	 * @return {string&} 目标字符串对象的引用
	 */
	string& operator+=(long n);

	/**
	 * 向目标字符串对象尾部添加无符号长整型数字，当为目标字符串对象为
	 * 二进制方式时，则按二进制数字方式添加；否则按文本方式添加
	 * @param n {long} 源无符号长整数
	 * @return {string&} 目标字符串对象的引用
	 */
	string& operator+=(unsigned long n);

	/**
	 * 向目标字符串对象尾部添加有符号整型数字，当为目标字符串对象为
	 * 二进制方式时，则按二进制数字方式添加；否则按文本方式添加
	 * @param n {long} 源有符号整数
	 * @return {string&} 目标字符串对象的引用
	 */
	string& operator+=(int n);

	/**
	 * 向目标字符串对象尾部添加无符号整型数字，当为目标字符串对象为
	 * 二进制方式时，则按二进制数字方式添加；否则按文本方式添加
	 * @param n {long} 源无符号整数
	 * @return {string&} 目标字符串对象的引用
	 */
	string& operator+=(unsigned int n);

	/**
	 * 向目标字符串对象尾部添加有符号短整型数字，当为目标字符串对象为
	 * 二进制方式时，则按二进制数字方式添加；否则按文本方式添加
	 * @param n {long} 源有符号短整数
	 * @return {string&} 目标字符串对象的引用
	 */
	string& operator+=(short n);

	/**
	 * 向目标字符串对象尾部添加无符号短整型数字，当为目标字符串对象为
	 * 二进制方式时，则按二进制数字方式添加；否则按文本方式添加
	 * @param n {long} 源无符号短整数
	 * @return {string&} 目标字符串对象的引用
	 */
	string& operator+=(unsigned short n);

	/**
	 * 向目标字符串对象尾部添加有符号字符，当为目标字符串对象为
	 * 二进制方式时，则按二进制数字方式添加；否则按文本方式添加
	 * @param n {long} 源有符号字符
	 * @return {string&} 目标字符串对象的引用
	 */
	string& operator+=(char n);

	/**
	 * 向目标字符串对象尾部添加无符号字符，当为目标字符串对象为
	 * 二进制方式时，则按二进制数字方式添加；否则按文本方式添加
	 * @param n {long} 源无符号字符
	 * @return {string&} 目标字符串对象的引用
	 */
	string& operator+=(unsigned char n);

	/**
	 * 向目标字符串对象尾部添加字符串
	 * @param s {const string&} 源字符串对象引用
	 * @return {string&} 目标字符串对象的引用
	 */
	string& operator<<(const string& s);

	/**
	 * 向目标字符串对象尾部添加字符串
	 * @param s {const string*} 源字符串对象指针
	 * @return {string&} 目标字符串对象的引用
	 */
	string& operator<<(const string* s);

	/**
	 * 向目标字符串对象尾部添加字符串
	 * @param s {const std::string&} 源字符串对象引用
	 * @return {string&} 目标字符串对象的引用
	 */
	string& operator<<(const std::string& s);

	/**
	 * 向目标字符串对象尾部添加字符串
	 * @param s {const std::string*} 源字符串对象指针
	 * @return {string&} 目标字符串对象的引用
	 */
	string& operator<<(const std::string* s);

	/**
	 * 向目标字符串对象尾部添加字符串
	 * @param s {const char*} 源字符串指针
	 * @return {string&} 目标字符串对象的引用
	 */
	string& operator<<(const char* s);
#if defined(_WIN32) || defined(_WIN64)
	/**
	 * 向目标字符串对象尾部添加有符号长整型数字，当为目标字符串对象为
	 * 二进制方式时，则按二进制数字方式添加；否则按文本方式添加
	 * @param n {long long int} 源 64 位有符号整数
	 * @return {string&} 目标字符串对象的引用
	 */
	string& operator<<(__int64 n);

	/**
	 * 向目标字符串对象尾部添加无符号长整型数字，当为目标字符串对象为
	 * 二进制方式时，则按二进制数字方式添加；否则按文本方式添加
	 * @param n {long long int} 源 64 位无符号整数
	 * @return {string&} 目标字符串对象的引用
	 */
	string& operator<<(unsigned __int64 n);
#else
	string& operator<<(long long int n);
	string& operator<<(unsigned long long int n);
#endif

	/**
	 * 向目标字符串对象尾部添加有符号长整型数字，当为目标字符串对象为
	 * 二进制方式时，则按二进制数字方式添加；否则按文本方式添加
	 * @param n {long} 源有符号长整数
	 * @return {string&} 目标字符串对象的引用
	 */
	string& operator<<(long n);

	/**
	 * 向目标字符串对象尾部添加无符号长整型数字，当为目标字符串对象为
	 * 二进制方式时，则按二进制数字方式添加；否则按文本方式添加
	 * @param n {long} 源无符号长整数
	 * @return {string&} 目标字符串对象的引用
	 */
	string& operator<<(unsigned long n);

	/**
	 * 向目标字符串对象尾部添加有符号整型数字，当为目标字符串对象为
	 * 二进制方式时，则按二进制数字方式添加；否则按文本方式添加
	 * @param n {long} 源有符号整数
	 * @return {string&} 目标字符串对象的引用
	 */
	string& operator<<(int n);

	/**
	 * 向目标字符串对象尾部添加无符号整型数字，当为目标字符串对象为
	 * 二进制方式时，则按二进制数字方式添加；否则按文本方式添加
	 * @param n {long} 源无符号整数
	 * @return {string&} 目标字符串对象的引用
	 */
	string& operator<<(unsigned int n);

	/**
	 * 向目标字符串对象尾部添加有符号短整型数字，当为目标字符串对象为
	 * 二进制方式时，则按二进制数字方式添加；否则按文本方式添加
	 * @param n {long} 源有符号短整数
	 * @return {string&} 目标字符串对象的引用
	 */
	string& operator<<(short n);

	/**
	 * 向目标字符串对象尾部添加无符号短整型数字，当为目标字符串对象为
	 * 二进制方式时，则按二进制数字方式添加；否则按文本方式添加
	 * @param n {long} 源无符号短整数
	 * @return {string&} 目标字符串对象的引用
	 */
	string& operator<<(unsigned short n);

	/**
	 * 向目标字符串对象尾部添加有符号字符，当为目标字符串对象为
	 * 二进制方式时，则按二进制数字方式添加；否则按文本方式添加
	 * @param n {long} 源有符号字符
	 * @return {string&} 目标字符串对象的引用
	 */
	string& operator<<(char n);

	/**
	 * 向目标字符串对象尾部添加无符号字符，当为目标字符串对象为
	 * 二进制方式时，则按二进制数字方式添加；否则按文本方式添加
	 * @param n {long} 源无符号字符
	 * @return {string&} 目标字符串对象的引用
	 */
	string& operator<<(unsigned char n);

	/**
	 * 将字符串对象中的内容赋予目标字符串对象
	 * @param s {string*} 目标字符串对象
	 * @return {size_t} 返回拷贝的实际字节数，empty() == true 时，则返回 0
	 */
	size_t operator>>(string* s);

	/**
	 * 将字符串对象中的内容赋予目标字符串对象
	 * @param s {string&} 目标字符串对象
	 * @return {size_t} 返回拷贝的实际字节数，empty() == true 时，则返回 0
	 */
	size_t operator>>(string& s);

	/**
	 * 将字符串对象中的内容赋予目标字符串对象
	 * @param s {std::string*} 目标字符串对象
	 * @return {size_t} 返回拷贝的实际字节数，empty() == true 时，则返回 0
	 */
	size_t operator>>(std::string* s);

	/**
	 * 将字符串对象中的内容赋予目标字符串对象
	 * @param s {std::string&} 目标字符串对象
	 * @return {size_t} 返回拷贝的实际字节数，empty() == true 时，则返回 0
	 */
	size_t operator>>(std::string& s);

#if defined(_WIN32) || defined(_WIN64)
	/**
	 * 将字符串对象中的内容赋予目标 64 位有符号整数
	 * @param n {string*} 目标 64 位有符号整数
	 * @return {size_t} 返回拷贝的实际字节数，empty() == true 时，则返回 0
	 */
	size_t operator>>(__int64& n);

	/**
	 * 将字符串对象中的内容赋予目标 64 位无符号整数
	 * @param n {string*} 目标 64 位无符号整数
	 * @return {size_t} 返回拷贝的实际字节数，empty() == true 时，则返回 0
	 */
	size_t operator>>(unsigned __int64& n);
#else
	size_t operator>>(long long int&);
	size_t operator>>(unsigned long long int&);
#endif

	/**
	 * 将字符串对象中的内容赋予目标 32 位有符号整数
	 * @param n {string*} 目标 32 位有符号整数
	 * @return {size_t} 返回拷贝的实际字节数，empty() == true 时，则返回 0
	 */
	size_t operator>>(int& n);

	/**
	 * 将字符串对象中的内容赋予目标 32 位无符号整数
	 * @param n {string*} 目标 32 位无符号整数
	 * @return {size_t} 返回拷贝的实际字节数，empty() == true 时，则返回 0
	 */
	size_t operator>>(unsigned int& n);

	/**
	 * 将字符串对象中的内容赋予目标 16 位有符号整数
	 * @param n {string*} 目标 16 位有符号整数
	 * @return {size_t} 返回拷贝的实际字节数，empty() == true 时，则返回 0
	 */
	size_t operator>>(short& n);

	/**
	 * 将字符串对象中的内容赋予目标 16 位无符号整数
	 * @param n {string*} 目标 16 位无符号整数
	 * @return {size_t} 返回拷贝的实际字节数，empty() == true 时，则返回 0
	 */
	size_t operator>>(unsigned short& n);

	/**
	 * 将字符串对象中的内容赋予目标 8 位有符号字符
	 * @param n {string*} 目标 16 位有符号字符
	 * @return {size_t} 返回拷贝的实际字节数，empty() == true 时，则返回 0
	 */
	size_t operator>>(char& n);

	/**
	 * 将字符串对象中的内容赋予目标 8 位无符号字符
	 * @param n {string*} 目标 16 位无符号字符
	 * @return {size_t} 返回拷贝的实际字节数，empty() == true 时，则返回 0
	 */
	size_t operator>>(unsigned char& n);

	/**
	 * 判断当前对象的内容与所给的字符串对象内容是否相等（内部区分大小写）
	 * @param s {const string&} 输入的字符串对象引用
	 * @return {bool} 返回 true 表示字符串内容相同
	 */
	bool operator==(const string& s) const;

	/**
	 * 判断当前对象的内容与所给的字符串对象内容是否相等（内部区分大小写）
	 * @param s {const string&} 输入的字符串对象指针
	 * @return {bool} 返回 true 表示字符串内容相同
	 */
	bool operator==(const string* s) const;

	/**
	 * 判断当前对象的内容与所给的字符串内容是否相等（内部区分大小写）
	 * @param s {const string&} 输入的字符串对象指针
	 * @return {bool} 返回 true 表示字符串内容相同
	 */
	bool operator==(const char* s) const;

	/**
	 * 判断当前对象的内容与所给的字符串对象内容是否不等（内部区分大小写）
	 * @param s {const string&} 输入的字符串对象引用
	 * @return {bool} 返回 true 表示字符串内容不同
	 */
	bool operator!=(const string& s) const;

	/**
	 * 判断当前对象的内容与所给的字符串对象内容是否不等（内部区分大小写）
	 * @param s {const string&} 输入的字符串对象指针
	 * @return {bool} 返回 true 表示字符串内容不同
	 */
	bool operator!=(const string* s) const;

	/**
	 * 判断当前对象的内容与所给的字符串内容是否不等（内部区分大小写）
	 * @param s {const string&} 输入的字符串对象指针
	 * @return {bool} 返回 true 表示字符串内容不同
	 */
	bool operator!=(const char* s) const;

	/**
	 * 判断当前对象的内容是否小于所给的字符串对象内容（内部区分大小写）
	 * @param s {const string&} 输入的字符串对象引用
	 * @return {bool} 返回 true 表示当前字符串对象的内容小于输入的字符串
	 *  对象内容
	 */
	bool operator<(const string& s) const;

	/**
	 * 判断当前对象的内容是否大于所给的字符串对象内容（内部区分大小写）
	 * @param s {const string&} 输入的字符串对象引用
	 * @return {bool} 返回 true 表示当前字符串对象的内容大于输入的字符串
	 *  对象内容
	 */
	bool operator>(const string& s) const;

	/**
	 * 将当前对象直接转为字符串指针（即将内部缓冲区直接导出）
	 * @return {const char*} 返回值永远为非空指针，有可能为空串
	 */
	operator const char*() const;

	/**
	 * 将当前字符串对象直接转为通用指针（即将内部缓冲区直接导出）
	 * @return {const char*} 返回值永远为非空指针
	 */
	operator const void*() const;

	/**
	 * 将一个有符号字符添加进当前字符串对象的尾部
	 * @param ch {char} 有符号字符
	 * @return {string&} 当前字符串对象的引用
	 */
	string& push_back(char ch);

	/**
	 * 比较两个字符串对象的内容是否相同（区分大小写）
	 * @param s {const string&} 输入的字符串对象的引用
	 * @param case_sensitive {bool} 为 true 表示区分大小写
	 * @return {bool} 返回 true 表示二者相等
	 */
	bool equal(const string& s, bool case_sensitive = true) const;

	/**
	 * 检查当前 string 对象是否以指定的字符串开始
	 * @param s {const char*}
	 * @param case_sensitive {bool} 是否区分大小写
	 * @return {bool}
	 */
	bool begin_with(const char* s, bool case_sensitive = true) const;
	bool begin_with(const char* s, size_t len, bool case_sensitive = true) const;
	bool begin_with(const string& s, bool case_sensitive = true) const;
	bool begin_with(const void* v, size_t len) const;

	/**
	 * 检查当前 string 对象是否以指定的字符串结束
	 * @param s {const char*}
	 * @param case_sensitive {bool} 是否区分大小写
	 * @return {bool}
	 */
	bool end_with(const char* s, bool case_sensitive = true) const;
	bool end_with(const char* s, size_t len, bool case_sensitive = true) const;
	bool end_with(const string& s, bool case_sensitive = true) const;
	bool end_with(const void* v, size_t len) const;

	/**
	 * 比较两个字符串对象的内容是否相同（区分大小写）
	 * @param s {const string&} 输入的字符串对象的引用
	 * @return {int} 0：表示二者相同； > 0：当前字符串内容大于输入的内容；
	 *  < 0 ：当前字符串内容小于输入的内容
	 */
	int compare(const string& s) const;

	/**
	 * 比较两个字符串对象的内容是否相同（区分大小写）
	 * @param s {const string&} 输入的字符串对象的指针
	 * @return {int} 0：表示二者相同； > 0：当前字符串内容大于输入的内容；
	 *  < 0 ：当前字符串内容小于输入的内容
	 */
	int compare(const string* s) const;

	/**
	 * 比较两个字符串的内容是否相同
	 * @param s {const string&} 输入的字符串对象的引用
	 * @param case_sensitive {bool} 为 true 表示区分大小写
	 * @return {int} 0：表示二者相同； > 0：当前字符串内容大于输入的内容；
	 *  < 0 ：当前字符串内容小于输入的内容
	 */
	int compare(const char* s, bool case_sensitive = true) const;

	/**
	 * 比较当前对象的缓冲区内容是否与所给的缓冲区的内容相同
	 * @param ptr {const void*} 输入的缓冲区地址
	 * @param len {size_t} ptr 的缓冲区内数据长度
	 * @return {int} 返回结果含义如下:
	 *  0：表示二者相同；
	 *  > 0：当前对象缓冲区内容大于输入的内容；
	 *  < 0 ：当前对象缓冲内容小于输入的内容
	 */
	int compare(const void* ptr, size_t len) const;

	/**
	 * 比较当前对象缓冲区内容是否与所给的缓冲区的内容相同，限定比较数据长度
	 * @param s {const void*} 输入的缓冲区地址
	 * @param len {size_t} ptr 的缓冲区内数据长度
	 * @param case_sensitive {bool} 为 true 表示区分大小写
	 * @return {int} 0：表示二者相同； > 0：当前对象缓冲区内容大于输入的内容；
	 *  < 0 ：当前对象缓冲内容小于输入的内容
	 */
	int ncompare(const char* s, size_t len, bool case_sensitive = true) const;

	/**
	 * 从尾部向前比较当前对象的缓冲区内容是否与所给的缓冲区的内容相同，
	 * 限定比较数据长度
	 * @param s {const void*} 输入的缓冲区地址
	 * @param len {size_t} ptr 的缓冲区内数据长度
	 * @param case_sensitive {bool} 为 true 表示区分大小写
	 * @return {int} 0：表示二者相同；
	 *  > 0：当前对象缓冲区内容大于输入的内容；
	 *  < 0 ：当前对象缓冲内容小于输入的内容
	 */
	int rncompare(const char* s, size_t len, bool case_sensitive = true) const;

	/**
	 * 在当前字符串缓冲区中查找空行的位置，可以循环调用本方法以获得所有的
	 * 符合条件的内容
	 * @param left_count {int*} 当该指针非空时存储当前字符串剩余的数据长度
	 * @param buf {string*} 当找到空行时，则将上一次空行(不含该空行)和
	 *  本次空行(包含该空行)之间的数据存放于该缓冲区内，注：内部并不负责
	 *  清空该缓冲区，因为采用数据追加方式
	 * @return {int} 返回 0 表示未找到空行；返回值 > 0 表示空行的下一个
	 *  位置(因为需要找到一个空行返回的是该空行的下一个位置，所以若找到
	 *  空行则返回值一定大于 0)；返回值 < 0 表示内部出错
	 */
	int find_blank_line(int* left_count = NULL, string* buf = NULL);

	/**
	 * 重置内部查询状态，当需要重新开始调用 find_blank_line 时需要调用本
	 * 方法以重置内部查询状态
	 * @return {string&}
	 */
	string& find_reset(void);

	/**
	 * 查找指定字符在当前对象缓冲区的位置（下标从 0 开始）
	 * @param n {char} 要查找的有符号字符
	 * @return {int} 字符在缓冲区中的位置，若返回值 < 0 则表示不存在
	 */
	int find(char n) const;

	/**
	 * 查找指定字符串在当前对象缓冲区的起始位置（下标从 0 开始）
	 * @param needle {const char*} 要查找的有符号字符串
	 * @param case_sensitive {bool} 为 true 表示区分大小写
	 * @return {char*} 字符串在缓冲区中的起始位置，返回空指针则表示不存在
	 */
	char* find(const char* needle, bool case_sensitive=true) const;

	/**
	 * 从尾部向前查找指定字符串在当前对象缓冲区的起始位置（下标从 0 开始）
	 * @param needle {const char*} 要查找的有符号字符串
	 * @param case_sensitive {bool} 为 true 表示区分大小写
	 * @return {char*} 字符串在缓冲区中的起始位置，若返回值为空指针则表示不存在
	 */
	char* rfind(const char* needle, bool case_sensitive=true) const;

	/**
	 * 返回从当前字符串对象中缓冲区指定位置以左的内容
	 * @param n {size_t} 下标位置，当该值大于等于当前字符串的数据长度时，
	 *  则返回整个字符串对象；返回值不包含该值指定位置的字符内容
	 * @return {string} 返回值为一完整的对象，不需要单独释放，该函数的效率
	 *  可能并不太高
	 */
	string left(size_t n);

	/**
	 * 返回从当前字符串对象中缓冲区指定位置以右的内容
	 * @param n {size_t} 下标位置，当该值大于等于当前字符串的数据长度时，
	 *  则返回的字符串对象内容为空；返回值不包含该值指定位置的字符内容
	 * @return {const string} 返回值为一完整的对象，不需要单独释放，该
	 *  函数的效率可能并不太高
	 */
	string right(size_t n);

	/**
	 * 将当前对象的缓冲内容拷贝一部分数据至目标缓冲内
	 * @param buf {void*} 目标缓冲区地址
	 * @param size {size_t} buf 缓冲区长度
	 * @param move {bool} 在拷贝完数据后，是否需要将后面的数据向前移动并
	 *  覆盖前面的已拷贝的数据
	 * @return {size_t} 返回拷贝的实际字节数，当 empty() == true 时，则返回 0
	 */
	size_t scan_buf(void* buf, size_t size, bool move = false);

	/**
	 * 从当前对象的缓冲区中拷贝一行数据(包含"\r\n")至目标缓冲区内，当数据
	 * 被拷贝至目标缓冲区后，在源缓冲区内未被拷贝的数据会发生移动并覆盖被
	 * 拷贝的数据区域
	 * @param out {string&} 目标缓冲区，函数内部不会先自动清空该缓冲区
	 * @param nonl {bool} 返回的一行数据是否去掉尾部的 "\r\n" 或 "\n"
	 * @param n {size_t*} 该参数为非空指针时，则存储拷贝到的数据长度；当读
	 *  到一个空行且 nonl 为 true 时，则该地址存储 0
	 * @param move {bool} 在拷贝完数据后，是否需要将后面的数据向前移动并
	 *  覆盖前面的已拷贝的数据
	 * @return {bool} 是否拷贝了一个完整行数据，如果返回 false 还需要根据
	 *  empty() == true 来判断当前缓冲区中是否还有数据
	 */
	bool scan_line(string& out, bool nonl = true, size_t* n = NULL,
		bool move = false);

	/**
	 * 当使用 scan_xxx 类方法对缓冲区进行操作时未指定 move 动作，则调用本
	 * 函数可以使缓冲区内剩余的数据向前移动至缓冲区首部
	 * @return {size_t} 移动的字节数
	 */
	size_t scan_move();

	/**
	 * 返回当前对象缓冲区中第一个不含数据的尾部地址
	 * @return {char*} 返回值为 NULL 则说明内部数据为空，即 empty() == true
	 */
	char* buf_end(void);

	/**
	 * 返回当前对象缓冲区的起始地址
	 * @return {void*} 返回地址永远非空
	 */
	void* buf() const;

	/**
	 * 以字符串方式返回当前对象缓冲区的起始地址
	 * @return {char*} 返回地址永远非空
	 */
	char* c_str() const;

	/**
	 * 返回当前对象字符串的长度（不含\0）
	 * @return {size_t} 返回值 >= 0
	 */
	size_t length() const;

	/**
	 * 返回当前对象字符串的长度（不含\0），功能与 length 相同
	 * @return {size_t} 返回值 >= 0
	 */
	size_t size() const;

	/**
	 * 返回当前对象的缓冲区的空间长度，该值 >= 缓冲区内数据长度
	 * @return {size_t} 返回值 > 0
	 */
	size_t capacity() const;

	/**
	 * 判断当前对象的缓冲区内数据长度是否为 0
	 * @return {bool} 返回 true 表示数据为空
	 */
	bool empty() const;

	/**
	 * 返回当前对象内部所用的 acl C 库中的 ACL_VSTRING 对象地址
	 * @return {ACL_VSTRING*} 返回值永远非空
	 */
	ACL_VSTRING* vstring(void) const;

	/**
	 * 将当前对象的缓冲区的下标位置移至指定位置
	 * @param n {size_t} 目标下标位置，当该值 >= capacity 时，内部会
	 *  重新分配更大些的内存
	 * @return {string&} 当前对象的引用
	 */
	string& set_offset(size_t n);

	/**
	 * 调用该函数可以预先保证所需要的缓冲区大小
	 * @param n {size_t} 希望的缓冲区空间大小值
	 * @return {string&} 当前对象的引用
	 */
	string& space(size_t n);

	/**
	 * 将当前对象存储的字符串进行分割
	 * @param sep {const char*} 进行分割时的分割标记
	 * @param quoted {bool} 当为 true 时，则对于由单/双引号引起来的
	 *  字符串内容，不做分割，但此时要求 sep 中不得存在单/双号
	 * @return {std::list<string>&} 返回 list 格式的分割结果，返回的结果
	 *  不需要释放，其引用了当前对象的一个内部指针
	 */
	std::list<string>& split(const char* sep, bool quoted = false);

	/**
	 * 将当前对象存储的字符串进行分割
	 * @param sep {const char*} 进行分割时的分割标记
	 * @param quoted {bool} 当为 true 时，则对于由单/双引号引起来的
	 *  字符串内容，不做分割，但此时要求 sep 中不得存在单/双号
	 * @return {std::vector<string>&} 返回 vector 格式的分割结果，返回的
	 *  结果不需要释放，其引用了当前对象的一个内部指针
	 */
	std::vector<string>& split2(const char* sep, bool quoted = false);

	/**
	 * 以 '=' 为分隔符将当前对象存储的字符串分割成 name/value 对，分割时会
	 * 自动去掉源字符串的起始处、结尾处以及分隔符 '=' 两边的空格及 TAB
	 * @return {std::pair<string, string>&} 如果当前对象存储的字符串
	 *  不符合分割条件（即不是严格的 name=value格式），则返回的结果中字符
	 *  串对象为空串,返回的结果不需要释放，其引用了当前对象的一个内部地址
	 */
	std::pair<string, string>& split_nameval(void);

	/**
	 * 将字符串拷贝到当前对象的缓冲区中
	 * @param ptr {const char*} 源字符串地址，需以 '\0' 结束
	 * @return {string&} 当前对象的引用
	 */
	string& copy(const char* ptr);

	/**
	 * 将源数据的定长数据拷贝至当前对象的缓冲区中
	 * @param ptr {const void*} 源数据地址
	 * @param len {size_t} ptr 源数据长度
	 * @return {string&} 当前对象的引用
	 */
	string& copy(const void* ptr, size_t len);

	/**
	 * 将源字符串的数据移动至当前对象的缓冲区中，内部会自动判断源数据
	 * 地址是否就在当前对象的缓冲区中
	 * @param src {const char*} 源数据地址
	 * @return {string&} 当前对象的引用
	 */
	string& memmove(const char* src);

	/**
	 * 将源字符串的数据移动至当前对象的缓冲区中，内部会自动判断源数据
	 * 地址是否就在当前对象的缓冲区中
	 * @param src {const char*} 源数据地址
	 * @param len {size_t} 移动数据的长度
	 * @return {string&} 当前对象的引用
	 */
	string& memmove(const char* src, size_t len);

	/**
	 * 将指定字符串添加在当前对象数据缓冲区数据的尾部
	 * @param s {const string&} 源数据对象引用
	 * @return {string&} 当前对象的引用
	 */
	string& append(const string& s);

	/**
	 * 将指定字符串添加在当前对象数据缓冲区数据的尾部
	 * @param s {const string&} 源数据对象指针
	 * @return {string&} 当前对象的引用
	 */
	string& append(const string* s);

	/**
	 * 将指定字符串添加在当前对象数据缓冲区数据的尾部
	 * @param s {const string&} 源数据对象指针
	 * @return {string&} 当前对象的引用
	 */
	string& append(const char* s);

	/**
	 * 将指定缓冲区中的数据添加在当前对象数据缓冲区数据的尾部
	 * @param ptr {const void*} 源数据对象指针
	 * @param len {size_t} ptr 数据长度
	 * @return {string&} 当前对象的引用
	 */
	string& append(const void* ptr, size_t len);

	/**
	 * 将指定字符串数据添加在当前对象数据缓冲区数据的首部
	 * @param s {const char*} 源数据地址
	 * @return {string&} 当前对象的引用
	 */
	string& prepend(const char* s);

	/**
	 * 将指定内存数据添加在当前对象数据缓冲区数据的首部
	 * @param ptr {const void*} 源数据地址
	 * @param len {size_t} ptr 数据长度
	 * @return {string&} 当前对象的引用
	 */
	string& prepend(const void* ptr, size_t len);

	/**
	 * 将内存数据插入指定下标位置开始的当前对象缓冲区中
	 * @param start {size_t} 当前对象缓冲区的开始插入下标值
	 * @param ptr {const void*} 内存数据的地址
	 * @param len {size_t} 内存数据的长度
	 * @return {string&} 当前对象的引用
	 */
	string& insert(size_t start, const void* ptr, size_t len);

	/**
	 * 带格式方式的添加数据（类似于 sprintf 接口方式）
	 * @param fmt {const char*} 格式字符串
	 * @param ... 变参数据
	 * @return {string&} 当前对象的引用
	 */
	string& format(const char* fmt, ...) ACL_CPP_PRINTF(2, 3);

	/**
	 * 带格式方式的添加数据（类似于 vsprintf 接口方式）
	 * @param fmt {const char*} 格式字符串
	 * @param ap {va_list} 变参数据
	 * @return {string&} 当前对象的引用
	 */
	string& vformat(const char* fmt, va_list ap);

	/**
	 * 带格式方式在当前对象的尾部添加数据
	 * @param fmt {const char*} 格式字符串
	 * @param ... 变参数据
	 * @return {string&} 当前对象的引用
	 */
	string& format_append(const char* fmt, ...)  ACL_CPP_PRINTF(2, 3);

	/**
	 * 带格式方式在当前对象的尾部添加数据
	 * @param fmt {const char*} 格式字符串
	 * @param ap {va_list} 变参数据
	 * @return {string&} 当前对象的引用
	 */
	string& vformat_append(const char* fmt, va_list ap);

	/**
	 * 将当前对象中的数据的字符进行替换
	 * @param from {char} 源字符
	 * @param to {char} 目标字符
	 * @return {string&} 当前对象的引用
	 */
	string& replace(char from, char to);

	/**
	 * 将当前对象的数据截短，缓冲区内部移动下标指针地址
	 * @param n {size_t} 数据截短后的数据长度，如果该值 >= 当前缓冲区
	 *  数据长度，则内部不做任何变化
	 * @return {string&} 当前对象的引用
	 */
	string& truncate(size_t n);

	/**
	 * 在当前对象的缓冲区数据中去掉指定的字符串内容，在处理过程中会发生
	 * 数据移动情况
	 * @param needle {const char*} 指定需要去掉的字符串数据
	 * @param each {bool} 当为 true 时，则每一个出现在 needle 中的字符都
	 *  会在当前对象的缓存区中去掉；否则，仅在当前对象缓冲区中去掉完整的
	 *  needle 字符串
	 * @return {string&} 当前对象的引用
	 *  如 acl::string s("hello world!");
	 *  若 s.strip("hel", true), 则结果为： s == "o word!"
	 *  若 s.strip("hel", false), 则结果为: s = "lo world!"
	 */
	string& strip(const char* needle, bool each = false);

	/**
	 * 将当前对象缓冲区左边的空白（包含空格及TAB）去掉
	 * @return {string&} 当前对象的引用
	 */
	string& trim_left_space();

	/**
	 * 将当前对象缓冲区右边的空白（包含空格及TAB）去掉
	 * @return {string&} 当前对象的引用
	 */
	string& trim_right_space();

	/**
	 * 将当前对象缓冲区中所有的空白（包含空格及TAB）去掉
	 * @return {string&} 当前对象的引用
	 */
	string& trim_space();

	/**
	 * 将当前对象缓冲区左边的回车换行符去掉
	 * @return {string&} 当前对象的引用
	 */
	string& trim_left_line();

	/**
	 * 将当前对象缓冲区右边的回车换行符去掉
	 * @return {string&} 当前对象的引用
	 */
	string& trim_right_line();

	/**
	 * 将当前对象缓冲区中所有的回车换行符去掉
	 * @return {string&} 当前对象的引用
	 */
	string& trim_line();

	/**
	 * 清空当前对象的数据缓冲区
	 * @return {string&} 当前对象的引用
	 */
	string& clear();

	/**
	 * 将当前对象的数据缓冲区中的数据均转为小写
	 * @return {string&} 当前对象的引用
	 */
	string& lower(void);

	/**
	 * 将当前对象的数据缓冲区中的数据均转为大写
	 * @return {string&} 当前对象的引用
	 */
	string& upper(void);

	/**
	 * 从当前缓冲区中将指定偏移量指定长度的数据拷贝至目标缓冲区中
	 * @param out {string&} 目标缓冲区，内部采用追加方式，并不清空该对象
	 * @param p {size_t} 当前缓冲区的起始位置
	 * @param len {size_t} 从 p 起始位置开始拷贝的数据量，当该值为 0 时
	 *  则拷贝指定 p 位置后所有的数据，否则拷贝指定长度的数据，若指定的
	 *  数据长度大于实际要拷贝的长度，则仅拷贝实际存在的数据
	 * @return {size_t} 返回拷贝的实际数据长度，p 越界时则该返回值为 0
	 */
	size_t substr(string& out, size_t p = 0, size_t len = 0) const;

	/**
	 * 将当前对象的数据缓冲区中的数据进行 base64 转码
	 * @return {string&} 当前对象的引用
	 */
	string& base64_encode(void);

	/**
	 * 将输入的源数据进行 base64 转码并存入当前对象的缓冲区中
	 * @param ptr {const void*} 源数据的地址
	 * @param len {size_t} 源数据长度
	 * @return {string&} 当前对象的引用
	 */
	string& base64_encode(const void* ptr, size_t len);

	/**
	 * 如果当前对象的缓冲区中的数据是经 base64 编码的，则此函数将这些
	 * 数据进行解码
	 * @return {string&} 当前对象的引用，如果解码出错，则内部缓冲区会被自动清空，
	 *  调用 string::empty() 返回 true
	 */
	string& base64_decode(void);

	/**
	 * 将输入的 base64 编码的数据进行解码并存入当前对象的缓冲区中
	 * @param s {const char*} 经 base64 编码的源数据
	 * @return {string&} 当前对象的引用，如果解码出错，则内部缓冲区会被自动清空，
	 *  调用 string::empty() 返回 true
	 */
	string& base64_decode(const char* s);

	/**
	 * 将输入的 base64 编码的数据进行解码并存入当前对象的缓冲区中
	 * @param ptr {const void*} 经 base64 编码的源数据
	 * @param len {size_t} ptr 数据长度
	 * @return {string&} 当前对象的引用，如果解码出错，则内部缓冲区会被自动清空，
	 *  调用 string::empty() 返回 true
	 */
	string& base64_decode(const void* ptr, size_t len);

	/**
	 * 将输入的源数据进行 url 编码并存入当前对象的缓冲区中
	 * @param s {const char*} 源数据
	 * @param dbuf {dbuf_pool*} 内存池对象，如果非空，则内部的动态内存在
	 *  该对象上分配且当调用者在释放该对象时内部临时动态内存随之被释放，
	 *  否则使用 acl_mymalloc 分配并自动释放
	 * @return {string&} 当前对象的引用
	 */
	string& url_encode(const char* s, dbuf_pool* dbuf = NULL);

	/**
	 * 将输入的用 url 编码的源数据解码并存入当前对象的缓冲区中
	 * @param s {const char*} 经 url 编码的源数据
	 * @param dbuf {dbuf_pool*} 内存池对象，如果非空，则内部的动态内存在
	 *  该对象上分配且当调用者在释放该对象时内部临时动态内存随之被释放，
	 * @return {string&} 当前对象的引用
	 */
	string& url_decode(const char* s, dbuf_pool* dbuf = NULL);

	/**
	 * 将源数据进行 H2B 编码并存入当前对象的缓冲区中
	 * @param s {const void*} 源数据地址
	 * @param len {size_t} 源数据长度
	 * @return {string&} 当前对象的引用
	 */
	string& hex_encode(const void* s, size_t len);

	/**
	 * 将源数据进行 H2B 解码并存入当前对象的缓冲区中
	 * @param s {const char*} 源数据地址
	 * @param len {size_t} 源数据长度
	 * @return {string&} 当前对象的引用
	 */
	string& hex_decode(const char* s, size_t len);

	/**
	 * 从文件全路径中提取文件名
	 * @param path {const char*} 文件全路径字符串，非空字符串
	 * @return {string&} 当前对象的引用
	 */
	string& basename(const char* path);

	/**
	 * 从文件全路径中提取文件所在目录
	 * @param path {const char*} 文件全路径字符串，非空字符串
	 * @return {string&} 当前对象的引用
	 */
	string& dirname(const char* path);

	/**
	 * 将 32 位有符号整数转为字符串存（内部使用了线程局部变量）
	 * @param n {int} 32 位有符号整数
	 * @return {string&} 转换结果对象的引用，其引用了内部的一个线程局部变量
	 */
	static string& parse_int(int n);

	/**
	 * 将 32 位无符号整数转为字符串存（内部使用了线程局部变量）
	 * @param n {int} 32 位无符号整数
	 * @return {string&} 转换结果对象的引用，其引用了内部的一个线程局部变量
	 */
	static string& parse_int(unsigned int n);
#if defined(_WIN32) || defined(_WIN64)
	static string& parse_int64(__int64 n);
	static string& parse_int64(unsigned __int64 n);
#else
	/**
	 * 将 64 位有符号整数转为字符串存（内部使用了线程局部变量）
	 * @param n {long long int} 64 位有符号整数
	 * @return {string&} 转换结果对象的引用，其引用了内部的一个线程局部变量
	 */
	static string& parse_int64(long long int n);

	/**
	 * 将 64 位无符号整数转为字符串存（内部使用了线程局部变量）
	 * @param n {unsigned long long int} 64 位无符号整数
	 * @return {string&} 转换结果对象的引用，其引用了内部的一个线程局部变量
	 */
	static string& parse_int64(unsigned long long int n);
#endif

	/**
	 * 模板函数，可用在以下场景:
	 * string s1, s2;
	 * T v;
	 * s1 = s2 + v;
	 */
	template<typename T>
	string operator+(T v)
	{
		string s(*this);
		s += v;
		return s;
	}

private:
	ACL_VSTRING* vbf_;
	char* scan_ptr_;
	std::list<string>* list_tmp_;
	std::vector<string>* vector_tmp_;
	std::pair<string, string>* pair_tmp_;
	ACL_LINE_STATE* line_state_;
	int  line_state_offset_;
	bool use_bin_;

	void init(size_t len);
};

/**
 * 模板函数，可用在以下场景:
 * string s1, s2;
 * T v;
 * s1 = v + s2;
 */
template<typename T>
string operator+(T v, const string& rhs)
{
	string s;
	s = v;
	s += rhs;
	return s;
}

/**
 * 示例:
 * string s, s1 = "hello", s2 = "world";
 * s = s1 + " " + s2;
 * s = ">" + s1 + " " + s2;
 * s = 1000 + s1 + " " + s2 + 1000;
 */

} // namespce acl
