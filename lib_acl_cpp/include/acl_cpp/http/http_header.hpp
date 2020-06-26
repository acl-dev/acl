#pragma once
#include "../acl_cpp_define.hpp"
#include <list>
#include "../stdlib/dbuf_pool.hpp"
#include "../http/http_type.hpp"

struct HTTP_HDR_RES;
struct HTTP_HDR_REQ;
struct HTTP_HDR_ENTRY;

namespace acl {

class string;
class HttpCookie;

/**
 * HTTP 头类，可以构建请求头或响应头
*/
class ACL_CPP_API http_header : public dbuf_obj
{
public:
	/**
	 * 构造函数
	 * @param dbuf {dbuf_guard*} 非空时将做为内存分配池
	 */
	http_header(dbuf_guard* dbuf = NULL);

	/**
	 * HTTP 请求头构造函数
	 * @param url {const char*} 请求的 URL，url 格式示例如下：
	 *   http://www.test.com/
	 *   /cgi-bin/test.cgi
	 *   http://www.test.com/cgi-bin/test.cgi
	 *   http://www.test.com/cgi-bin/test.cgi?name=value
	 *   /cgi-bin/test.cgi?name=value
	 * 如果该 url 中有主机字段，则内部自动添加主机；
	 * 如果该 url 中有参数字段，则内部自动进行处理并调用 add_param 方法；
	 * 调用该函数后用户仍可以调用 add_param 等函数添加其它参数；
	 * 当参数字段只有参数名没有参数值时，该参数将会被忽略，所以如果想
	 * 单独添加参数名，应该调用 add_param 方法来添加
	 * @param dbuf {dbuf_guard*} 非空时将做为内存分配池
	 */
	http_header(const char* url, dbuf_guard* dbuf = NULL);

	/**
	 * HTTP 响应头构造函数
	 * @param status {int} 状态字如：1xx, 2xx, 3xx, 4xx, 5xx
	 * @param dbuf {dbuf_guard*} 非空时将做为内存分配池
	 */
	http_header(int status, dbuf_guard* dbuf = NULL);

	/**
	 * 根据 C语言 的 HTTP 响应头进行构造
	 * @param hdr_res {const HTTP_HDR_RES&}
	 * @param dbuf {dbuf_guard*} 非空时将做为内存分配池
	 */
	http_header(const HTTP_HDR_RES& hdr_res, dbuf_guard* dbuf = NULL);

	/**
	 * 根据 C语言 的 HTTP 请求头进行构造
	 * @param hdr_req {const HTTP_HDR_REQ&}
	 * @param dbuf {dbuf_guard*} 非空时将做为内存分配池
	 */
	http_header(const HTTP_HDR_REQ& hdr_req, dbuf_guard* dbuf = NULL);

	virtual ~http_header(void);

	/**
	 * 重置 HTTP 头信息同时将上次的临时资源释放
	 */
	void reset(void);

	//////////////////////////////////////////////////////////////////////
	//            HTTP 请求与 HTTP 响应通用的方法函数
	//////////////////////////////////////////////////////////////////////

	/**
	 * 设置 HTTP 头是客户端的请求头还是服务器的响应头
	 * @param onoff {bool} true 表示是请求头，否则表示响应头
	 * @return {http_header&} 返回本对象的引用，便于用户连续操作
	 */
	http_header& set_request_mode(bool onoff);

	/**
	 * 向 HTTP 头中添加字段
	 * @param name {const char*} 字段名，非空指针
	 * @param value {const char*} 字段值，非空指针
	 * @param replace {bool} 如果存在重复项时是否自动覆盖旧数据
	 * @return {http_header&} 返回本对象的引用，便于用户连续操作
	 */
	http_header& add_entry(const char* name, const char* value,
			bool replace = true);
	
	/**
	 * 从 HTTP 头中获得指定的头部字段
	 * @param name {const char*} 字段名，非空指针
	 * @return {const char*} 返回值 NULL 表示不存在
	 */
	const char* get_entry(const char* name) const;

