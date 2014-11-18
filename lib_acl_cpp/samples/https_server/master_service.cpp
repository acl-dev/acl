#include "stdafx.h"
#include "http_servlet.h"
#include "master_service.h"

////////////////////////////////////////////////////////////////////////////////
// 配置内容项

char *var_cfg_crt_file;
char *var_cfg_key_file;
acl::master_str_tbl var_conf_str_tab[] = {
	{ "crt_file", "./mm263com1.crt", &var_cfg_crt_file },
	{ "key_file", "./mm263com.key", &var_cfg_key_file },

	{ 0, 0, 0 }
};

int  var_cfg_session_cache;
acl::master_bool_tbl var_conf_bool_tab[] = {
	{ "session_cache", 1, &var_cfg_session_cache },

	{ 0, 0, 0 }
};

int  var_cfg_io_timeout;
acl::master_int_tbl var_conf_int_tab[] = {
	{ "io_timeout", 60, &var_cfg_io_timeout, 0, 0 },

	{ 0, 0 , 0 , 0, 0 }
};

acl::master_int64_tbl var_conf_int64_tab[] = {

	{ 0, 0 , 0 , 0, 0 }
};

////////////////////////////////////////////////////////////////////////////////

master_service::master_service()
: conf_(NULL)
{
}

master_service::~master_service()
{
	if (conf_)
		delete conf_;
}

static bool ssl_handshake(acl::socket_stream& conn, acl::polarssl_conf& conf)
{
	acl::stream_hook* hook = conn.get_hook();
	if (hook != NULL)
		return true;

	// 对于使用 SSL 方式的流对象，需要将 SSL IO 流对象注册至网络
	// 连接流对象中，即用 ssl io 替换 stream 中默认的底层 IO 过程

	logger("begin setup ssl hook...");

	acl::polarssl_io* ssl = new acl::polarssl_io(conf, true);
	if (conn.setup_hook(ssl) == ssl)
	{
		logger_error("setup_hook error!");
		ssl->destroy();
		return false;
	}

	logger("setup hook ok, tid: %lu", acl::thread::thread_self());
	return true;
}

bool master_service::thread_on_read(acl::socket_stream* conn)
{
	http_servlet* servlet = (http_servlet*) conn->get_ctx();
	if (servlet == NULL)
		logger_fatal("servlet null!");

	if (conf_ == NULL)
		return servlet->doRun("127.0.0.1:11211", conn);

	// 当 thread_on_accept 在主线程中时，则需要在子线程中进行 SSL 握手
	if (acl_var_threads_thread_accept == 0
		&& ssl_handshake(*conn, *conf_) == false)
	{
		return false;
	}

	return servlet->doRun("127.0.0.1:11211", conn);
}

bool master_service::thread_on_accept(acl::socket_stream* conn)
{
	logger("connect from %s, fd: %d", conn->get_peer(true),
		conn->sock_handle());

	conn->set_rw_timeout(var_cfg_io_timeout);

	http_servlet* servlet = new http_servlet();
	conn->set_ctx(servlet);

	// 当 thread_on_accept 在子线程中时，则可以在子线程中进行 SSL 握手
	if (acl_var_threads_thread_accept
		&& ssl_handshake(*conn, *conf_) == false)
	{
		return false;
	}

	return true;
}

bool master_service::thread_on_timeout(acl::socket_stream* conn)
{
	logger("read timeout from %s, fd: %d", conn->get_peer(),
		conn->sock_handle());
	return false;
}

void master_service::thread_on_close(acl::socket_stream* conn)
{
	logger("disconnect from %s, fd: %d", conn->get_peer(),
		conn->sock_handle());

	http_servlet* servlet = (http_servlet*) conn->get_ctx();
	if (servlet)
		delete servlet;
}

void master_service::thread_on_init()
{
}

void master_service::thread_on_exit()
{
}

void master_service::proc_on_init()
{
	logger("main tid: %lu, ioctl_thread_accept: %d",
		acl::thread::thread_self(), acl_var_threads_thread_accept);

	if (var_cfg_crt_file == NULL || *var_cfg_crt_file == 0
		|| var_cfg_key_file == NULL || *var_cfg_key_file == 0)
	{
		return;
	}

	conf_ = new acl::polarssl_conf();

	// 允许服务端的 SSL 会话缓存功能
	conf_->enable_cache(var_cfg_session_cache);

	// 添加本地服务的证书
	if (conf_->add_cert(var_cfg_crt_file) == false)
	{
		logger_error("add cert failed, crt: %s, key: %s",
			var_cfg_crt_file, var_cfg_key_file);
		delete conf_;
		conf_ = NULL;
		return;
	}
	logger("load cert ok, crt: %s, key: %s",
		var_cfg_crt_file, var_cfg_key_file);

	// 添加本地服务密钥
	if (conf_->set_key(var_cfg_key_file) == false)
	{
		logger_error("set private key error");
		delete conf_;
		conf_ = NULL;
	}
}

void master_service::proc_on_exit()
{
}
