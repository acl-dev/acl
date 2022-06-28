#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"

#ifndef ACL_CLIENT_ONLY

namespace acl {

class dbuf_guard;
class string;
class xml;
class json;
class ostream;
class socket_stream;
class http_header;
class http_client;
class HttpCookie;
class HttpServletRequest;

/**
 * 与 HTTP 客户端响应相关的类，该类不应被继承，用户也不需要
 * 定义或创建该类对象
 */
class ACL_CPP_API HttpServletResponse : public noncopyable
{
public:
	/**
	 * 构造函数
	 * @param stream {socket_stream&} 数据流，内部不会自动关闭流
	 */
	HttpServletResponse(socket_stream& stream);
	~HttpServletResponse(void);

	/**
	 * 设置 HTTP 响应数据体的长度
	 * @param n {acl_int64} 数据体长度
	 */
#if defined(_WIN32) || defined(_WIN64)
	HttpServletResponse& setContentLength(__int64 n);
#else
	HttpServletResponse& setContentLength(long long int n);
#endif

	/**
	 * 设置 HTTP chunked 传输模式
	 * @param on {bool} 如果为 true，即使设置了 setContentLength，
	 *  则内部也会采用 chunked 传输方式，根据 HTTP RFC 规范要求，
	 *  chunked 传输的优先级高级 conteng-length 方式
	 * @return {HttpServletResponse&}
	 */
	HttpServletResponse& setChunkedTransferEncoding(bool on);

	/**
	 * 设置与 HTTP 客户端保持联系长连接
	 * @param on {bool}
	 * @return {HttpServletResponse&}
	 */
	HttpServletResponse& setKeepAlive(bool on);

	/**
	 * 设置 HTTP 响应数据体的 Content-Type 字段值，可字段值可以为：
	 * text/html 或 text/html; charset=utf8 格式
	 * @param value {const char*} 字段值
	 * @return {HttpServletResponse&}
	 */
	HttpServletResponse& setContentType(const char* value);

	/**
	 * 设置 HTTP 响应数据体采用 gzip 压缩格式
	 * @param gzip {bool} 是否采用 gzip 压缩格式
	 * @return {HttpServletResponse&}
	 */
	HttpServletResponse& setContentEncoding(bool gzip);

	/**
	 * 设置 HTTP 响应数据体中字符集，当已经在 setContentType 设置
	 * 了字符集，则就不必再调用本函数设置字符集
	 * @param charset {const char*} 响应体数据的字符集
	 * @return {HttpServletResponse&}
	 */
	HttpServletResponse& setCharacterEncoding(const char* charset);

	/**
	 * 设置 HTTP 响应头中的日期格式的字段
	 * @param name {const char*} HTTP 响应头中的字段名
	 * @param value {time_t} 时间值
	 */
	HttpServletResponse& setDateHeader(const char* name, time_t value);

	/**
	 * 设置 HTTP 响应头中的字符串格式字段
	 * @param name {const char*} HTTP 响应头中的字段名
	 * @param value {const char*} 字段值
	 */
	HttpServletResponse& setHeader(const char* name, const char* value);

	/**
	 * 设置 HTTP 响应头中的整数格式字段
	 * @param name {const char*} HTTP 响应头中的字段名
	 * @param value {int} 字段值
	 */
	HttpServletResponse& setHeader(const char* name, int value);

	/**
	 * 对于分区下载，调用本函数设置数据下载的偏移位置（下标从 0 开始）
	 * @param from {http_off_t} 数据区间起始偏移位置（下标从 0 开始计算）
	 * @param to {http_off_t} 数据区间结束位置（该值需小于总数据长度）
	 * @param total {http_off_t} 总数据长度，当数据源为一个静态文件时该值
	 *  应等于该文件的总长度大小
	 * @return {HttpServletResponse&}
	 */
#if  defined(_WIN32) || defined(_WIN64)
	HttpServletResponse& setRange(__int64 from,
		__int64 to, __int64 total);
#else
	HttpServletResponse& setRange(long long from,
		long long to, long long total);
#endif

	/**
	 * 设置 HTTP 响应头中的状态码：1xx, 2xx, 3xx, 4xx, 5xx
	 * @param status {int} HTTP 响应状态码, 如：200
	 */
	HttpServletResponse& setStatus(int status);

	/**
	 * 设置为 CGI 模式，用户一般不需手工设置，因为 HttpServlet 类
	 * 会自动设置是否是 CGI 模式
	 * @param on {bool} 是否是 CGI 模式
	 */
	HttpServletResponse& setCgiMode(bool on);

	/**
	 * 设置 HTTP 响应头中的重定向 location 字段
	 * @param location {const char*} URL，非空
	 * @param status {int} HTTP 响应状态码，一般为 3xx 类
	 */
	HttpServletResponse& setRedirect(const char* location, int status = 302);

	/**
	 * 添加 cookie 对象，该对象必须是动态分配的，且用户自己不能
	 * 再显示释放该对象，因为内部会自动释放
	 * @param cookie {HttpCookie*}
	 */
	HttpServletResponse& addCookie(HttpCookie* cookie);

	/**
	 * 添加 cookie
	 * @param name {const char*} cookie 名
	 * @param value {const char*} cookie 值
	 * @param domain {const char*} cookie 存储域
	 * @param path {const char*} cookie 存储路径
	 * @param expires {time_t} cookie 过期时间间隔，当当前时间加
	 *  该值为 cookie 的过期时间截(秒)
	 */
	HttpServletResponse& addCookie(const char* name, const char* value,
		const char* domain = NULL, const char* path = NULL,
		time_t expires = 0);