	/**
	 * 设置 HTTP 头中的 Content-Length 字段
	 * @param n {int64} 设置值
	 * @return {http_header&} 返回本对象的引用，便于用户连续操作
	 */
#if defined(_WIN32) || defined(_WIN64)
	http_header& set_content_length(__int64 n);

	/**
	 * 获得通过 set_content_length 设置的 HTTP 头中的 Content-Length 值
	 * @return {int64}
	 */
	__int64 get_content_length() const
	{
		return content_length_;
	}
#else
	http_header& set_content_length(long long int n);
	long long int get_content_length() const
	{
		return content_length_;
	}
#endif

	/**
	 * 设置 HTTP 请求头（响应头）中的 Range 字段，用于分段请求（响应）数据，
	 * 多用于支持断点续传的 WEB 服务器中
	 * @param from {http_off_t} 起始偏移位置，下标从 0 开始，该
	 *  值当 >= 0 时才有效
	 * @param to {http_off_t} 请求结束偏移位置，下标从 0 开始，
	 *  在请求头中当该值输入 < 0 时，则认为是请求从起始位置开始至最终长度位置
	 * @return {http_header&} 返回本对象的引用，便于用户连续操作
	 */
#if defined(_WIN32) || defined(_WIN64)
	http_header& set_range(__int64 from, __int64 to);
#else
	http_header& set_range(long long from, long long to);
#endif

	/**
	 * 对于响应头在分段传输前需要调用此函数设置数据体总长度
	 * @param total {http_off_t} 仅对于响应头，该参数需要设为数据总长度
	 * @return {http_header&}
	 */
#if defined(_WIN32) || defined(_WIN64)
	http_header& set_range_total(__int64 total);
#else
	http_header& set_range_total(long long total);
#endif

	/**
	 * 获得由 set_range 设置的分段请求位置值
	 * @param from {http_off_t*} 非空时存储起始位置偏移
	 * @param to {http_off_t*} 非空时存储结束位置偏移
	 */
#if defined(_WIN32) || defined(_WIN64)
	void get_range(__int64* from, __int64* to);
#else
	void get_range(long long int* from, long long int* to);
#endif

	/**
	 * 设置 HTTP 头中的 Content-Type 字段
	 * @param value {const char*} 设置值
	 * @return {http_header&} 返回本对象的引用，便于用户连续操作
	 */
	http_header& set_content_type(const char* value);

	/**
	 * 设置 HTTP 头中的 Connection 字段，是否保持长连接
	 * 不过，目前并未真正支持长连接，即使设置了该标志位，
	 * 则得到响应数据后也会主动关闭连接
	 * @param on {bool} 是否保持长连接
	 * @return {http_header&} 返回本对象的引用，便于用户连续操作
	 */
	http_header& set_keep_alive(bool on);

	/**
	 * 检查当前头是否设置了保持长连接选项
	 */
	bool get_keep_alive() const
	{
		return keep_alive_;
	}

	http_header& set_upgrade(const char* value = "websocket");
	const char* get_upgrade(void) const
	{
		return upgrade_;
	}

	/**
	 * 向 HTTP 头中添加 cookie
	 * @param name {const char*} cookie 名
	 * @param value {const char*} cookie 值
	 * @param domain {const char*} 所属域
	 * @param path {const char*} 存储路径
	 * @param expires {time_t} 过期时间，当该值为 0 时表示不过期，
	 *  > 0 时，则从现在起再增加 expires 即为过期时间，单位为秒
	 * @return {http_header&} 返回本对象的引用，便于用户连续操作
	 */
	http_header& add_cookie(const char* name, const char* value,
		const char* domain = NULL, const char* path = NULL,
		time_t expires = 0);

	/**
	 * 向 HTTP 头中添加 cookie
	 * @param cookie {const http_cookie*} cookie 对象
	 * @return {http_header&} 返回本对象的引用，便于用户连续操作
	 */
	http_header& add_cookie(const HttpCookie* cookie);

