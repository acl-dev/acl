#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <vector>
#include "acl_cpp/http/http_header.hpp"
#include "acl_cpp/connpool/connect_client.hpp"

namespace acl {

class http_client;
class http_pipe;
class socket_stream;
class charset_conv;
class xml;
class json;

/**
 * HTTP 客户端请求类，该类对象支持长连接，同时当连接断时会自动重试
 */
class ACL_CPP_API http_request : public connect_client
{
public:
	/**
	 * 构造函数：通过该构造函数传入的 socket_stream 流对象并
	 * 不会被关闭，需要调用者自己关闭
	 * @param client {socket_stream*} 数据连接流，非空，
	 *  在本类对象被销毁时该流对象并不会被销毁，所以用户需自行释放
	 * @param conn_timeout {int} 如果传入的流关闭，则内部会
	 *  自动重试，此时需要该值表示连接服务器的超时时间(秒)，
	 *  至于重连流的 IO 读写超时时间是从 输入的流中继承的
	 * @param unzip {bool} 是否对服务器响应的数据自动进行解压
	 * 注：当该类实例被多次使用时，用户应该在每次调用前调用
	 * request_header::http_header::reset()
	 */
	http_request(socket_stream* client, int conn_timeout = 60,
		bool unzip = true);

	/**
	 * 构造函数：该构造函数内部创建的 socket_stream 流会自行关闭
	 * @param addr {const char*} WEB 服务器地址
	 * @param conn_timeout {int} 远程连接服务器超时时间(秒)
	 * @param rw_timeout {int} IO 读写超时时间(秒)
	 * @param unzip {bool} 是否对服务器响应的数据自动进行解压
	 */
	http_request(const char* addr, int conn_timeout = 60,
		int rw_timeout = 60, bool unzip = true);

	virtual ~http_request(void);

	/**
	 * 设置在读取服务响应数据时是否针对压缩数据进行解压
	 * @param on {bool}
	 * @return {http_request&}
	 */
	http_request& set_unzip(bool on);

	/**
	 * 获得 HTTP 请求头对象，然后在返回的 HTTP 请求头对象中添加
	 * 自己的请求头字段或 http_header::reset()重置请求头状态，
	 * 参考：http_header 类
	 * @return {http_header&}
	 */
	http_header& request_header(void);

	/**
	 * 向 HTTP 服务器发送 HTTP 请求头及 HTTP 请求体，同时从
	 * HTTP 服务器读取 HTTP 响应头，对于长连接，当连接中断时
	 * 会再重试一次，在调用下面的几个 get_body 函数前必须先
	 * 调用本函数；
	 * 正常情况下，该函数在发送完请求数据后会读 HTTP 响应头，
	 * 所以用户在本函数返回 true 后可以调用：get_body() 或
	 * http_request::get_clinet()->read_body(char*, size_t)
	 * 继续读 HTTP 响应的数据体
	 * @param data {const char*} 发送的数据体地址，非空时自动按
	 *  POST 方法发送，否则按 GET 方法发送
	 * @param len {size_} data 非空时指定 data 数据长度
	 * @return {bool} 发送请求数据及读 HTTP 响应头数据是否成功
	 */
	bool request(const char* data, size_t len);

	/**
	 * 当调用 request 成功后调用本函数，读取服务器响应体数据
	 * 并将结果存储于规定的 xml 对象中
	 * @param out {xml&} HTTP 响应体数据存储于该 xml 对象中
	 * @param to_charset {const char*} 当该项非空，内部自动
	 *  将数据转成该字符集存储于 xml 对象中
	 * @return {bool} 读数据是否成功
	 * 注：当响应数据体特别大时不应用此函数，以免内存耗光
	 */
	bool get_body(xml& out, const char* to_charset = NULL);

	/**
	 * 当调用 request 成功后调用本函数，读取服务器响应体数据
	 * 并将结果存储于规定的 json 对象中
	 * @param out {json&} HTTP 响应体数据存储于该 json 对象中
	 * @param to_charset {const char*} 当该项非空，内部自动
	 *  将数据转成该字符集存储于 json 对象中
	 * @return {bool} 读数据是否成功
	 * 注：当响应数据体特别大时不应用此函数，以免内存耗光
	 */
	bool get_body(json& out, const char* to_charset = NULL);

	/*
	 * 当调用 request 成功后调用本函数，读取服务器全部响应数据
	 * 存储于输入的缓冲区中
	 * @param out {string&} 存储响应数据体
	 * @param to_charset {const char*} 当该项非空，内部自动
	 *  将数据转成该字符集存储于 out 对象中
	 * 注：当响应数据体特别大时不应用此函数，以免内存耗光
	 */
	bool get_body(string& out, const char* to_charset = NULL);

	/*
	 * 当调用 request 成功后调用本函数，读取服务器响应数据并
	 * 存储于输入的缓冲区中，可以循环调用本函数，直至数据读完了，
	 * @param buf {char*} 存储部分响应数据体
	 * @param size {size_t} buf 缓冲区大小
	 * @return {int} 返回值 == 0 表示正常读完毕，< 0 表示服务器
	 *  关闭连接，> 0 表示已经读到的数据，用户应该一直读数据直到
	 *  返回值 <= 0 为止
	 *  注：该函数读到的是原始 HTTP 数据体数据，不做解压和字符集
	 *  解码，用户自己根据需要进行处理
	 */
	int get_body(char* buf, size_t size);

