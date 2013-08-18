#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <vector>
#include <list>
#include <utility>

struct ACL_VSTRING;

namespace acl {

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
	string(size_t n = 64, bool bin = false);

	/**
	 * 构造函数
	 * @param s {const string&} 源字符串对象，初始化后的类对象内部自动复制该字符串
	 */
	string(const string& s);

	/**
	 * 构造函数
	 * @param s {const char*} 内部自动用该字符串初始化类对象，s 必须是以 \0 结尾
	 */
	string(const char* s);

	/**
	 * 构造函数
	 * @param s {const char*} 源缓冲内容
	 * @param n {size_t} s 缓冲区数据长度
	 */
	string(const void* s, size_t n);
	~string(void);

	/**
	 * 设置字符串类对象为二进制处理模式
	 * @param bin {bool} 当该值为 true 时，则设置字符串类对象为二进制处理方式；
	 *  否则为文本方式；该值为 true 时，则当调用 += int|int64|short|char
	 *  或调用 << int|int64|short|char 时，则按二进制方式处理，否则按文本方式处理
	 */
	void set_bin(bool bin);

	/**
	 * 判断当前字符串类对象是否为二进制处理方式 
	 * @return {bool} 返回值为 true 时则表示为二进制方式
	 */
	bool get_bin() const;

	/**
	 * 根据字符数组下标获得指定位置的字符，输入参数必须为合法值，否则则内部产生断言
	 * @param n {size_t} 指定的位置（该值 >= 0 且 < 字符串长度)，如果越界，则产生断言
	 * @return {char} 返回指定位置的字符
	 */
	char operator[](size_t n);

	/**
	 * 根据字符数组下标获得指定位置的字符，输入参数必须为合法值，否则则内部产生断言
	 * @param n {int} 指定的位置（该值 >= 0 且 < 字符串长度)，如果越界，则产生断言
	 * @return {char} 返回指定位置的字符
	 */
	char operator[](int n);

	/**
	 * 对目标字符串类对象赋值
	 * @param s {const char*} 源字符串
	 * @return {string&} 返回当前字符串类对象的引用，便于对该类对象连续进行操作
	 */
	string& operator=(const char* s);

	/**
	 * 对目标字符串类对象赋值
	 * @param s {const string&} 源字符串对象
	 * @return {string&} 返回当前字符串类对象的引用，便于对该类对象连续进行操作
	 */
	string& operator=(const string&);

	/**
	 * 对目标字符串类对象赋值
	 * @param s {const string*} 源字符串对象
	 * @return {string&} 返回当前字符串类对象的引用，便于对该类对象连续进行操作
	 */
	string& operator=(const string*);

#ifdef WIN32
	/**
	 * 对目标字符串类对象赋值
	 * @param n {long long int} 源 64 位符号长整数，若字符串对象的当前状态为
	 *  二进制模式，则该函数便会以二进制方式赋值给字符串对象，否则以文本方式赋值给
	 *  字符串对象；关于二进制模式还是文本方式，其含义参见 set_bin(bool)
	 * @return {string&} 返回当前字符串类对象的引用，便于对该类对象连续进行操作
	 */
	string& operator=(__int64 n);

	/**
	 * 对目标字符串类对象赋值
	 * @param n {unsinged long long int} 源 64 位无符号长整数，若字符串对象
	 *  的当前状态为二进制模式，则该函数便会以二进制方式赋值给字符串对象，否则以文本方式
	 *  赋值给字符串对象；关于二进制模式还是文本方式，其含义参见 set_bin(bool)
	 * @return {string&} 返回当前字符串类对象的引用，便于对该类对象连续进行操作
	 */
	string& operator=(unsigned __int64);
#else
	string& operator=(long long int);
	string& operator=(unsigned long long int);
#endif

	/**
	 * 对目标字符串类对象赋值
	 * @param n {char} 源有符号字符；若字符串对象的当前状态为二进制模式，则该函数
	 *  便会以二进制方式赋值给字符串对象，否则以文本方式赋值给字符串对象；关于二进制模
	 *  式还是文本方式，其含义参见 set_bin(bool)
	 * @return {string&} 返回当前字符串类对象的引用，便于对该类对象连续进行操作
	 */
	string& operator=(char n);

