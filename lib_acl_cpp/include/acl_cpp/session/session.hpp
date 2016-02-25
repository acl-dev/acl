#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <map>
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl_cpp/stdlib/string.hpp"

namespace acl {

// 用来存储属性值的缓冲区对象定义，这主要是为了兼容属性值
// 可以为二进制的情形而增加的结构类型
typedef enum
{
	TODO_NUL,
	TODO_SET,
	TODO_DEL
} todo_t;

class ACL_CPP_API session_string : public string
{
public:
	session_string(size_t n = 64) : string(n), todo_(TODO_NUL) {}
	session_string(const session_string& ss)
		: string(ss)
	{
		todo_ = ss.todo_;
	}
	session_string(const string& s) : string(s), todo_(TODO_NUL) {}
	session_string(const char* s) : string(s), todo_(TODO_NUL) {}
	~session_string() {}
	todo_t todo_;
};

/**
 * session 类，该类使用 memcached 存储 session 数据
 */
class ACL_CPP_API session : public dbuf_obj
{
public:
	/**
	 * 当构造函数的参数 sid 非空时，则该 session 对象使用该
	 * sid；否则内部会自动生成一个 sid，用户应该通过 get_sid()
	 * 获得这个自动生成的 sid 以便于每次查询该 sid 对应的数据
	 * @param ttl {time_t} 指定 session 的生存周期(秒)
	 * @param sid {const char*} 非空时，则 session 的 sid 使
	 *  该值，否则内部会产生一个随机的 session sid，该随机的
	 *  sid 可以通过调用 get_sid() 获得；当然在使用过程中，用户
	 *  也可以通过 set_sid() 修改本对象的 session sid；
	 *  此外，如果该 sid 为空，则如果用户查查找某个 sid 对应的
	 *  数据，则用户必须先调用 set_sid()
	 */
	session(time_t ttl = 0, const char* sid = NULL);
	virtual ~session(void);
	
	/**
	 * 重置内部状态，清理掉一些临时数据
	 */
	void reset(void);

	/**
	 * 获得本 session 对象的唯一 ID 标识
	 * @return {const char*} 非空
	 */
	virtual const char* get_sid(void) const
	{
		return sid_.c_str();
	}

	/**
	 * 设置本 session 对象的唯一 ID 标识
	 * @param sid {const char*} 非空
	 * 注：调用本函数后，会自动清除之前的中间缓存数据
	 */
	void set_sid(const char* sid);

	/**
	 * 当调用 session 类的 set/set_ttl 时，如果最后一个参数 delay 为 true，
	 * 则必须通过调用本函数将数据真正进行更新
	 * @return {bool} 数据更新是否成功
	 */
	virtual bool flush();

	/**
	 * 向 session 中添加新的字符串属性，同时设置该
	 * session 的过期时间间隔(秒)
	 * @param name {const char*} session 名，非空
	 * @param value {const char*} session 值，非空
	 * @return {bool} 返回 false 表示出错
	 */
	virtual bool set(const char* name, const char* value);

	/**
	 * 向 session 中添加新的属性对象并设置该 session 的过期时间间隔(秒)，
	 * @param name {const char*} session 属性名，非空
	 * @param value {const char*} session 属性值，非空
	 * @param len {size_t} value 值长度
	 * @return {bool} 返回 false 表示出错
	 */
	virtual bool set(const char* name, const void* value, size_t len);

	/**
	 * 延迟向 session 中添加新的属性对象并设置该 session 的过期时间间隔(秒)，
	 * 当用户调用了 session::flush 后再进行数据更新， 这样可以提高传输效率
	 * @param name {const char*} session 属性名，非空
	 * @param value {const char*} session 属性值，非空
	 * @param len {size_t} value 值长度
	 * @return {bool} 返回 false 表示出错
	 */
	virtual bool set_delay(const char* name, const void* value, size_t len);
	
