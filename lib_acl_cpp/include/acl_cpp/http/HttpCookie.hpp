#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <list>
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl_cpp/http/http_type.hpp"

namespace acl {

/**
 * http 协议头中 cookie 对象类
 */
class ACL_CPP_API HttpCookie : public dbuf_obj
{
public:
	/**
	 * 构造函数
	 * @param name {const char*} cookie 名，为非空字符串且字符串长度 > 0
	 * @param value {const char*} cookie 值，指针非空，字符串长度可以为 0
	 * 注：如果输入的两个参数不符合条件，内部将会产生断言
	 * @param dbuf {dbuf_guard*} 非空时将做为内存分配池
	 */
	HttpCookie(const char* name, const char* value, dbuf_guard* dbuf = NULL);

	/**
	 * 当使用该构造函数时，可以使用 setCookie 来添加 cookie 项
	 * @param dbuf {dbuf_guard*} 非空时将做为内存分配池
	 */
	HttpCookie(dbuf_guard* dbuf = NULL);

	/**
	 * 拷贝构造函数
	 * @param cookie {const HttpCookie*} 非 NULL， 内部将复制拷贝其成员变量
	 * @param dbuf {dbuf_guard*} 非空时将做为内存分配池
	 */
	HttpCookie(const HttpCookie* cookie, dbuf_guard* dbuf = NULL);

	/**
	 * 析构函数
	 */
	~HttpCookie(void);

	/**
	 * 对于 Set-Cookie: xxx=xxx; domain=xxx; expires=xxx; path=xxx; max-age=xxx; ...
	 * 类的数据进行分析
	 * @param value {const char*} 类似于 xxx=xxx; domain=xxx; ... 内容
	 * @return {bool} 传入的数据是否合法
	 */
	bool setCookie(const char* value);

	/**
	 * 动态创建的类对象通过此函数释放
	 */
	void destroy();

	/**
	 * 设置 cookie 的作用域
	 * @param domain {const char*} cookie 作用域
	 * @return {HttpCookie&} 返回本对象的引用，便于用户连续操作
	 */
	HttpCookie& setDomain(const char* domain);

	/**
	 * 设置 cookie 的 path 字段
	 * @param path {const char*} path 字段值
	 * @return {HttpCookie&} 返回本对象的引用，便于用户连续操作
	 */
	HttpCookie& setPath(const char* path);

	/**
	 * 设置 cookie 的过期时间段，即用当前时间加输入的时间即为 cookie
	 * 的过期时间
	 * @param timeout {time_t} 过期时间值(单位为秒)，当前时间加该时间
	 * 即 cookie 的过期时间
	 * @return {HttpCookie&} 返回本对象的引用，便于用户连续操作
	 */
	HttpCookie& setExpires(time_t timeout);

	/**
	 * 设置 cookie 的过期时间截字符串
	 * @param expires {const char*} 过期时间截
	 * @return {HttpCookie&} 返回本对象的引用，便于用户连续操作
	 */
	HttpCookie& setExpires(const char* expires);

	/**
	 * 设置 cookie 的生存周期
	 * @param max_age {int} 生存秒数
	 * @return {HttpCookie&} 返回本对象的引用，便于用户连续操作
	 */
	HttpCookie& setMaxAge(int max_age);

	/**
	 * 添加与该 cookie 对象其它属性值
	 * @param name {const char*} 属性名
	 * @param value {const char*} 属性值
	 * @return {HttpCookie&} 返回本对象的引用，便于用户连续操作
	 */
	HttpCookie& add(const char* name, const char* value);

	/**
	 * 获得 cookie 名称，取决于构建函数输入值
	 * @return {const char*} 为长度大于 0 的字符串，永远非空指针
	 * 注：用户必须在调用 HttpCookie(const char*, const char*) 构造
	 *     或调用 setCookie(const char*) 成功后才可以调用该函数，
	 *     否则返回的数据是 "\0"
	 */
	const char* getName(void) const;

	/**
	 * 获得 cookie 值，取决于构造函数输入值
	 * @return {const char*} 非空指针，有可能是空字符串("\0")
	 */
	const char* getValue(void) const;

	/**
	 * 获得字符串格式的过期时间
	 * @return {const char*} 非空指针，返回值为 "\0" 表示不存在
	 */
	const char* getExpires(void) const;

	/**
	 * 获得 cookie 作用域
	 * @return {const char*} 非空指针，返回值为 "\0" 表示不存在
	 */
	const char* getDomain(void) const;

	/**
	 * 获得 cookie 的存储路径
	 * @return {const char*} 非空指针，返回值为 "\0" 表示不存在
	 */
	const char* getPath(void) const;

	/**
	 * 获得 cookie 的生存周期
	 * @return {int} 返回 -1 时表示没有该 Max-Age 字段
	 */
	int  getMaxAge(void) const;

	/**
	 * 获得对应参数名的参数值
	 * @param name {const char*} 参数名
	 * @param case_insensitive {bool} 是否区分大小写，true 表示
	 *  不区分大小写
	 * @return {const char*} 非空指针，返回值为 "\0" 表示不存在
	 */
	const char* getParam(const char* name,
		bool case_insensitive = true) const;

	/**
	 * 获得该 cookie 对象的除 cookie 名及 cookie 值之外的
	 * 所有属性及属性值
	 * @return {const std::list<HTTP_PARAM*>&}
	 */
	const std::list<HTTP_PARAM*>& getParams(void) const;

private:
	dbuf_guard* dbuf_internal_;
	dbuf_guard* dbuf_;
	char  dummy_[1];
	char* name_;
	char* value_;
	std::list<HTTP_PARAM*> params_;

	bool splitNameValue(char* data, HTTP_PARAM* param);

protected:
//	HttpCookie(HttpCookie&) {}
//	HttpCookie(const HttpCookie&) {}
};

} // namespace acl end