	/**
	 * 对目标字符串类对象赋值
	 * @param n {char} 源无符号字符；若字符串对象的当前状态为二进制模式，则该函数
	 *  便会以二进制方式赋值给字符串对象，否则以文本方式赋值给字符串对象；关于二进制模
	 *  式还是文本方式，其含义参见 set_bin(bool)
	 * @return {string&} 返回当前字符串类对象的引用，便于对该类对象连续进行操作
	 */
	string& operator=(unsigned char n);

	/**
	 * 对目标字符串类对象赋值
	 * @param n {char} 源有符号长整型；若字符串对象的当前状态为二进制模式，则该函数
	 *  便会以二进制方式赋值给字符串对象，否则以文本方式赋值给字符串对象；关于二进制模
	 *  式还是文本方式，其含义参见 set_bin(bool)
	 * @return {string&} 返回当前字符串类对象的引用，便于对该类对象连续进行操作
	 */
	string& operator=(long n);

	/**
	 * 对目标字符串类对象赋值
	 * @param n {char} 源无符号长整型；若字符串对象的当前状态为二进制模式，则该函数
	 *  便会以二进制方式赋值给字符串对象，否则以文本方式赋值给字符串对象；关于二进制模
	 *  式还是文本方式，其含义参见 set_bin(bool)
	 * @return {string&} 返回当前字符串类对象的引用，便于对该类对象连续进行操作
	 */
	string& operator=(unsigned long n);

	/**
	 * 对目标字符串类对象赋值
	 * @param n {char} 源有符号整型；若字符串对象的当前状态为二进制模式，则该函数
	 *  便会以二进制方式赋值给字符串对象，否则以文本方式赋值给字符串对象；关于二进制模
	 *  式还是文本方式，其含义参见 set_bin(bool)
	 * @return {string&} 返回当前字符串类对象的引用，便于对该类对象连续进行操作
	 */
	string& operator=(int n);

	/**
	 * 对目标字符串类对象赋值
	 * @param n {char} 源无符号整型；若字符串对象的当前状态为二进制模式，则该函数
	 *  便会以二进制方式赋值给字符串对象，否则以文本方式赋值给字符串对象；关于二进制模
	 *  式还是文本方式，其含义参见 set_bin(bool)
	 * @return {string&} 返回当前字符串类对象的引用，便于对该类对象连续进行操作
	 */
	string& operator=(unsigned int n);

	/**
	 * 对目标字符串类对象赋值
	 * @param n {char} 源有符号短整型；若字符串对象的当前状态为二进制模式，则该函数
	 *  便会以二进制方式赋值给字符串对象，否则以文本方式赋值给字符串对象；关于二进制模
	 *  式还是文本方式，其含义参见 set_bin(bool)
	 * @return {string&} 返回当前字符串类对象的引用，便于对该类对象连续进行操作
	 */
	string& operator=(short n);

	/**
	 * 对目标字符串类对象赋值
	 * @param n {char} 源无符号短整型；若字符串对象的当前状态为二进制模式，则该函数
	 *  便会以二进制方式赋值给字符串对象，否则以文本方式赋值给字符串对象；关于二进制模
	 *  式还是文本方式，其含义参见 set_bin(bool)
	 * @return {string&} 返回当前字符串类对象的引用，便于对该类对象连续进行操作
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
	string& operator+=(const string*);
#ifdef WIN32
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
	string& operator+=(unsigned short);

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
	 * @param s {const char*} 源字符串指针
	 * @return {string&} 目标字符串对象的引用
	 */
	string& operator<<(const char* s);
#ifdef WIN32
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
	 * @return {string&} 源字符串对象的引用
	 */
	string& operator>>(string* s);
#ifdef WIN32
	/**
	 * 将字符串对象中的内容赋予目标 64 位有符号整数
	 * @param n {string*} 目标 64 位有符号整数
	 * @return {string&} 源字符串对象的引用
	 */
	string& operator>>(__int64& n);