	/**
	 * 从 session 中取得字符串类型属性值
	 * @param name {const char*} session 属性名，非空
	 * @return {const char*} session 属性值，返回的指针地址永远非空，用户
	 *  可以通过判断返回的是否是空串(即: "\0")来判断出错或不存在
	 *  注：该函数返回非空数据后，用户应该立刻保留此返回值，因为下次
	 *      的其它函数调用可能会清除该临时返回数据
	 */
	const char* get(const char* name);

	/**
	 * 从 session 中取得二进制数据类型的属性值
	 * @param name {const char*} session 属性名，非空
	 * @return {const session_string*} session 属性值，返回空时
	 *  表示出错或不存在
	 *  注：该函数返回非空数据后，用户应该立刻保留此返回值，因为下次
	 *      的其它函数调用可能会清除该临时返回数据
	 */
	virtual const session_string* get_buf(const char* name);

	/**
	 * 从 session 中删除指定属性值，当所有的变量都删除
	 * 时会将整个对象从 memcached 中删除
	 * @param name {const char*} session 属性名，非空
	 * @return {bool} true 表示成功(含不存在情况)，false 表示删除失败
	 *  注：当采用延迟方式删除某个属性时，则延迟发送更新指令到后端的
	 *  缓存服务器，当用户调用了 session::flush 后再进行数据更新，这
	 *  样可以提高传输效率；否则，则立刻更新数据
	 */
	virtual bool del_delay(const char* name);
	virtual bool del(const char* name);

	/**
	 * 重新设置 session 在缓存服务器上的缓存时间
	 * @param ttl {time_t} 生存周期(秒)
	 * @param delay {bool} 当为 true 时，则延迟发送更新指令到后端的
	 *  缓存服务器，当用户调用了 session::flush 后再进行数据更新，这
	 *  样可以提高传输效率；当为 false 时，则立刻更新数据
	 * @return {bool} 设置是否成功
	 */
	bool set_ttl(time_t ttl, bool delay);

	/**
	 * 获得本 session 对象中记录的 session 生存周期；该值有可能
	 * 与真正存储在缓存服务器的时间不一致，因为有可能其它的实例
	 * 重新设置了 session 在缓存服务器上的生存周期
	 * @return {time_t}
	 */
	time_t get_ttl(void) const
	{
		return ttl_;
	}

	/**
	 * 使 session 从服务端的缓存中删除即使 session 失效
	 * @return {bool} 是否使 session 失效
	 */
	virtual bool remove(void) = 0;

	/**
	 * 从后端缓存中获得对应 sid 的属性对象集合
	 * @param attrs {std::map<string, session_string>&}
	 * @return {bool}
	 */
	virtual bool get_attrs(std::map<string, session_string>& attrs) = 0;

	/**
	 * 从后端缓存中获得对应 sid 的指定属性集合
	 * @param names {const std::vector<string>&} 属性名集合
	 * @param values {std::vector<session_string>&} 存储对应的属性值结果集
	 * @return {bool} 操作是否成功
	 */
	virtual bool get_attrs(const std::vector<string>& names,
		std::vector<session_string>& values);

	/**
	 * 向后端缓存写入对应 sid 的属性对象集合
	 * @param attrs {std::map<string, session_string>&}
	 * @return {bool}
	 */
	virtual bool set_attrs(const std::map<string, session_string>& attrs) = 0;

protected:
	// 设置对应 sid 数据的过期时间
	virtual bool set_timeout(time_t ttl) = 0;

protected:
	// 将 session 数据序列化
	static void serialize(const std::map<string, session_string>& attrs,
		string& out);

	// 将 session 数据反序列化
	static void deserialize(string& buf,
		std::map<string, session_string>& attrs);

	// 清空 session 属性集合
	static void attrs_clear(std::map<string, session_string>& attrs);

protected:
	session_string sid_;
	time_t ttl_;

	// 该变量主要用在 set_ttl 函数中，如果推测该 sid_ 只是新产生的
	// 且还没有在后端 cache 服务端存储，则 set_ttl 不会立即更新后端
	// 的 cache 服务器
	bool sid_saved_;
	bool dirty_;
	std::map<string, session_string> attrs_;
	std::map<string, session_string> attrs_cache_;
};

} // namespace acl
