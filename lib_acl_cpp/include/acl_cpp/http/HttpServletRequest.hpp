#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <vector>
#include "acl_cpp/http/http_header.hpp"
#include "acl_cpp/http/http_ctype.hpp"
#include "acl_cpp/http/http_type.hpp"

namespace acl {

class istream;
class ostream;
class socket_stream;
class http_client;
class http_mime;
class session;
class HttpSession;
class HttpCookie;
class HttpServletResponse;

/**
 * 与 HTTP 客户端请求相关的类，该类不应被继承，用户也不需要
 * 定义或创建该类对象
 */
class ACL_CPP_API HttpServletRequest
{
public:
	/**
	 * 构造函数
	 * @param res {HttpServletResponse&}
	 * @param store {session&} 存储会话数据的对象
	 * @param stream {socket_stream&} 数据流，内部不会主动关闭流
	 * @param local_charset {const char*} 本地字符集，该值非空时，
	 *  内部会自动将 HTTP 请求的数据转换为本地字符集，否则不转换
	 * @param body_parse {bool} 针对 POST 方法，该参数指定是否需要
	 *  读取 HTTP 请求数据体并按 n/v 方式进行分析；当为 true 则内
	 *  部会读取 HTTP 请求体数据，并进行分析，当用户调用 getParameter
	 *  时，不仅可以获得 URL 中的参数，同时可以获得 POST 数据体中
	 *  的参数；当该参数为 false 时则不读取数据体，把读数据体的任务
	 *  交给子类处理
	 * @param body_limit {int} 针对 POST 方法，当数据体为文本参数
	 *  类型时，此参数限制数据体的长度；当数据体为数据流或 MIME
	 *  格式或 body_read 为 false，此参数无效
	 */
	HttpServletRequest(HttpServletResponse& res, session& store,
		socket_stream& stream, const char* local_charset = NULL,
		bool body_parse = true, int body_limit = 102400);
	~HttpServletRequest(void);

	/**
	 * 获得 HTTP 客户端请求方法：GET, POST, PUT, CONNECT, PURGE
	 * @return {http_method_t}
	 */
	http_method_t getMethod(void) const;

	/**
	 * 获得 HTTP 客户端请求的所有 cookie 对象集合
	 * @return {const std::vector<HttpCookie*>&}
	 */
	const std::vector<HttpCookie*>& getCookies(void) const;

	/**
	 * 获得 HTTP 客户端请求的某个 cookie 值
	 * @param name {const char*} cookie 名称，必须非空
	 * @return {const char*} cookie 值，当返回 NULL 时表示
	 *  cookie 值不存在
	 */
	const char* getCookieValue(const char* name) const;

	/**
	 * 给 HTTP 请求对象添加 cookie 对象
	 * @param name {const char*} cookie 名，非空字符串
	 * @param value {const char*} cookie 值，非空字符串
	 */
	void setCookie(const char* name, const char* value);

	/**
	 * 获得 HTTP 请求头中的某个字段值
	 * @param name {const char*} HTTP 请求头中的字段名，非空
	 * @return {const char*} HTTP 请求头中的字段值，返回 NULL
	 *  时表示不存在
	 */
	const char* getHeader(const char* name) const;

	/**
	 * 获得 HTTP GET 请求方式 URL 中的参数部分，即 ? 后面的部分
	 * @return {const char*} 没有进行URL 解码的请求参数部分，
	 *  返回 NULL 则表示 URL 中没有参数
	 */
	const char* getQueryString(void) const;

	/**
	 * 获得  http://test.com.cn/cgi-bin/test?name=value 中的
	 * /cgi-bin/test 路径部分
	 * @return {const char*} 返回空表示不存在？
	 */
	const char* getPathInfo(void) const;

	/**
	 * 获得  http://test.com.cn/cgi-bin/test?name=value 中的
	 * /cgi-bin/test?name=value 路径部分
	 * @return {const char*} 返回空表示不存在？
	 */
	const char* getRequestUri(void) const;

	/**
	 * 获得与该 HTTP 会话相关的 HttpSession 对象引用
	 * @param create {bool} 当 session 不存在时是否在缓存服务器自动创建；
	 *  当某客户端的 session 不存在且该参数为 false 时，则该函数返
	 *  回的 session 对象会因没有被真正创建而无法进行读写操作
	 * @param sid {const char*} 当 session 不存在，且 create 参数非空时，
	 *  如果 sid 非空，则使用此值设置用户的唯一会话，同时添加进客户端的
	 *  cookie 中
	 * @return {HttpSession&}
	 *  注：优先级，浏览器 COOKIE > create = true > sid != NULL
	 */
	HttpSession& getSession(bool create = true, const char* sid = NULL);

	/**
	 * 获得与 HTTP 客户端连接关联的输入流对象引用
	 * @return {istream&}
	 */
	istream& getInputStream(void) const;

	/**
	 * 获得 HTTP 请求数据的数据长度
	 * @return {acl_int64} 返回 -1 表示可能为 GET 方法，
	 *  或 HTTP 请求头中没有 Content-Length 字段
	 */
#ifdef WIN32
	__int64 getContentLength(void) const;
#else
	long long int getContentLength(void) const;
#endif
	/**
	 * 获得 HTTP 请求头中 Content-Type: text/html; charset=gb2312
	 * Content-Type 的字段值
	 * @param part {bool} 如果为 true 则返回 text，否则返回完整的
	 * 值，如：text/html; charset=gb2312
	 * @return {const char*} 返回 NULL 表示 Content-Type 字段不存在
	 */
	const char* getContentType(bool part = true) const;