	/**
	 * 从 HTTP 头中获得对应名称的 cookie 对象
	 * @param name {const char*} cookie 名
	 * @return {const HttpCookie*}
	 */
	const HttpCookie* get_cookie(const char* name) const;

	/**
	 * 将整型的日期转换为 rfc1123 字符串格式的日期
	 */
	static void date_format(char* out, size_t size, time_t t);

	/**
	 * 判断是否是 HTTP 请求头
	 * @return {bool} 返回 false 表明是 HTTP 响应头
	 */
	bool is_request(void) const;

	/**
	 * 设置标志位，针对 HTTP 请求的 URI 中的 ? 问号被转义(即被转成 %3F)的请求是否
	 * 做兼容性处理，内部缺省为做兼容性处理
	 * @param on {bool} 为 true 表示做兼容性处理
	 */
	static void uri_unsafe_correct(bool on);

	//////////////////////////////////////////////////////////////////////
	//                        HTTP 请求方法函数
	//////////////////////////////////////////////////////////////////////
	
	/**
	 * 创建 HTTP 请求头数据
	 * @param buf {string&} 存储结果数据
	 * @return {bool} 创建请求头中否成功
	 */
	bool build_request(string& buf) const;

	/**
	 * 设置请求的 URL，url 格式示例如下：
	 * 1、http://www.test.com/
	 * 2、/cgi-bin/test.cgi
	 * 3、http://www.test.com/cgi-bin/test.cgi
	 * 3、http://www.test.com/cgi-bin/test.cgi?name=value
	 * 4、/cgi-bin/test.cgi?name=value
	 * 5、http://www.test.com
	 * 如果该 url 中有主机字段，则内部自动添加主机；
	 * 如果该 url 中有参数字段，则内部自动进行处理并调用 add_param 方法；
	 * 调用该函数后用户仍可以调用 add_param 等函数添加其它参数；
	 * 当参数字段只有参数名没有参数值时，该参数将会被忽略，所以如果想
	 * 单独添加参数名，应该调用 add_param 方法来添加
	 * @param url {const char*} 请求的 url，非空指针
	 * @param encoding {bool} 是否对存在于 url 中的参数进行 url 编码
	 * @return {http_header&} 返回本对象的引用，便于用户连续操作
	 */
	http_header& set_url(const char* url, bool encoding = true);

	/**
	 * 设置 HTTP 请求头的 HOST 字段
	 * @param value {const char*} 请求头的 HOST 字段值
	 * @return {http_header&} 返回本对象的引用，便于用户连续操作
	 */
	http_header& set_host(const char* value);

	/**
	 * 获得设置的 HTTP 请求头中的 HOST 字段
	 * @return {const char*} 返回空指针表示没有设置 HOST 字段
	 */
	const char* get_host() const
	{
		return host_[0] == 0 ? NULL : host_;
	}

	/**
	 * 设置 HTTP 协议的请求方法，如果不调用此函数，则默认用 GET 方法
	 * @param method {http_method_t} HTTP 请求方法
	 * @return {http_header&} 返回本对象的引用，便于用户连续操作
	 */
	http_header& set_method(http_method_t method);

	/**
	 * 设置 HTTP 协议的请求方法，本函数允许用户扩展 HTTP 请求方法，
	 * 通过该函数设置的请求方法仅影响 HTTP 请求过程
	 * @param method {const char*} 请求方法
	 * @return {http_header&} 返回本对象的引用，便于用户连续操作
	 */
	http_header& set_method(const char* method);

	/**
	 * 当作为请求头时，本函数取得当前邮件头的请求方法
	 * @param buf {string*} 存储用字符串表示的请求方法
	 * @return {http_method_t}
	 */
	http_method_t get_method(string* buf = NULL) const;

