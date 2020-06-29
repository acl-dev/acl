#pragma once

class http_service : public acl::master_proc
{
public:
	http_service(void);
	~http_service(void);

public:
	// Register all Http handlers with the http url path

	http_service& Get(const char* path, http_handler_t fn);
	http_service& Post(const char* path, http_handler_t fn);
	http_service& Head(const char* path, http_handler_t fn);
	http_service& Put(const char* path, http_handler_t fn);
	http_service& Patch(const char* path, http_handler_t fn);
	http_service& Connect(const char* path, http_handler_t fn);
	http_service& Purge(const char* path, http_handler_t fn);
	http_service& Delete(const char* path, http_handler_t fn);
	http_service& Options(const char* path, http_handler_t fn);
	http_service& Propfind(const char* path, http_handler_t fn);
	http_service& Websocket(const char* path, http_handler_t fn);
	http_service& Unknown(const char* path, http_handler_t fn);
	http_service& Error(const char* path, http_handler_t fn);

public:
	/**
	 * 当以 CGI 方式运行时的入口
	 */
	void do_cgi(void);

protected:
	/**
	 * @override
	 * 虚函数：当接收到一个客户端连接时调用此函数
	 * @param stream {aio_socket_stream*} 新接收到的客户端异步流对象
	 * 注：该函数返回后，流连接将会被关闭，用户不应主动关闭该流
	 */
	void on_accept(acl::socket_stream* stream);

	/**
	 * @override
	 * 在进程启动时，服务进程每成功监听一个本地地址，便调用本函数
	 * @param ss {acl::server_socket&} 监听对象
	 */
	void proc_on_listen(acl::server_socket& ss);

	/**
	 * @override
	 * 当进程切换用户身份后调用的回调函数，此函数被调用时，进程
	 * 的权限为普通受限级别
	 */
	void proc_on_init(void);

	/**
	 * @override
	 * 当进程退出前调用的回调函数
	 */
	void proc_on_exit(void);

	/**
	 * @override
	 * 当进程收到 SIGHUP 信号后的回调函数
	 */
	bool proc_on_sighup(acl::string&);

private:
	http_handlers_t handlers_[http_handler_max];

	void Service(int type, const char* path, http_handler_t fn);
};