	/**
	 * 获得 HTTP 请求头中的 Content-Type: text/html; charset=gb2312
	 * 中的 charset 字段值 gb2312
	 * @return {const char*} 返回 NULL 表示 Content-Type 字段 或
	 *  charset=xxx 不存在
	 */
	const char* getCharacterEncoding(void) const;

	/**
	 * 返回本地的字段字符集
	 * @ return {const char*} 返回 NULL 表示没有设置本地字符集
	 */
	const char* getLocalCharset(void) const;

	/**
	 * 返回 HTTP 连接的本地 IP 地址
	 * @return {const char*} 返回空，表示无法获得
	 */
	const char* getLocalAddr(void) const;

	/**
	 * 返回 HTTP 连接的本地 PORT 号
	 * @return {unsigned short} 返回 0 表示无法获得
	 */
	unsigned short getLocalPort(void) const;

	/**
	 * 返回 HTTP 连接的远程客户端 IP 地址
	 * @return {const char*} 返回空，表示无法获得
	 */
	const char* getRemoteAddr(void) const;

	/**
	 * 返回 HTTP 连接的远程客户端 PORT 号
	 * @return {unsigned short} 返回 0 表示无法获得
	 */
	unsigned short getRemotePort(void) const;

	/**
	 * 获得 HTTP 请求头中设置的 Host 字段
	 * @return {const char*} 如果为空，则表示不存在
	 */
	const char* getRemoteHost(void) const;

	/**
	 * 获得 HTTP 请求头中设置的 User-Agent 字段
	 * @return {const char*} 如果为空，则表示不存在
	 */
	const char* getUserAgent(void) const;

	/**
	 * 获得 HTTP 请求中的参数值，该值已经被 URL 解码且
	 * 转换成本地要求的字符集；针对 GET 方法，则是获得
	 * URL 中 ? 后面的参数值；针对 POST 方法，则可以获得
	 * URL 中 ? 后面的参数值或请求体中的参数值
	 */
	const char* getParameter(const char* name) const;

	/**
	 * 当 HTTP 请求头中的 Content-Type 为
	 * multipart/form-data; boundary=xxx 格式时，说明为文件上传
	 * 数据类型，则可以通过此函数获得 http_mime 对象
	 * @return {const http_mime*} 返回 NULL 则说明没有 MIME 对象，
	 *  返回的值用户不能手工释放，因为在 HttpServletRequest 的析
	 *  构中会自动释放
	 */
	http_mime* getHttpMime(void) const;

	/**
	 * 获得 HTTP 请求数据的类型
	 * @return {http_request_t}，一般对 POST 方法中的上传
	 *  文件应用而言，需要调用该函数获得是否是上传数据类型，
	 *  当该函数返回 HTTP_REQUEST_OTHER 时，用户可以通过调用
	 *  getContentType 获得具体的类型字符串
	 */
	http_request_t getRequestType(void) const;

	/**
	 * 获得 HTTP 请求页面的 referer URL
	 * @return {const char*} 为 NULL 则说明用户直接访问本 URL
	 */
	const char* getRequestReferer(void) const;

	/**
	 * 获得根据 HTTP 请求头获得的 http_ctype 对象
	 * @return {const http_ctype&}
	 */
	const http_ctype& getHttpCtype(void) const;

	/**
	 * 判断 HTTP 客户端是否要求保持长连接
	 * @return {bool}
	 */
	bool isKeepAlive(void) const;

	/**
	 * 当客户端要求保持长连接时，从 HTTP 请求头中获得保持的时间
	 * @return {int} 返回值 < 0 表示不存在 Keep-Alive 字段
	 */
	int getKeepAlive(void) const;

	/*
	 * 当 HTTP 请求为 POST 方法，通过本函数设置读 HTTP 数据体的
	 * IO 超时时间值(秒)
	 * @param rw_timeout {int} 读数据体时的超时时间(秒)
	 */
	void setRwTimeout(int rw_timeout);

	/**
	 * 获得上次出错的错误号
	 * @return {http_request_error_t}
	 */
	http_request_error_t getLastError(void) const;

	/**
	 * 当 HttpServlet 类以服务模式(即非 CGI 方式)运行时，可以调用此
	 * 方法获得客户端连接的 HTTP 类对象，从而获得更多的参数
	 * @return {http_client*} 当以服务模式运行时，此函数返回 HTTP 客户端
	 *  连接非空对象；当以 CGI 方式运行时，则返回空指针
	 */
	http_client* getClient(void) const;

	/**
	 * 将 HTTP 请求头输出至流中（文件流或网络流）
	 * @param out {ostream&}
	 * @param prompt {const char*} 提示内容
	 */	 
	void fprint_header(ostream& out, const char* prompt);
private:
	http_request_error_t req_error_;
	char cookie_name_[64];
	HttpServletResponse& res_;
	session& store_;
	HttpSession* http_session_;
	socket_stream& stream_;
	bool body_parse_;
	int  body_limit_;

	std::vector<HttpCookie*> cookies_;
	bool cookies_inited_;
	http_client* client_;
	http_method_t method_;
	bool cgi_mode_;
	http_ctype content_type_;
	char localAddr_[32];
	char remoteAddr_[32];
	char localCharset_[32];
	int  rw_timeout_;
	std::vector<HTTP_PARAM*> params_;
	http_request_t request_type_;
	http_mime* mime_;

	bool readHeaderCalled_;
	bool readHeader(void);

	void parseParameters(const char* str);
};

} // namespace acl
