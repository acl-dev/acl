#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include "../connpool/connect_client.hpp"
#include "http_header.hpp"

namespace acl {

class http_client;
class http_pipe;
class socket_stream;
class charset_conv;
class sslbase_conf;
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
	 * @param client {socket_stream*} HTTP 连接流对象，可以是请求端的流，
	 *  也可以是响应端的流；当本对象被销毁时，client 对象是否会被自动销毁，
	 *  取决于参数 stream_fixed 的值
	 * @param conn_timeout {int} 如果传入的流关闭，则内部会
	 *  自动重试，此时需要该值表示连接服务器的超时时间(秒)，
	 *  至于重连流的 IO 读写超时时间是从 输入的流中继承的
	 * @param unzip {bool} 是否对服务器响应的数据自动进行解压
	 * 注：当该类实例被多次使用时，用户应该在每次调用前调用
	 * request_header::http_header::reset()
	 * @param stream_fixed {bool} 当该值为 true 时，则当 http_client 对象
	 *  被销毁时，传入的 client 流对象不会被销毁，需应用自行销毁；如果该
	 *  值为 false 时，则当本对象销毁时，client 流对象也将被销毁
	 */
	http_request(socket_stream* client, int conn_timeout = 60,
		bool unzip = true, bool stream_fixed = true);

	/**
	 * 构造函数：该构造函数内部创建的 socket_stream 流会自行关闭
	 * @param addr {const char*} WEB 服务器地址，地址格式：domain|port，
	 *  如：www.baidu.com|80
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
	 * 设置客户端 SSL 通信方式，内部缺省为非 SSL 通信方式
	 * @param conf {sslbase_conf*} 客户端 SSL 配置对象
	 * @return {http_request&}
	 */
	http_request& set_ssl(sslbase_conf* conf);

	/**
	 * 获得 HTTP 请求头对象，然后在返回的 HTTP 请求头对象中添加
	 * 自己的请求头字段或 http_header::reset()重置请求头状态，
	 * 参考：http_header 类
	 * @return {http_header&}
	 */
	http_header& request_header(void);

	/**
	 * 设置本地字符集，当本地字符集非空时，则边接收数据边进行字符集转换
	 * @param local_charset {const char*} 本地字符集
	 * @return {http_header&}
	 */
	http_request& set_local_charset(const char* local_charset);

	/**
	 * 向 HTTP 服务器发送 HTTP 请求头及 HTTP 请求体，同时从
	 * HTTP 服务器读取 HTTP 响应头，对于长连接，当连接中断时
	 * 会再重试一次，在调用下面的几个 get_body 函数前必须先
	 * 调用本函数(或调用 write_head/write_body)；
	 * 正常情况下，该函数在发送完请求数据后会读 HTTP 响应头，
	 * 所以用户在本函数返回 true 后可以调用：get_body() 或
	 * http_request::get_clinet()->read_body(char*, size_t)
	 * 继续读 HTTP 响应的数据体
	 * @param data {const void*} 发送的数据体地址，非空时自动按
	 *  POST 方法发送，否则按 GET 方法发送
	 * @param len {size_} data 非空时指定 data 数据长度
	 * @return {bool} 发送请求数据及读 HTTP 响应头数据是否成功
	 */
	bool request(const void* data, size_t len);

	/**
	 * 当采用流式写数据时，需要首先调用本函数发送 HTTP 请求头
	 * @return {bool} 是否成功，如果成功才可以继续调用 write_body
	 */
	bool write_head();

	/**
	 * 当采用流式写数据时，在调用 write_head 后，可以循环调用本函数
	 * 发送 HTTP 请求体数据；当输入的两个参数为空值时则表示数据写完；
	 * 当发送完数据后，该函数内部会自动读取 HTTP 响应头数据，用户可
	 * 继续调用 get_body/read_body 获取 HTTP 响应体数据
	 * @param data {const void*} 数据地址指针，当该值为空指针时表示
	 *  数据发送完毕
	 * @param len {size_t} data 非空指针时表示数据长度
	 * @return {bool} 发送数据体是否成功
	 *  注：当应用发送完数据后，必须再调用一次本函数，同时将两个参数都赋空
	 */
	bool write_body(const void* data, size_t len);

	/////////////////////////////////////////////////////////////////////