	/**
	 * 设置 HTTP 请求头中是否允许接收压缩数据，对应的 HTTP 头字段为：
	 * Accept-Encoding: gzip, deflate，但目前仅支持 gzip 格式
	 * @param on {bool} 如果为 true 则自动添加 HTTP 压缩头请求
	 * @return {http_header&} 返回本对象的引用，便于用户连续操作
	 */
	http_header& accept_gzip(bool on);

	/**
	 * 向请求的 URL 中添加参数对，当只有参数名没有参数值时则：
	 * 1、参数名非空串，但参数值为空指针，则 URL 参数中只有：{name}
	 * 2、参数名非空串，但参数值为空串，则 URL参数中为：{name}=
	 * @param name {const char*} 参数名，不能为空指针
	 * @param value {const char*} 参数值，当为空指针时，仅添加参数名，
	 * @return {http_header&} 返回本对象的引用，便于用户连续操作
	 */
	http_header& add_param(const char* name, const char* value);
	http_header& add_int(const char* name, short value);
	http_header& add_int(const char* name, int value);
	http_header& add_int(const char* name, long value);
	http_header& add_int(const char* name, unsigned short value);
	http_header& add_int(const char* name, unsigned int value);
	http_header& add_int(const char* name, unsigned long value);
	http_header& add_format(const char* name, const char* fmt, ...)
		ACL_CPP_PRINTF(3, 4);
#if defined(_WIN32) || defined(_WIN64)
	http_header& add_int(const char* name, __int64 vlaue);
	http_header& add_int(const char* name, unsigned __int64 vlaue);
#else
	http_header& add_int(const char* name, long long int value);
	http_header& add_int(const char* name, unsigned long long int value);
#endif

	http_header& set_ws_origin(const char* url);
	http_header& set_ws_key(const void* key, size_t len);
	http_header& set_ws_key(const char* key);
	http_header& set_ws_protocol(const char* proto);
	http_header& set_ws_version(int ver);

	const char* get_ws_origin(void) const
	{
		return ws_origin_;
	}

	const char* get_ws_key(void) const
	{
		return ws_sec_key_;
	}

	const char* get_ws_protocol(void) const
	{
		return ws_sec_proto_;
	}

	int get_ws_version(void) const
	{
		return ws_sec_ver_;
	}

	http_header& set_ws_accept(const char* key);
	const char* get_ws_accept(void) const
	{
		return ws_sec_accept_;
	}

	/**
	 * url 重定向
	 * @param url {const char*} 重定向的 URL，格式为：
	 *  http://xxx.xxx.xxx/xxx 或 /xxx
	 *  如果是前者，则自动从中取出 HOST 字段，如果是后者，则
	 *  延用之前的 HOST
	 */
	bool redirect(const char* url);

	/**
	 * 设置重定向次数，如果该值 == 0 则不主动进行重定向，否则
	 * 进行重定向且重定向的次数由该值决定
	 * @param n {int} 允许重定向的次数
	 * @return {http_header&} 返回本对象的引用，便于用户连续操作
	 */
	http_header& set_redirect(unsigned int n = 5);

	/**
	 * 获取通过 set_redirect 设置的允许的最大重定向次数
	 * @return {unsigned int}
	 */
	unsigned int get_redirect(void) const;

	/**
	 * 当需要重定向时，会主动调用此函数允许子类做一些重置工作
	 */
	virtual void redicrect_reset(void) {}

	//////////////////////////////////////////////////////////////////////
	//                       HTTP 响应方法函数
	//////////////////////////////////////////////////////////////////////

	/**
	 * 创建 HTTP 响应头数据
	 * @param buf {string&} 存储结果数据
	 * @return {bool} 创建响应头中否成功
	 */
	bool build_response(string& buf) const;

	/**
	 * 设置 HTTP 响应头中的响应状态字
	 * @param status {int} 状态字如：1xx, 2xx, 3xx, 4xx, 5xx
	 * @return {http_header&} 返回本对象的引用，便于用户连续操作
	 */
	http_header& set_status(int status);

