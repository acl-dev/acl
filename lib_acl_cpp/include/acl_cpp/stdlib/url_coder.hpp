#pragma once
#include "../acl_cpp_define.hpp"
#include "dbuf_pool.hpp"
#include <vector>

namespace acl {

class string;

struct URL_NV
{
	char* name;
	char* value;
};

class ACL_CPP_API url_coder : public dbuf_obj
{
public:
	/**
	 * 构造函数
	 * @param nocase {bool} 当为 true 时表示参数名不区别大小写
	 * @param dbuf {dbuf_guard*} 内存池对象
	 */
	url_coder(bool nocase = true, dbuf_guard* dbuf = NULL);

	/**
	 * 构造函数，通过类实例对象构造
	 * @param coder {const url_coder&}
	 * @param dbuf {dbuf_guard*} 内存池对象
	 */
	url_coder(const url_coder& coder, dbuf_guard* dbuf = NULL);

	~url_coder(void);

	/**
	 * 将存储于 params_ 数组中的数据进行 url 编码
	 * @param buf {string&} 存储编码后的结果
	 * @param clean {bool} 是否清空传入的 buf 缓冲区
	 */
	void encode(string& buf, bool clean = true) const;

	/**
	 * 获得将数组对象转换为编码后的字符串对象
	 * @return {const string&}
	 */
	const string& to_string(void) const;

	/**
	 * 解析以 URL 编码的字符串
	 * @param str {const char*} url 编码形式的字符串
	 */
	void decode(const char* str);
	
	/**
	 * 采用 url 编码时，调用此函数添加变量
	 * @param name {const char*} 变量名
	 * @param value 变量值
	 * @param override {bool} 如果存在同名变量是否直接覆盖
	 * @return 返回 url_coder 对象的引用
	 */
	url_coder& set(const char* name, const char* value,
		bool override = true);
	url_coder& set(const char* name, int value, bool override = true);
	url_coder& set(const char* name, bool override, const char* fmt, ...)
		ACL_CPP_PRINTF(4, 5);
	url_coder& set(const char* name, const char* fmt, va_list ap,
		bool override = true);

	/**
	 * 获得 URL 解码后 params_ 数组中某个变量名的值
	 * @param name {const char*} 变量名
	 * @param found {bool*} 该指针非 NULL 时，将存储 name 是否存在，主要
	 *  用在 name 的值为空的情形
	 * @return {const char*} 返回 NULL 表示不存在
	 */
	const char* get(const char* name, bool* found = NULL) const;

	/**
	 * 获得 URL 解码后 params_ 数组中某个变量名的值
	 * @param name {const char*} 变量名
	 * @return {const char*} 返回 NULL 表示不存在或 name 的值为空
	 *  注：如果 name 的值为空，则不能正确判断 name 是否存在
	 */
	const char* operator[](const char* name) const;

	/**
	 * URL 编码器对象的拷贝
	 * @param coder {const url_coder&} URL 源编码器对象
	 * @return {const url_coder&}
	 */
	const url_coder& operator =(const url_coder& coder);

	/**
	 * 获得参数数组对象
	 * @return {std::vector<URL_NV*>&}
	 */
	const std::vector<URL_NV*>& get_params(void) const
	{
		return params_;
	}

	/**
	 * 从 params_ 参数数组中删除某个变量
	 * @param name {const char*} 变量名
	 * @return {bool} 返回 true 表示删除成功，否则表示不存在
	 */
	bool del(const char* name);

	/**
	 * 重置解析器状态，清除内部缓存
	 */
	void reset(void);

private:
	bool nocase_;
	dbuf_guard* dbuf_;
	dbuf_guard* dbuf_internal_;
	std::vector<URL_NV*> params_;
	string*  buf_;

	void init_dbuf(dbuf_guard* dbuf);
};

} // namespace acl end
