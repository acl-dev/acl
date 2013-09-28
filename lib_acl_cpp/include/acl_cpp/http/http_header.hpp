#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <list>
#include "acl_cpp/http/http_type.hpp"

struct HTTP_HDR_RES;
struct HTTP_HDR_ENTRY;

namespace acl {

class string;
class HttpCookie;

/**
	* HTTP 头类，可以构建请求头或响应头
	*/
class ACL_CPP_API http_header
{
public:
	http_header(void);

	/**
	 * HTTP 请求头构造函数
	 * @param url {const char*} 请求的 url，
	 * 该 url 不能包含 ? 以及 ? 后面的参数部分，如果想添加该 url 的参数，
	 * 应该通过调用 add_param 完成，url 格式
	 * 如: http://www.test.com/, /cgi-bin/test.cgi,
	 *     http://www.test.com/cgi-bin/test.cgi
	 * 不能为: http://www.test.com/cgi-bin/test.cgi?name=value 或
	 * /cgi-bin/test.cgi?name=value，因为其中的 name=value 参数
	 * 必须由 add_param 来添加
	 */
	http_header(const char* url);

	/**
	 * HTTP 响应头构造函数
	 * @param status {int} 状态字如：1xx, 2xx, 3xx, 4xx, 5xx
	 */
	http_header(int status);
	virtual ~http_header(void);

	/**
	 * 重置 HTTP 头信息同时将上次的临时资源释放
	 */
	void reset(void);

	//////////////////////////////////////////////////////////////////////////
	//            HTTP 请求与 HTTP 响应通用的方法函数
	//////////////////////////////////////////////////////////////////////////

	/**
	 * 设置 HTTP 头是客户端的请求头还是服务器的响应头
	 * @param onoff {bool} true 表示是请求头，否则表示响应头
	 * @return {http_header&} 返回本对象的引用，便于用户连续操作
	 */
	http_header& set_request_mode(bool onoff);

	/**
	 * 向 HTTP 头中添加字段
	 * @param name {const char*} 字段名
	 * @param value {const char*} 字段值
	 * @return {http_header&} 返回本对象的引用，便于用户连续操作
	 */
	http_header& add_entry(const char* name, const char* value);
	
	/**
	 * 设置 HTTP 头中的 Content-Length 字段
	 * @param n {int64} 设置值
	 * @return {http_header&} 返回本对象的引用，便于用户连续操作
	 */
#ifdef WIN32
	http_header& set_content_length(__int64 n);
#else
	http_header& set_content_length(long long int n);
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
	 * @param cookie {http_cookie*} 必须是动态分配的 cookie 对象
	 * @return {http_header&} 返回本对象的引用，便于用户连续操作
	 */
	http_header& add_cookie(HttpCookie* cookie);

	/**
	 * 将整型的日期转换为 rfc1123 字符串格式的日期
	 */
	static void date_format(char* out, size_t size, time_t t);

	/**
	 * 判断是否是 HTTP 请求头
	 * @return {bool} 返回 false 表明是 HTTP 响应头
	 */
	bool is_request(void) const;

	//////////////////////////////////////////////////////////////////////////
	//            HTTP 请求方法函数
	//////////////////////////////////////////////////////////////////////////
	
	/**
	 * 创建 HTTP 请求头数据
	 * @param buf {string&} 存储结果数据
	 * @return {bool} 创建请求头中否成功
	 */
	bool build_request(string& buf) const;

	/**
	 * 设置请求的 URL，该 URL 不能包含 ? 以及 ? 后面的参数部分，
	 * 如果想添加该 URL 的参数，应该通过调用 add_param 完成，url 格式
	 * 如: http://www.test.com/, /cgi-bin/test.cgi,
	 *     http://www.test.com/cgi-bin/test.cgi
	 * 不能为: http://www.test.com/cgi-bin/test.cgi?name=value 或
	 * /cgi-bin/test.cgi?name=value，因为其中的 name=value 参数必须由 add_param
	 * 来添加
	 * @param url {const char*} 请求的 url
	 * @return {http_header&} 返回本对象的引用，便于用户连续操作
	 */
	http_header& set_url(const char* url);