	/**
	 * 将字符串对象中的内容赋予目标 64 位无符号整数
	 * @param n {string*} 目标 64 位无符号整数
	 * @return {string&} 源字符串对象的引用
	 */
	string& operator>>(unsigned __int64& n);
#else
	string& operator>>(long long int&);
	string& operator>>(unsigned long long int&);
#endif

	/**
	 * 将字符串对象中的内容赋予目标 32 位有符号整数
	 * @param n {string*} 目标 32 位有符号整数
	 * @return {string&} 源字符串对象的引用
	 */
	string& operator>>(int& n);

	/**
	 * 将字符串对象中的内容赋予目标 32 位无符号整数
	 * @param n {string*} 目标 32 位无符号整数
	 * @return {string&} 源字符串对象的引用
	 */
	string& operator>>(unsigned int& n);

	/**
	 * 将字符串对象中的内容赋予目标 16 位有符号整数
	 * @param n {string*} 目标 16 位有符号整数
	 * @return {string&} 源字符串对象的引用
	 */
	string& operator>>(short& n);

	/**
	 * 将字符串对象中的内容赋予目标 16 位无符号整数
	 * @param n {string*} 目标 16 位无符号整数
	 * @return {string&} 源字符串对象的引用
	 */
	string& operator>>(unsigned short& n);

	/**
	 * 将字符串对象中的内容赋予目标 8 位有符号字符
	 * @param n {string*} 目标 16 位有符号字符
	 * @return {string&} 源字符串对象的引用
	 */
	string& operator>>(char& n);

	/**
	 * 将字符串对象中的内容赋予目标 8 位无符号字符
	 * @param n {string*} 目标 16 位无符号字符
	 * @return {string&} 源字符串对象的引用
	 */
	string& operator>>(unsigned char& n);

	/**
	 * 判断当前字符串对象的内容与所给的字符串对象内容是否相等（内部区分大小写）
	 * @param s {const string&} 输入的字符串对象引用
	 * @return {bool} 返回 true 表示字符串内容相同
	 */
	bool operator==(const string& s) const;

	/**
	 * 判断当前字符串对象的内容与所给的字符串对象内容是否相等（内部区分大小写）
	 * @param s {const string&} 输入的字符串对象指针
	 * @return {bool} 返回 true 表示字符串内容相同
	 */
	bool operator==(const string* s) const;

	/**
	 * 判断当前字符串对象的内容与所给的字符串内容是否相等（内部区分大小写）
	 * @param s {const string&} 输入的字符串对象指针
	 * @return {bool} 返回 true 表示字符串内容相同
	 */
	bool operator==(const char* s) const;

	/**
	 * 判断当前字符串对象的内容与所给的字符串对象内容是否不等（内部区分大小写）
	 * @param s {const string&} 输入的字符串对象引用
	 * @return {bool} 返回 true 表示字符串内容不同
	 */
	bool operator!=(const string& s) const;

	/**
	 * 判断当前字符串对象的内容与所给的字符串对象内容是否不等（内部区分大小写）
	 * @param s {const string&} 输入的字符串对象指针
	 * @return {bool} 返回 true 表示字符串内容不同
	 */
	bool operator!=(const string* s) const;

	/**
	 * 判断当前字符串对象的内容与所给的字符串内容是否不等（内部区分大小写）
	 * @param s {const string&} 输入的字符串对象指针
	 * @return {bool} 返回 true 表示字符串内容不同
	 */
	bool operator!=(const char*) const;

	/**
	 * 判断当前字符串对象的内容是否小于所给的字符串对象内容（内部区分大小写）
	 * @param s {const string&} 输入的字符串对象引用
	 * @return {bool} 返回 true 表示当前字符串对象的内容小于输入的字符串
	 *  对象内容
	 */
	bool operator<(const string&) const;

