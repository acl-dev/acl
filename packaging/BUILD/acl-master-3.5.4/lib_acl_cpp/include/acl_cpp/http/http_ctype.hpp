#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"
#include "../stdlib/noncopyable.hpp"

#if !defined(ACL_MIME_DISABLE)

namespace acl {

/**
 * 与 HTTP 头中的 Content-Type 相关的类定义，可以分析如下数据：
 * Content-Type: application/x-www-form-urlencoded
 * Content-Type: multipart/form-data; boundary=xxx
 * Content-Type: application/octet-stream
 * Content-Type: text/html; charset=utf8
 * Content-Type: xxx/xxx; name=xxx
 * ...
 */
class ACL_CPP_API http_ctype : public noncopyable
{
public:
	http_ctype(void);
	~http_ctype(void);

	/**
	 * 重载了 "=" 操作符进行对象的复制
	 * @param ctype {const http_ctype&} 源对象
	 * @return {http_ctype&}
	 */
	http_ctype& operator=(const http_ctype& ctype);

	/**
	 * 分析 HTTP 头中 Content-Type 字段值
	 * @param cp {const char*} Content-Type 字段值，如：
	 * application/x-www-form-urlencoded
	 * multipart/form-data; boundary=xxx
	 * application/octet-stream
	 * @return {bool} 输入数据是否合法
	 */
	bool parse(const char* cp);

	/**
	 * 获得 Content-Type 字段值 text/html; charset=utf8 中的 text
	 * @return {const char*} 返回 NULL 说明没有该数据，一般是因为
	 *  parse 失败导致的
	 */
	const char* get_ctype(void) const;

	/**
	 * 获得 Content-Type 字段值 text/html; charset=utf8 中的 html
	 * @return {const char*} 返回 NULL 说明没有该数据
	 */
	const char* get_stype(void) const;

	/**
	 * 获得 Content-Type 字段值 multipart/form-data; boundary=xxx
	 * 中的 boundary 的值 xxx
	 * @return {const char*} 返回 NULL 说明没有该数据
	 */
	const char* get_bound(void) const;

	/**
	 * 获得 Content-Type: xxx/xxx; name=name_xxx
	 * 中的 name 的值 name_xxx
	 * @return {const char*} 返回 NULL 说明没有该数据
	 */
	const char* get_name(void) const;

	/**
	 * 获得 Content-Type 字段值 text/html; charset=utf8 中的 utf8
	 * @return {const char*} 返回 NULL 说明没有该数据
	 */
	const char* get_charset(void) const;

private:
	char* ctype_;
	char* stype_;
	char* name_;
	char* charset_;
	string* bound_;

	void reset(void);
};

} // namespace acl

#endif // !defined(ACL_MIME_DISABLE)
