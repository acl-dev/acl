#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/http/http_header.hpp"

namespace acl {

class socket_stream;
class session;
class HttpServletRequest;
class HttpServletResponse;

/**
 * 处理 HTTP 客户端请求的基类，子类需要继承该类
 */
class ACL_CPP_API HttpServlet
{
public:
	HttpServlet(void);
	virtual ~HttpServlet(void) = 0;

	/**
	 * 设置本地字符集，如果设置了本地字符集，则在接收 HTTP 请求数据时，会自动将请求的
	 * 字符集转为本地字符集；该函数必须在 doRun 之前调用才有效
	 * @param charset {const char*} 本地字符集，如果该指针为空，
	 *  则清除本地字符集
	 * @return {HttpServlet&}
	 */
	HttpServlet& setLocalCharset(const char* charset);

	/**
	 * 设置 HTTP 会话过程的 IO 读写超时时间；该函数必须在 doRun 之前调用才有效
	 * @param rw_timeout {int} 读写超时时间(秒)
	 * @return {HttpServlet&}
	 */
	HttpServlet& setRwTimeout(int rw_timeout);

	/**
	 * 针对 POST 方法，该方法设置是否需要解析数据体数据，默认为解析，该函数必须在 doRun
	 * 之前调用才有效；当数据体为数据流或 MIME 格式，即使调用本方法设置了解析数据，也不
	 * 会对数据体进行解析
	 * @param on {bool} 是否需要解析
	 * @return {HttpServlet&}
	 */
	HttpServlet& setParseBody(bool on);

	/**
	 * 针对 POST 方法，该方法设置解析数据体的最大长度，如果数据体，该函数必须在 doRun
	 * 之前调用才有效
	 * @param length {int} 最大长度限制，如果请求的数据体长度过大，则直接返回 false
	 * @return {HttpServlet&}
	 */
	HttpServlet& setParseBodyLimit(int length);

	/**
	 * HttpServlet 对象开始运行，接收 HTTP 请求，并回调以下 doXXX 虚函数
	 * @param session {session&} 存储 session 数据的对象
	 * @param stream {socket_stream*} 当在 acl_master 服务器框架控制下
	 *  运行时，该参数必须非空；当在 apache 下以 CGI 方式运行时，该参数
	 *  设为 NULL；另外，该函数内部不会关闭流连接，应用应自行处理流对象
	 *  的关闭情况，这样可以方便与 acl_master 架构结合
	 * @return {bool} 返回处理结果
	 */
	bool doRun(session& session, socket_stream* stream = NULL);

	/**
	 * HttpServlet 对象开始运行，接收 HTTP 请求，并回调以下 doXXX 虚函数，
	 * 调用本函数意味着采用 memcached 来存储 session 数据
	 * @param memcached_addr {const char*} memcached 服务器地址，格式：IP:PORT
	 * @param stream {socket_stream*} 含义同上
	 * @return {bool} 返回处理结果
	 */
	bool doRun(const char* memcached_addr = "127.0.0.1:11211",
		socket_stream* stream = NULL);

	/**
	 * 当 HTTP 请求为 GET 方式时的虚函数
	 */
	virtual bool doGet(HttpServletRequest&, HttpServletResponse&)
	{
		logger_error("child not implement doGet yet!");
		return false;
	}

	/**
	 * 当 HTTP 请求为 POST 方式时的虚函数
	 */
	virtual bool doPost(HttpServletRequest&, HttpServletResponse&)
	{
		logger_error("child not implement doPost yet!");
		return false;
	}

	/**
	 * 当 HTTP 请求为 PUT 方式时的虚函数
	 */
	virtual bool doPut(HttpServletRequest&, HttpServletResponse&)
	{
		logger_error("child not implement doPut yet!");
		return false;
	}

	/**
	 * 当 HTTP 请求为 CONNECT 方式时的虚函数
	 */
	virtual bool doConnect(HttpServletRequest&, HttpServletResponse&)
	{
		logger_error("child not implement doConnect yet!");
		return false;
	}

	/**
	 * 当 HTTP 请求为 PURGE 方式时的虚函数，该方法在清除 SQUID 的缓存
	 * 时会用到
	 */
	virtual bool doPurge(HttpServletRequest&, HttpServletResponse&)
	{
		logger_error("child not implement doPurge yet!");
		return false;
	}

	/**
	 * 当 HTTP 请求方法未知时的虚函数
	 */
	virtual bool doUnknown(HttpServletRequest&, HttpServletResponse&)
	{
		logger_error("child not implement doUnknown yet!");
		return false;
	}

	/**
	 * 当 HTTP 请求出错时的虚函数
	 */
	virtual bool doError(HttpServletRequest&, HttpServletResponse&)
	{
		logger_error("child not implement doError yet!");
		return false;
	}

protected:
private:
	char local_charset_[32];
	int  rw_timeout_;
	bool parse_body_enable_;
	int  parse_body_limit_;
};

} // namespace acl