	/**
	 * 当发送完请求数据后，内部会自动调用读 HTTP 响应头过程，可以通过此函数
	 * 获得服务端响应的 HTTP 状态字(2xx, 3xx, 4xx, 5xx)；
	 * 其实该函数内部只是调用了 http_client::response_status 方法
	 * @return {int}
	 */
	int http_status() const;

	/**
	 * 获得 HTTP 响应的数据体长度
	 * @return {int64) 返回值若为 -1 则表明 HTTP 头不存在或没有长度字段
	 */
#if defined(_WIN32) || defined(_WIN64)
	__int64 body_length(void) const;
#else
	long long int body_length(void) const;
#endif
	/**
	 * HTTP 数据流(响应流是否允许保持长连接)
	 * @return {bool}
	 */
	bool keep_alive(void) const;

	/**
	 * 获得 HTTP 响应头中某个字段名的字段值
	 * @param name {const char*} 字段名
	 * @return {const char*} 字段值，为空时表示不存在
	 */
	const char* header_value(const char* name) const;

	/**
	 * 是否读完了数据体
	 * @return {bool}
	 */
	bool body_finish() const;

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

	/**
	 * 当调用 request 成功后调用本函数，读取服务器全部响应数据
	 * 存储于输入的缓冲区中
	 * @param out {string&} 存储响应数据体
	 * @param to_charset {const char*} 当该项非空，内部自动
	 *  将数据转成该字符集存储于 out 对象中
	 * 注：当响应数据体特别大时不应用此函数，以免内存耗光
	 */
	bool get_body(string& out, const char* to_charset = NULL);

	/**
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
	int read_body(char* buf, size_t size);

	/**
	 * 当调用 request 成功后调用本函数读 HTTP 响应数据体，可以循环调用
	 * 本函数，本函数内部自动对压缩数据进行解压，如果在调用本函数之前调用
	 * set_charset 设置了本地字符集，则还同时对数据进行字符集转码操作
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
	 *  当返回 0 时，可调用 body_finish 函数判断是否读完了所有数据体
	 */
	int read_body(string& out, bool clean = false, int* real_size = NULL);

	/**
	 * 调用 request 成功后调用本函数来从 HTTP 服务端读一行数据，可循环调用
	 * 本函数，直到返回 false 或 body_finish() 返回 true 为止；
	 * 内部自动对压缩数据解压，如果在调用本函数之前调用 set_charset 设置了
	 * 本地字符集，则还同时对数据进行字符集转码操作
	 * @param out {string&} 存储结果数据
	 * @param nonl {bool} 读到的一行数据是否自动去掉尾部的 "\r\n" 或 "\n"
	 * @param size {size_t*} 该指针非空时存放读到的数据长度
	 * @return {bool} 是否读到一行数据：返回 true 时表示读到了一行数据，
	 *  可以通过 body_finish() 是否为 true 来判断是否读数据体已经结束，
	 *  当读到一个空行 且 nonl = true 时，则 *size = 0；当返回 false 时
	 *  表示未读完整行且读完毕，
	 *  *size 中存放着读到的数据长度
	 */
	bool body_gets(string& out, bool nonl = true, size_t* size = NULL);

	/**
	 * 当通过 http_request::request_header().set_range() 设置了
	 * range 的请求时，此函数检查服务器返回的数据是否支持 range
	 * @return {bool}
	 */
	bool support_range(void) const;

#if defined(_WIN32) || defined(_WIN64)
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

	/////////////////////////////////////////////////////////////////////

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
	 * 基类 connect_client 纯虚函数，显式调用本函数用来打开与服务端的连接
	 * @return {bool} 连接是否成功
	 */
	virtual bool open();

private:
	char addr_[64];
	bool unzip_;
	sslbase_conf* ssl_conf_;
	char local_charset_[64];
	charset_conv* conv_;
        http_client* client_;
	http_header  header_;
	bool cookie_inited_;
	std::vector<HttpCookie*>* cookies_;
#if defined(_WIN32) || defined(_WIN64)
	__int64 range_from_;
	__int64 range_to_;
	__int64 range_max_;
#else
	long long int range_from_;
	long long int range_to_;
	long long int range_max_;
#endif
	// 在写 HTTP 请求数据体时，该标志位标识是否允许重试过
	bool need_retry_;

	bool send_request(const void* data, size_t len);
	bool try_open(bool* reuse_conn);
	void close(void);
	void create_cookies(void);
	http_pipe* get_pipe(const char* to_charset);
	void set_charset_conv();
	void check_range(void);
};

} // namespace acl
