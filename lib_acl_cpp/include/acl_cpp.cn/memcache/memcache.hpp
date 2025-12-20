#pragma once
#include "../acl_cpp_define.hpp"
#include <time.h>
#include "../connpool/connect_client.hpp"
#include "../stdlib/string.hpp"
#include "../mime/rfc2047.hpp"

#ifndef ACL_CLIENT_ONLY

namespace acl {

class socket_stream;

typedef class memcache mem_cache;

/**
 * memcached 客户端通信协议库，支持长连接与自动重连
 */
class ACL_CPP_API memcache : public connect_client
{
public:
	/**
	* 构造函数
	* @param addr {const char*} memcached 服务器监听地址，格式为：
	*  ip:port，如: 127.0.0.1:11211
	* @param conn_timeout {int} 连接服务器的超时时间(秒)
	* @param rw_timeout {int} 网络 IO 超时时间(秒)
	*/
	memcache(const char* addr = "127.0.0.1:11211", int conn_timeout = 30,
		int rw_timeout = 10);

	~memcache();

	/**
	 * 设置 key 的前缀，即实际的 key 将由 该前缀+原始key 组成，缺省时不设
	 * 前缀，当多个应用共用同一个 memcached 服务时，建议应用设置自身的
	 * key 前缀，这样可以避免与其它应用的 key 产生重复问题
	 * @param keypre {const char*} 非空时设置 key 前缀，否则取消 key 前缀
	 * @return {memcache&}
	 */
	memcache& set_prefix(const char* keypre);

	/**
	 * 在保持的长连接中断时是否要求自动重连，缺省为自动重连
	 * @param onoff {bool} 为 true 时表示长连接意外断开后自动重连
	 * @return {memcache&}
	 */
	memcache& auto_retry(bool onoff);

	/**
	 * 设置是否针对 KEY 键值进行编码，缺少时不对 key 编码，当应用的 key 中
	 * 可能会有特殊字符或二进制值时，建议调用此函数对 key 进行编码
	 * @param onoff {bool} 为 true 表示内部需要对 key 进行编码
	 * @return {memcache&}
	 */
	memcache& encode_key(bool onoff);

	/**
	* 向 memcached 中修改或添加新的数据缓存对象
	* @param key {const char*} 键值
	* @param klen {size_t} key 键值长度
	* @param dat {const void*} 数据
	* @param dlen {size_t} data 数据长度
	* @param timeout {time_t} 缓存超时时间(秒)
	* @param flags {unsigned short} 附属的标志位
	* @return {bool} 是否成功
	*/
	bool set(const char* key, size_t klen,
		const void* dat, size_t dlen,
		time_t timeout = 0, unsigned short flags = 0);

	/**
	* 向 memcached 中修改或添加新的数据缓存对象
	* @param key {const char*} 字符串键值
	* @param dat {const void*} 数据
	* @param dlen {size_t} data 数据长度
	* @param timeout {time_t} 缓存超时时间(秒)
	* @param flags {unsigned short} 附属的标志位
	* @return {bool} 是否成功
	*/
	bool set(const char* key, const void* dat, size_t dlen,
		time_t timeout = 0, unsigned short flags = 0);

	/**
	* 更新 memcached 中已经存在的键的过期日期，因为目前 libmemcached 没有
	* 提供此接口，所以该函数实现的方式是先调用 get 取出对应键的值，然后再
	* 调用 set 重新设置该键的值及过期时间
	* @param key {const char*} 键值
	* @param klen {size_t} key 键值长度
	* @param timeout {time_t} 过期时间(秒)
	* @return {bool} 是否成功
	*/
	bool set(const char* key, size_t klen, time_t timeout = 0);

	/**
	* 更新 memcached 中已经存在的键的过期日期，因为目前 libmemcached 没有
	* 提供此接口，所以该函数实现的方式是先调用 get 取出对应键的值，然后再
	* 调用 set 重新设置该键的值及过期时间
	* @param key {const char*} 字符串键值
	* @param timeout {time_t} 过期时间(秒)
	* @return {bool} 是否成功
	*/
	bool set(const char* key, time_t timeout = 0);

	/**
	 * 以流式方式上传大数据时，该函数发送数据头
	 * @param key {const char*} 键值字符串
	 * @param dlen {size_t} 数据体的数据总长度
	 * @param timeout {time_t} 数据的过期时间(秒)
	 * @param flags {unsigned short} 附属的标志位
	 * @return {bool} 是否成功
	 */
	bool set_begin(const char* key, size_t dlen,
		time_t timeout = 0, unsigned short flags = 0);

	/**
	 * 循环调用本函数上传数据值，内部会自动计算已经上传的数据总和是否达到
	 * 了 set_begin 中设置的数据总长度，当达到后会自动补一个 "\r\n"，调用
	 * 者不应再调用此函数上传数据，除非是一个新的上传过程开始了
	 * @param data {const void*} 数据地址指针
	 * @param len {data} data 数据长度
	 * @return {bool} 是否成功
	 */
	bool set_data(const void* data, size_t len);

