#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/dbuf_pool.hpp"
#include "../session/session.hpp"
#include <map>

#ifndef ACL_CLIENT_ONLY

namespace acl {

class session;

/**
 * 服务端 HttpSession 类，目前该类的数据存储只能支持存在 memcached 上
 */
class ACL_CPP_API HttpSession : public dbuf_obj
{
public:
	/**
	 * 构造函数
	 * @param session {session&} 缓存对象
	 */
	HttpSession(session& session);
	virtual ~HttpSession(void);

	/**
	 * 获得客户端在服务端存储的 session 的字符串属性值
	 * @param name {const char*} session 属性名，非空
	 * @return {const char*} session 属性值，返回地址永远非空指针，用户
	 *  可以通过判断返回地址是否为空串("\0")来判断是否存在或出错
	 *  注：该函数返回非空数据后，用户应该立刻保留此返回值，因为下次
	 *      的其它函数调用可能会清除该临时返回数据
	 */
	virtual const char* getAttribute(const char* name) const;

	/**
	 * 获得客户端在服务端存储的 session 的二进制属性值
	 * @param name {const char*} session 属性名，非空
	 * @param size {size_t*} 该参数非空且属性值非空时，该指针地址
	 *  存储返回属性值的大小
	 * @return {const void*} session 属性值，为空指针时说明不存在
	 *  或内部查询失败
	 *  注：该函数返回非空数据后，用户应该立刻保留此返回值，因为下次
	 *      的其它函数调用可能会清除该临时返回数据
	 */
	virtual const void* getAttribute(const char* name, size_t* size) const;

	/**
	 * 从服务端获得对应客户端的所有会话属性对象，这样可以减少与服务端的交互次数
	 * @param attrs {std::map<string, session_string>&}
	 * @return {bool} 是否成功
	 */
	virtual bool getAttributes(std::map<string, session_string>& attrs) const;

	/**
	 * 从服务端获得对应客户端的相应属性集合
	 * @param names {const std::vector<string>&} 属性名集合
	 * @param values {std::vector<session_string>&} 存储对应的属性值结果集
	 * @return {bool} 是否成功
	 */
	virtual bool getAttributes(const std::vector<string>& names,
		std::vector<session_string>& values) const;

	/**
	 * 在设置服务端设置 session 的字符串属性值
	 * @param name {const char*} session 属性名，非空
	 * @param value {const char*} session 属性值，非空
	 * @return {bool} 返回 false 说明设置失败
	 */
	virtual bool setAttribute(const char* name, const char* value);

	/**
	 * 在设置服务端设置 session 的二进制属性值
	 * @param name {const char*} session 属性名，非空
	 * @param value {const void*} session 属性值，非空
	 * @param len {size_t} value 数据长度
	 * @return {bool} 返回 false 说明设置失败
	 */
	virtual bool setAttribute(const char* name, const void* value, size_t len);

	/**
	 * 在服务端设置 session 属性集合，这样可以减少与后端的交互次数
	 * @param attrs {const std::map<string, session_string>&} 属性集合对象
	 * @return {bool} 设置是否成功
	 */
	virtual bool setAttributes(const std::map<string, session_string>& attrs);

	/**
	 * 删除客户端 session 中的某个属性值
	 * @param name {const char*} session 属性名，非空
	 * @return {bool} 删除是否成功
	 */
	virtual bool removeAttribute(const char* name);

	/**
	 * 设置 session 在缓存服务器上的生存周期
	 * @param ttl {time_t} 生存周期(秒)
	 * @return {bool} 是否成功
	 */
	virtual bool setMaxAge(time_t ttl);

	/**
	 * 使 session 从服务端的缓存中删除即使 session 失效
	 * @return {bool} 是否使 session 失效
	 */
	virtual bool invalidate(void);

	/**
	 * 获得所产生的 session ID 标识
	 * @return {const char*} 永远返回以 '\0' 结尾的非空指针，可根据返回
	 *  值是否为空串("\0")来判断 sid 是否存在
	 */
	const char* getSid(void) const;

protected:
	session& session_;
};

} // namespace acl

#endif // ACL_CLIENT_ONLY
