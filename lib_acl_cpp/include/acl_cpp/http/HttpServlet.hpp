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
	 * 设置本地字符集，如果设置了本地字符集，则在接收 HTTP 请求
	 * 数据时，会自动将请求的字符集转为本地字符集
	 * @param charset {const char*} 本地字符集，如果该指针为空，
	 *  则清除本地字符集
	 */
	void setLocalCharset(const char* charset);

	/**
	 * 设置 HTTP 会话过程的 IO 读写超时时间
	 * @param rw_timeout {int} 读写超时时间(秒)
	 */
	void setRwTimeout(int rw_timeout);

	/**
	 * HttpServlet 对象开始运行，接收 HTTP 请求，并回调以下 doXXX 虚函数
	 * @param session {session&} 存储 session 数据的对象
	 * @param stream {socket_stream*} 当在 acl_master 服务器框架控制下
	 *  运行时，该参数必须非空；当在 apache 下以 CGI 方式运行时，该参数
	 *  设为 NULL；另外，该函数内部不会关闭流连接，应用应自行处理流对象
	 *  的关闭情况，这样可以方便与 acl_master 架构结合
	 * @param body_parse {bool} 针对 POST 方法，该参数指定是否需要
	 *  读取 HTTP 请求数据体并按 n/v 方式进行分析；当为 true 则内
	 *  部会读取 HTTP 请求体数据，并进行分析，当用户调用 getParameter
	 *  时，不仅可以获得 URL 中的参数，同时可以获得 POST 数据体中
	 *  的参数；当该参数为 false 时则不读取数据体
	 * @param body_limit {int} 针对 POST 方法，当数据体为文本参数
	 *  类型时，此参数限制数据体的长度；当数据体为数据流或 MIME
	 *  格式或 body_read 为 false，此参数无效
	 * @return {bool} 返回处理结果
	 */
	bool doRun(session& session, socket_stream* stream = NULL,
		bool body_parse = true, int body_limit = 102400);

	/**
	 * HttpServlet 对象开始运行，接收 HTTP 请求，并回调以下 doXXX 虚函数，
	 * 调用本函数意味着采用 memcached 来存储 session 数据
	 * @param memcached_addr {const char*} memcached 服务器地址，格式：IP:PORT
	 * @param stream {socket_stream*} 含义同上
	 * @param body_parse {bool} 含义同上
	 * @param body_limit {int} 含义同上
	 * @return {bool} 返回处理结果
	 */
	bool doRun(const char* memcached_addr = "127.0.0.1:11211",
		socket_stream* stream = NULL,
		bool body_parse = true, int body_limit = 102400);

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
};

} // namespace acl