	/**
	 * 判断当前字符串对象的内容是否大于所给的字符串对象内容（内部区分大小写）
	 * @param s {const string&} 输入的字符串对象引用
	 * @return {bool} 返回 true 表示当前字符串对象的内容大于输入的字符串
	 *  对象内容
	 */
	bool operator>(const string&) const;

	/**
	 * 将当前字符串对象直接转为字符串指针（即将内部缓冲区直接导出）
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
	 * @return {int} 0：表示二者相同； > 0：当前对象缓冲区内容大于输入的内容；
	 *  < 0 ：当前对象缓冲内容小于输入的内容
	 */
	int compare(const void* ptr, size_t len) const;

	/**
	 * 比较当前对象的缓冲区内容是否与所给的缓冲区的内容相同，限定比较数据长度
	 * @param s {const void*} 输入的缓冲区地址
	 * @param len {size_t} ptr 的缓冲区内数据长度
	 * @param case_sensitive {bool} 为 true 表示区分大小写
	 * @return {int} 0：表示二者相同； > 0：当前对象缓冲区内容大于输入的内容；
	 *  < 0 ：当前对象缓冲内容小于输入的内容
	 */
	int ncompare(const char* s, size_t len, bool case_sensitive = true) const;

	/**
	 * 从尾部向前比较当前对象的缓冲区内容是否与所给的缓冲区的内容相同，限定比较数据长度
	 * @param s {const void*} 输入的缓冲区地址
	 * @param len {size_t} ptr 的缓冲区内数据长度
	 * @param case_sensitive {bool} 为 true 表示区分大小写
	 * @return {int} 0：表示二者相同； > 0：当前对象缓冲区内容大于输入的内容；
	 *  < 0 ：当前对象缓冲内容小于输入的内容
	 */
	int rncompare(const char* s, size_t len, bool case_sensitive = true) const;

	/**
	 * 查找指定字符在当前对象缓冲区的位置（下标从 0 开始）
	 * @param n {char} 要查找的有符号字符
	 * @return {int} 字符在缓冲区中的位置，若返回值 < 0 则表示不存在
	 */
	int find(char n) const;

	/**
	 * 查找指定字符吕在当前对象缓冲区的起始位置（下标从 0 开始）
	 * @param needle {const char*} 要查找的有符号字符串
	 * @param case_sensitive {bool} 为 true 表示区分大小写
	 * @return {const char*} 字符串在缓冲区中的起始位置，若返回值为空指针则表示不存在
	 */
	const char* find(const char* needle, bool case_sensitive=true) const;

	/**
	 * 从尾部向前查找指定字符吕在当前对象缓冲区的起始位置（下标从 0 开始）
	 * @param needle {const char*} 要查找的有符号字符串
	 * @param case_sensitive {bool} 为 true 表示区分大小写
	 * @return {const char*} 字符串在缓冲区中的起始位置，若返回值为空指针则表示不存在
	 */
	const char* rfind(const char* needle, bool case_sensitive=true) const;

	/**
	 * 返回从当前字符串对象中缓冲区指定位置以左的内容
	 * @param size_t {npos} 下标位置，当该值大于等于当前字符串的数据长度时，
	 *  则返回整个字符串对象；返回值不包含该值指定位置的字符内容
	 * @return {const string} 返回值为一完整的对象，不需要单独释放，该函数的效率
	 *  可能并不太高
	 */
	const string left(size_t npos);

	/**
	 * 返回从当前字符串对象中缓冲区指定位置以右的内容
	 * @param size_t {npos} 下标位置，当该值大于等于当前字符串的数据长度时，
	 *  则返回的字符串对象内容为空；返回值不包含该值指定位置的字符内容
	 * @return {const string} 返回值为一完整的对象，不需要单独释放，该函数的效率
	 *  可能并不太高
	 */
	const string right(size_t npos);

	/**
	 * 将当前对象的缓冲内容拷贝一部分数据至目标缓冲内
	 * @param buf {void*} 目标缓冲区地址
	 * @param size {size_t} buf 缓冲区长度
	 * @return {string&} 当前对象的引用
	 */
	string& scan_buf(void* buf, size_t size);

