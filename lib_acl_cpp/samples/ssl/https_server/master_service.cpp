#include "stdafx.h"
#include "http_servlet.h"
#include "master_service.h"

////////////////////////////////////////////////////////////////////////////////
// 配置内容项

char *var_cfg_libcrypto_path;
char *var_cfg_libx509_path;
char *var_cfg_libssl_path;
char *var_cfg_crt_file;
char *var_cfg_key_file;
acl::master_str_tbl var_conf_str_tab[] = {
#ifdef __APPLE__
	{ "libcrypto_path", "./libmbedcrypto.dylib", &var_cfg_libcrypto_path },
	{ "libx509_path", "./libmbedx509.dylib", &var_cfg_libx509_path },
	{ "libssl_path", "./libmbedtls.dylib", &var_cfg_libssl_path },
#else
	{ "libcrypto_path", "./libmbedcrypto.so", &var_cfg_libcrypto_path },
	{ "libx509_path", "./libmbedx509.so", &var_cfg_libx509_path },
	{ "libssl_path", "./lib.so", &var_cfg_libssl_path },
#endif
	{ "crt_file", "./ssl_crt.pem", &var_cfg_crt_file },
	{ "key_file", "./ssl_key.pem", &var_cfg_key_file },

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
	if (conf_) {
		delete conf_;
	}
}

static acl::sslbase_io* setup_ssl(acl::socket_stream& conn, acl::sslbase_conf& conf)
{
	acl::sslbase_io* hook = (acl::sslbase_io*) conn.get_hook();
	if (hook != NULL) {
		return hook;
	}

	// 对于使用 SSL 方式的流对象，需要将 SSL IO 流对象注册至网络
	// 连接流对象中，即用 ssl io 替换 stream 中默认的底层 IO 过程

	//logger("begin setup ssl hook...");

	// 采用阻塞 SSL 握手方式
	acl::sslbase_io* ssl = conf.create(false);
	if (conn.setup_hook(ssl) == ssl) {
		logger_error("setup_hook error!");
		ssl->destroy();
		return NULL;
	}

	if (!ssl->handshake()) {
		logger_error("ssl handshake failed");
		return NULL;
	}

	if (!ssl->handshake_ok()) {
		logger("handshake trying again...");
		return NULL;
	}

	logger("handshake_ok");

	return ssl;
}

bool master_service::thread_on_read(acl::socket_stream* conn)
{
	http_servlet* servlet = (http_servlet*) conn->get_ctx();
	if (servlet == NULL) {
		logger_fatal("servlet null!");
	}

	if (conf_ == NULL) {
		return servlet->doRun("127.0.0.1:11211", conn);
	}

	acl::sslbase_io* ssl = setup_ssl(*conn, *conf_);
	if (ssl == NULL) {
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
	if (var_cfg_crt_file == NULL || *var_cfg_crt_file == 0
		|| var_cfg_key_file == NULL || *var_cfg_key_file == 0) {
		return;
	}

	if (strstr(var_cfg_libssl_path, "mbedtls")) {
		acl::mbedtls_conf::set_libpath(var_cfg_libcrypto_path,
			var_cfg_libx509_path, var_cfg_libssl_path);
		if (!acl::mbedtls_conf::load()) {
			logger_error("load %s error", var_cfg_libssl_path);
			return;
		}

		conf_ = new acl::mbedtls_conf(true);
	} else if (strstr(var_cfg_libssl_path, "polarssl")) {
		acl::polarssl_conf::set_libpath(var_cfg_libssl_path);
		if (!acl::polarssl_conf::load()) {
			logger_error("load %s error", var_cfg_libssl_path);
			return;
		}

		conf_ = new acl::polarssl_conf();
	}

	// 允许服务端的 SSL 会话缓存功能
	conf_->enable_cache(var_cfg_session_cache);

	// 添加本地服务的证书
	if (!conf_->add_cert(var_cfg_crt_file)) {
		logger_error("add cert failed, crt: %s, key: %s",
			var_cfg_crt_file, var_cfg_key_file);
		delete conf_;
		conf_ = NULL;
		return;
	}
	logger("load cert ok, crt: %s, key: %s",
		var_cfg_crt_file, var_cfg_key_file);

	// 添加本地服务密钥
	if (!conf_->set_key(var_cfg_key_file)) {
		logger_error("set private key error");
		delete conf_;
		conf_ = NULL;
	}
}

void master_service::proc_on_exit()
{
}