	/**
	 * 获得响应头中的 HTTP 状态字
	 * @return {int} HTTP 响应状态码：1xx, 2xx, 3xx, 4xx, 5xx
	 */
	int get_status(void) const
	{
		return status_;
	}

	/**
	 * 设置 HTTP 响应头中的 chunked 传输标志
	 * @param on {bool}
	 * @return {http_header&}
	 */
	http_header& set_chunked(bool on);

	/**
	 * 判断当前 HTTP 传输是否采用 chunked 传输方式
	 * @return {bool}
	 */
	bool chunked_transfer(void) const
	{
		return chunked_transfer_;
	}

	/**
	 * 设置是否用来生成 CGI 格式的响应头
	 * @param on {bool} 是否 CGI 格式响应头
	 * @return {http_header&} 返回本对象的引用，便于用户连续操作
	 */
	http_header& set_cgi_mode(bool on);

	/**
	 * 是否设置了 CGI 模式
	 * @return {bool}
	 */
	bool is_cgi_mode() const
	{
		return cgi_mode_;
	}

	/**
	 * 设置传输的数据是否采用 gzip 方式进行压缩
	 * @param on {bool}
	 * @return {http_header&}
	 */
	http_header& set_transfer_gzip(bool on);

	/**
	 * 获得当前的数据传输是否设置了采用 gzip 压缩方式
	 * @return {bool}
	 */
	bool is_transfer_gzip() const
	{
		return transfer_gzip_;
	}

private:
	dbuf_guard* dbuf_internal_;
	dbuf_guard* dbuf_;
	bool fixed_;                          // HTTP 是否已经完整了
	//char* domain_;  // HTTP 服务器域名
	//unsigned short port_;               // HTTP 服务器端口
	char* url_;                           // HTTP 请求的 URL
	std::list<HTTP_PARAM*> params_;       // 请求参数集合
	std::list<HttpCookie*> cookies_;      // cookies 集合
	std::list<HTTP_HDR_ENTRY*> entries_;  // HTTP 请求头中各字段集合
	http_method_t method_;                // HTTP 请求的方法
	char  method_s_[64];                  // HTTP 请求方法以字符串表示
	char  host_[256];                     // HTTP 请求头中的 HOST 字段
	bool keep_alive_;                     // 是否保持长连接
	unsigned int nredirect_;              // 最大重定向的次数限制
	bool accept_compress_;                // 是否接收压缩数据
	int  status_;                         // 响应头的状态字
	bool is_request_;                     // 是请求头还是响应头
	bool cgi_mode_;                       // 是否 CGI 响应头
#if defined(_WIN32) || defined(_WIN64)
	__int64 range_from_;                  // 请求头中，range 起始位置
	__int64 range_to_;                    // 请求头中，range 结束位置
	__int64 range_total_;                 // range 传输模式下记录数据总长度
	__int64 content_length_;              // HTTP 数据体长度
#else
	long long int range_from_;            // 请求头中，range 起始位置
	long long int range_to_;              // 请求头中，range 结束位置
	long long int range_total_;           // range 传输模式下记录数据总长度
	long long int content_length_;        // HTTP 数据体长度
#endif
	bool chunked_transfer_;               // 是否为 chunked 传输模式
	bool transfer_gzip_;                  // 数据是否采用 gzip 压缩

	char* upgrade_;
	// just for websocket
	char* ws_origin_;
	char* ws_sec_key_;
	char* ws_sec_proto_;
	int   ws_sec_ver_;
	char* ws_sec_accept_;

	void init(void);                      // 初始化
	void clear(void);
	void build_common(string& buf) const; // 构建通用头

	void add_res_cookie(const HTTP_HDR_ENTRY& entry);
	void append_accept_key(const char* sec_key, string& out) const;
	unsigned char* create_ws_key(const void* key, size_t len) const;
};

}  // namespace acl end