	/**
	* 从 memcached 中获得对应键值的缓存数据
	* @param key {const char*} 字符串键值
	* @param klen {size_t} 键值长度
	* @param buf {string&} 存储结果的缓冲区，内部首先会清空该缓冲区
	* @param flags {unsigned short*} 存储附属的标志位
	* @return {bool} 返回 true 表示正确获得结果值，否则表示键值对应的
	*  数据不存在或出错
	*/
	bool get(const char* key, size_t klen, string& buf,
		unsigned short* flags = NULL);

	/**
	* 从 memcached 中获得对应键值的缓存数据
	* @param key {const char*} 字符串键值
	* @param buf {string&} 存储结果的缓冲区，内部首先会清空该缓冲区
	* @param flags {unsigned short*} 存储附属的标志位
	* @return {bool} 返回 true 表示正确获得结果值，否则表示键值对应的
	*  数据不存在或出错
	*/
	bool get(const char* key, string& buf, unsigned short* flags = NULL);

	/**
	 * 流式方式从服务端获取数据，本函数发送请求协议
	 * @param key {const void*} 键值
	 * @param klen {size_t} key 键值长度
	 * @param flags {unsigned short*} 存储附属的标志位
	 * @return {int} 返回数据体的长度，分以下三种情形：
	 *   0：表示不存在
	 *  -1：表示出错
	 *  >0：表示数据体的长度
	 */
	int get_begin(const void* key, size_t klen, unsigned short* flags = NULL);

	/**
	 * 流式方式从服务端获取数据，本函数发送请求协议
	 * @param key {const char*} 键值字符串
	 * @param flags {unsigned short*} 存储附属的标志位
	 * @return {int} 返回数据体的长度，分以下三种情形：
	 *   0：表示不存在
	 *  -1：表示出错
	 *  >0：表示数据体的长度
	 */
	int get_begin(const char* key, unsigned short* flags = NULL);

	/**
	 * 流式方式从服务端获取数据，循环调用本函数接收数据
	 * @param buf {void*} 缓冲区地址
	 * @param size {size_t} 缓冲区大小
	 * @return {int} 已读到的数据大小，分为以下三种情形：
	 *  0：表示数据读完
	 *  > 0: 表示本次读到的数据长度
	 *  -1：表示出错
	 */
	int  get_data(void* buf, size_t size);

	/**
	* 从 memcached 中删除数据
	* @param key {const char*} 键值
	* @param klen {size_t} 键值长度
	* @return {bool} 删除是否成功
	*/
	bool del(const char* key, size_t klen);

	/**
	* 从 memcached 中删除数据
	* @param key {const char*} 字符串键值
	* @return {bool} 删除是否成功
	*/
	bool del(const char* key);

	/**
	* 获得上次操作 memcached 错误描述信息
	* @return {const char*} 错误描述信息，永不为空
	*/
	const char* last_serror() const;

	/**
	* 获得上次操作 memcached 的错误号
	* @return {int} 错误号
	*/
	int  last_error() const;

	/**
	* 打开与 memcached 的连接, 因为 set/get/del 操作都会自动打开与
	* memcached 的连接，所以不必显示地调用此函数来打开与 memcached
	* 的连接
	* @return {bool} 打开是否成功
	*/
	virtual bool open();

	/**
	* 关闭与 memcached 的连接，一般该函数不需要调用，因为类对象在
	* 析构时会自动调用此函数
	*/
	void close();

	/**
	* 列出 memcached 连接的一些属性，调试用
	*/
	void property_list();

private:
	bool set(const string& key, const void* dat, size_t dlen,
		time_t timeout, unsigned short flags);
	bool get(const string& key, string& buf, unsigned short* flags);
	const string& build_key(const char* key, size_t klen);

	string* keypre_;         // 非空时，该字符串被添加在 KEY 值前组成新的 KEY
	rfc2047 coder_;          // 当需要对 KEY 编码时的编码器
	bool  encode_key_;       // 是否需要对 KEY 进行编码

	bool  opened_;           // 连接是否打开
	bool  retry_;            // 是否支持连接中断重试
	char* addr_;             // 服务地址(ip:port)
	int   enum_;             // 出错号，留作将来扩充用
	string ebuf_;            // 存储出错信息
	string kbuf_;            // 存储经转码后的 KEY 值缓冲区

	size_t content_length_;  // 当采用流式上传/下载大数据时此值记录数据体的总长度
	size_t length_;          // 已经上传/下载的数据总和

	socket_stream* conn_;    // 与后端服务的连接对象
	string req_line_;        // 存储请求数据
	string res_line_;        // 存储响应数据
	bool error_happen(const char* line);
};

} // namespace acl

#endif // ACL_CLIENT_ONLY