	/**
	 * 将 url 进行 url 编码
	 * @param out {string&} 存储编码后的结果
	 * @param url {const char*} 未编码前原始的 url
	 */
	void encodeUrl(string& out, const char* url);

	/**
	 * 获得 HTTP 响应头
	 * @return {http_header&}
	 */
	http_header& getHttpHeader(void) const;

	/**
	 * 向客户端发送 HTTP 数据体响应数据，可以循环调用此函数，
	 * 当通过 setChunkedTransferEncoding 设置了 chunked 传输方式后，
	 * 内部自动采用 chunked 传输方式；调用此函数不必显式调用
	 * sendHeader 函数来发送 HTTP 响应头，因为内部会自动在第一次
	 * 写时发送 HTTP 响应头；另外，在使用 chunked 方式传输数据时，
	 * 应该应该最后再调用一次本函数，且参数均设为 0 表示数据结束
	 * @param data {const void*} 数据地址
	 * @param len {size_t} data 数据长度
	 * @return {bool} 发送是否成功，如果返回 false 表示连接中断
	 */
	bool write(const void* data, size_t len);

	/**
	 * 向客户端发送 HTTP 数据体响应数据，可以循环调用此函数，该函数
	 * 内部调用 HttpServletResponse::write(const void*, size_t) 过程，
	 * 另外，在使用 chunked 方式传输数据时，应该应该最后再调用一次本函数，
	 * 且输入空串，即 buf.empty() == true
	 * @param buf {const string&} 数据缓冲区
	 * @return {bool} 发送是否成功，如果返回 false 表示连接中断
	 */
	bool write(const string& buf);

	/**
	 * 向客户端发送 HTTP Xml 响应数据体
	 * @param body {const xml&} 数据缓冲区
	 * @param charset {const char*} 数据体字符集
	 * @return {bool} 发送是否成功，如果返回 false 表示连接中断
	 */
	bool write(const xml& body, const char* charset = "utf-8");

	/**
	 * 向客户端发送 HTTP Json 响应数据体
	 * @param body {const json&} 数据缓冲区
	 * @param charset {const char*} 数据体字符集
	 * @return {bool} 发送是否成功，如果返回 false 表示连接中断
	 */
	bool write(const json& body, const char* charset = "utf-8");

	/**
	 * 带格式方式向 HTTP 客户端发送响应数据，内部自动调用
	 * HttpServletResponse::write(const void*, size_t) 过程，在使用
	 * chunked 方式传输数据时，应该应该最后再调用 write(NULL, 0)
	 * 表示数据结束
	 * @param fmt {const char*} 变参格式字符串
	 * @return {int} 成功则返回值 > 0，否则返回 -1
	 */
	int format(const char* fmt, ...) ACL_CPP_PRINTF(2, 3);

	/**
	 * 带格式方式向 HTTP 客户端发送响应数据，内部自动调用
	 * HttpServletResponse::write(const string&) 过程，在使用 chunked
	 * 方式传输数据时，应该应该最后再调用 write(NULL, 0) 表示数据结束
	 * @param fmt {const char*} 变参格式字符串
	 * @param ap {va_list} 变参列表
	 * @return {int} 成功则返回值 > 0，否则返回 -1
	 */
	int vformat(const char* fmt, va_list ap);

	///////////////////////////////////////////////////////////////////

	/**
	 * 发送 HTTP 响应头，用户应该发送数据体前调用此函数将 HTTP
	 * 响应头发送给客户端
	 * @return {bool} 发送是否成功，若返回 false 则表示连接中断，
	 *  当调用以上几个写的函数时，本函数不必显式被调用，如果是
	 *  通过从 getOutputStream 获得的 socket 流写数据时，则本函数
	 *  必须显式被调用
	 */
	bool sendHeader(void);

	/**
	 * 获得 HTTP 响应对象的输出流对象，用户在调用 sendHeader 发送
	 * 完 HTTP 响应头后，通过该输出流来发送 HTTP 数据体
	 * @return {ostream&}
	 */
	ostream& getOutputStream(void) const;

	/**
	 * 获得 HTTP 双向流对象，由构造函数的参数输入
	 * @return {socket_stream&}
	 */
	socket_stream& getSocketStream(void) const;

	/**
	 * 获得底层的 http_client 通信对象
	 * @return {http_client*} 非 NULL
	 */
	http_client* getClient() const
	{
		return client_;
	}

	/**
	 * 设置 http 请求对象，该函数目前只应被 HttpServlet 类内部调用
	 * @param request {HttpServletRequest*}
	 */
	void setHttpServletRequest(HttpServletRequest* request);

private:
	dbuf_guard* dbuf_internal_;
	dbuf_guard* dbuf_;
	socket_stream& stream_;		// 客户端连接流
	HttpServletRequest* request_;	// http 请求对象
	http_client* client_;		// http 响应流对象
	http_header* header_;		// http 响应头
	char  charset_[32];		// 字符集
	char  content_type_[32];	// content-type 类型
	bool  head_sent_;		// 是否已经发送了 HTTP 响应头
};

}  // namespace acl

#endif // ACL_CLIENT_ONLY