	/**
	 * 设置 HTTP 请求头的 HOST 字段
	 * @param value {const char*} 请求头的 HOST 字段值
	 * @return {http_header&} 返回本对象的引用，便于用户连续操作
	 */
	http_header& set_host(const char* value);

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
	 * @return {http_method_t}
	 */
	http_method_t get_method(void) const
	{
		return method_;
	}

	/**
	 * 设置 HTTP 请求头中 Range 字段，用于分段请求数据，多用于
	 * 支持断点续传的 WEB 服务器中
	 * @param from {http_off_t} 起始偏移位置，下标从 0 开始，该
	 *  值当 >= 0 时才有效
	 * @param to {http_off_t} 请求结束偏移位置，下标从 0 开始，
	 *  当该值输入 < 0 时，则认为是请求从起始位置开始至最终长度
	 *  位置
	 * @return {http_header&} 返回本对象的引用，便于用户连续操作
	 */
#ifdef WIN32
	http_header& set_range(__int64 from, __int64 to);
#else
	http_header& set_range(long long int from, long long int to);
#endif

	/**
	 * 获得由 set_range 设置的分段请求位置值
	 * @param from {http_off_t*} 非空时存储起始位置偏移
	 * @param to {http_off_t*} 非空时存储结束位置偏移
	 */
#ifdef WIN32
	void get_range(__int64* from, __int64* to);
#else
	void get_range(long long int* from, long long int* to);
#endif

	/**
	 * 设置 HTTP 请求头中是否允许接收压缩数据，对应的 HTTP 头字段为：
	 * Accept-Encoding: gzip, deflate，但目前仅支持 gzip 格式
	 * @param on {bool} 如果为 true 则自动添加 HTTP 压缩头请求
	 * @return {http_header&} 返回本对象的引用，便于用户连续操作
	 */
	http_header& accept_gzip(bool on);

	/**
	 * 向请求的 URL 中添加参数对
	 * @param name {const char*} 参数名
	 * @param value {const char*} 参数值
	 * @return {http_header&} 返回本对象的引用，便于用户连续操作
	 */
	http_header& add_param(const char* name, const char* value);

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

	//////////////////////////////////////////////////////////////////////////
	//            HTTP 响应方法函数
	//////////////////////////////////////////////////////////////////////////

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
	 * 设置是否用来生成 CGI 格式的响应头
	 * @param on {bool} 是否 CGI 格式响应头
	 * @return {http_header&} 返回本对象的引用，便于用户连续操作
	 */
	http_header& set_cgi_mode(bool on);

protected:
private:
	//char* domain_;  // HTTP 服务器域名
	unsigned short port_;                 // HTTP 服务器端口
	char* url_;                           // HTTP 请求的 URL
	std::list<HTTP_PARAM*> params_;       // 请求参数集合
	std::list<HttpCookie*> cookies_;      // cookies 集合
	std::list<HTTP_HDR_ENTRY*> entries_;  // HTTP 请求头中各字段集合
	http_method_t method_;                // HTTP 请求的方法
	char  method_s_[64];                  // HTTP 请求方法以字符串表示
	bool keep_alive_;                     // 是否保持长连接
	unsigned int nredirect_;              // 最大重定向的次数限制
	bool accept_compress_;                // 是否接收压缩数据
	int  status_;                         // 响应头的状态字
	bool is_request_;                     // 是请求头还是响应头
	bool cgi_mode_;                       // 是否 CGI 响应头
#ifdef WIN32
	__int64 range_from_;                  // 请求头中，range 起始位置
	__int64 range_to_;                    // 请求头中，range 结束位置
#else
	long long int range_from_;            // 请求头中，range 起始位置
	long long int range_to_;              // 请求头中，range 结束位置
#endif

	void init(void);                      // 初始化
	void clear(void);
	void build_common(string& buf) const; // 构建通用头
};

}  // namespace acl end