	/**
	 * 返回当前对象缓冲区中第一个不含数据的尾部地址
	 * @return {char*} 返回值为 NULL 则可能是内部出错了
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
	 * @return {const std::list<string>&} 返回 list 格式的分割结果
	 */
	const std::list<string>& split(const char* sep);

	/**
	 * 将当前对象存储的字符串进行分割
	 * @param sep {const char*} 进行分割时的分割标记
	 * @return {const std::vector<string>&} 返回 vector 格式的分割结果
	 */
	const std::vector<string>& split2(const char*);

	/**
	 * 以 '=' 为分隔符将当前对象存储的字符串分割成 name/value 对，分割时会自动
	 * 去掉源字符串的起始处、结尾处以及分隔符 '=' 两边的空格及 TAB
	 * @return {const std::pair<string, string>&} 如果当前对象存储的字符串
	 *  不符合分割条件（即不是严格的 name=value格式），则返回的结果中字符串对象为空串
	 */
	const std::pair<string, string>& split_nameval(void);

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
	string& format(const char* fmt, ...);

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
	string& format_append(const char* fmt, ...);

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
	 * 在当前对象的缓冲区数据中去掉指定的字符串内容，在处理过程中会发生数据移动情况
	 * @param needle {const char*} 指定需要去掉的字符串数据
	 * @param each {bool} 是否去掉所有的指定字符串数据
	 * @return {string&} 当前对象的引用
	 */
	string& strip(const char* needle, bool each = false);

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
	 * 如果当前对象的缓冲区中的数据是经 base64 编码的，则此函数将这些数据进行解码
	 * @return {string&} 当前对象的引用
	 */
	string& base64_decode(void);

	/**
	 * 将输入的 base64 编码的数据进行解码并存入当前对象的缓冲区中
	 * @param s {const char*} 经 base64 编码的源数据
	 * @return {string&} 当前对象的引用
	 */
	string& base64_decode(const char* s);

	/**
	 * 将输入的 base64 编码的数据进行解码并存入当前对象的缓冲区中
	 * @param ptr {const void*} 经 base64 编码的源数据
	 * @param len {size_t} ptr 数据长度
	 * @return {string&} 当前对象的引用
	 */
	string& base64_decode(const void* ptr, size_t len);

	/**
	 * 将输入的源数据进行 url 编码并存入当前对象的缓冲区中
	 * @param s {const char*} 源数据
	 * @return {string&} 当前对象的引用
	 */
	string& url_encode(const char* s);

	/**
	 * 将输入的用 url 编码的源数据解码并存入当前对象的缓冲区中
	 * @param s {const char*} 经 url 编码的源数据
	 * @return {string&} 当前对象的引用
	 */
	string& url_decode(const char* s);

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
	 * 将 32 位有符号整数转为字符串存
	 * @param n {int} 32 位有符号整数
	 * @return {const string&} 转换结果对象的引用
	 */
	static const string& parse_int(int n);

	/**
	 * 将 32 位无符号整数转为字符串存
	 * @param n {int} 32 位无符号整数
	 * @return {const string&} 转换结果对象的引用
	 */
	static const string& parse_int(unsigned int n);
#ifdef WIN32
	static const string& parse_int64(__int64 n);


	static const string& parse_int64(unsigned __int64 n);
#else
	/**
	 * 将 64 位有符号整数转为字符串存
	 * @param n {long long int} 64 位有符号整数
	 * @return {const string&} 转换结果对象的引用
	 */
	static const string& parse_int64(long long int);

	/**
	 * 将 64 位无符号整数转为字符串存
	 * @param n {unsigned long long int} 64 位无符号整数
	 * @return {const string&} 转换结果对象的引用
	 */
	static const string& parse_int64(unsigned long long int);
#endif

private:
	bool m_bin;
	void init(size_t len);
	ACL_VSTRING* m_pVbf;
	const char* m_ptr;
	std::list<string>* m_psList;
	std::vector<string>* m_psList2;
	std::pair<string, string>* m_psPair;
};

} // namespce acl