	/**
	 * 在调用 read_body 之前，通过此函数设置本地字符集，从而使数据
	 * 边接收边转换
	 * @param to_charset {const char*} 本地字符集
	 */
	void set_charset(const char* to_charset);

	/**
	 * 当调用 request 成功后调用本函数读 HTTP 响应数据体，可以循环调用
	 * 本函数，本函数内部自动对压缩数据进行解压，如果在调用本函数之前
	 * 调用 set_charset 设置了本地字符集，则还同时对数据进行字符集转码
	 * 操作
	 * @param out {string&} 存储结果数据
	 * @param clean {bool} 每次调用本函数时，是否要求先自动将缓冲区 out
	 *  的数据清空
	 * @param real_size {int*} 当该指针非空时，存储解压前读到的真正数据
	 *  长度，如果在构造函数中指定了非自动解压模式且读到的数据 > 0，则该
	 *  值存储的长度值应该与本函数返回值相同；当读出错或未读到任何数据时，
	 *  该值存储的长度值为 0
	 * @return {int} == 0 表示读完毕，可能连接并未关闭；>0 表示本次读操作
	 *  读到的数据长度(当为解压后的数据时，则表示为解压之后的数据长度，
	 *  与真实读到的数据不同，真实读到的数据长度应该通过参数 real_size 来
	 *  获得); < 0 表示数据流关闭，此时若 real_size 非空，则 real_size 存
	 *  储的值应该为 0
	 */
	int read_body(string& out, bool clean = false,
		int* real_size = NULL);

	/**
	 * 当通过 http_request::request_header().set_range() 设置了
	 * range 的请求时，此函数检查服务器返回的数据是否支持 range
	 * @return {bool}
	 */
	bool support_range(void) const;

#ifdef WIN32
	/**
	 * 当调用了 http_request::request_header().set_range() 且读取服务器
	 * 返回的数据头后，此函数用来获得支持分段功能的起始偏移位置
	 * @return {acl_int64} 若服务器不支持 range 方式，则返回值 < 0
	 */
	__int64 get_range_from(void) const;

	/**
	 * 当调用了 http_request::request_header().set_range() 且读取服务器
	 * 返回的数据头后，此函数用来获得支持分段功能结束偏移位置
	 * @return {acl_int64} 若服务器不支持 range 方式，则返回值 < 0
	 */
	__int64 get_range_to(void) const;

	/**
	 * 当调用了 http_request::request_header().set_range() 且读取服务器
	 * 返回的数据头后，此函数用来获得支持分段功能的整个数据体大小，该值
	 * 即代表 HTTP 响应数据体大小
	 * @return {acl_int64} 若服务器不支持 range 方式，则返回值 < 0
	 */
	__int64 get_range_max(void) const;
#else
	long long int get_range_from(void) const;
	long long int get_range_to(void) const;
	long long int get_range_max(void) const;
#endif

	/**
	 * 获得服务器返回的 Set-Cookie 的集合
	 * @return {const std::vector<HttpCookie*>*} 返回空表示
	 *  没有 cookie 对象或连接流为空
	 */
	const std::vector<HttpCookie*>* get_cookies(void) const;

	/**
	 * 获得服务器返回的 Set-Cookie 设置的某个 cookie 对象
	 * @param name {const char*} cookie 名
	 * @param case_insensitive {bool} 是否区分大小写，true 表示
	 *  不区分大小写
	 * @return {const HttpCookie*} 返回 NULL 表示不存在
	 */
	const HttpCookie* get_cookie(const char* name,
		bool case_insensitive = true) const;

	/**
	 * 获得 http_client HTTP 连接流，可以通过返回的对象获得
	 * 服务器响应的头部分数据，参考：http_client 类
	 * @return {http_client*} 当返回空时表示流出错了
	 */
	http_client* get_client(void) const;

	/**
	 * 重置请求状态，在同一个连接的多次请求时会调用此函数
	 */
	void reset(void);

protected:
	/**
	 * 基类 connect_client 的纯虚函数，显式地调用本函数用来打开与服务端的连接
	 * @reutrn {bool} 连接是否成功
	 */
	virtual bool open();

private:
	char addr_[64];
	bool keep_alive_;
	int  conn_timeout_;
	int  rw_timeout_;
	bool unzip_;
	charset_conv* conv_;
        http_client* client_;
	http_header  header_;
	bool cookie_inited_;
	std::vector<HttpCookie*>* cookies_;
#ifdef WIN32
	__int64 range_from_;
	__int64 range_to_;
	__int64 range_max_;
#else
	long long int range_from_;
	long long int range_to_;
	long long int range_max_;
#endif

	bool send_request(const char* data, size_t len);
	bool try_open(bool* reuse_conn);
	void close(void);
	void create_cookies(void);
	http_pipe* get_pipe(const char* to_charset);
	void check_range(void);
};

} // namespace acl
