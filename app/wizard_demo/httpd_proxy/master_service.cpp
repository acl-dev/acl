#include "stdafx.h"
#include "http_servlet.h"
#include "master_service.h"

char *var_cfg_libcrypto_path;	// For OpenSSL, MbedTLS
char *var_cfg_libx509_path;	// For MbedTLS
char *var_cfg_libssl_path;	// For OpenSSL, MbedTLS, and PolarSSL
char *var_cfg_crt_file;		// For OpenSSL, MbedTLS, and PolarSSL
char *var_cfg_key_file;		// For OpenSSL, MbedTLS, and PolarSSL
char *var_cfg_key_pass;		// For OpenSSL, MbedTLS, and PolarSSL
char *var_cfg_https_port;

acl::master_str_tbl var_conf_str_tab[] = {
	{ "libcrypto_path",	"",	&var_cfg_libcrypto_path	},
	{ "libx509_path",	"",	&var_cfg_libx509_path	},
	{ "libssl_path",	"",	&var_cfg_libssl_path	},
	{ "crt_file",		"",	&var_cfg_crt_file	},
	{ "key_file",		"",	&var_cfg_key_file	},
	{ "key_pass",		"",	&var_cfg_key_pass	},
	{ "https_port",		"443",	&var_cfg_https_port	},

	{ 0, 0, 0 }
};

static int  var_cfg_debug_enable;

acl::master_bool_tbl var_conf_bool_tab[] = {
	{ "debug_enable", 1, &var_cfg_debug_enable },

	{ 0, 0, 0 }
};

static int  var_cfg_io_timeout;

acl::master_int_tbl var_conf_int_tab[] = {
	{ "io_timeout", 120, &var_cfg_io_timeout, 0, 0 },

	{ 0, 0 , 0 , 0, 0 }
};

acl::master_int64_tbl var_conf_int64_tab[] = {
	{ 0, 0 , 0 , 0, 0 }
};


//////////////////////////////////////////////////////////////////////////

master_service::master_service(void)
: server_conf_(NULL)
, client_conf_(NULL)
{
}

master_service::~master_service(void)
{
	delete server_conf_;
	delete client_conf_;
}

acl::sslbase_io* master_service::setup_ssl(acl::socket_stream& conn,
		acl::sslbase_conf& server_conf)
{
	acl::sslbase_io* hook = (acl::sslbase_io*) conn.get_hook();
	if (hook != NULL) {
		return hook;
	}

	// 对于使用 SSL 方式的流对象，需要将 SSL IO 流对象注册至网络
	// 连接流对象中，即用 ssl io 替换 stream 中默认的底层 IO 过程

	logger_debug(DEBUG_SSL, 2, "begin setup ssl hook...");

	// 采用阻塞 SSL 握手方式
	acl::sslbase_io* ssl = server_conf.create(false);
	if (conn.setup_hook(ssl) == ssl) {
		logger_error("setup_hook error!");
		ssl->destroy();
		return NULL;
	}

	if (!ssl->handshake()) {
		logger_error("ssl handshake failed");
		ssl->destroy();
		return NULL;
	}

	if (!ssl->handshake_ok()) {
		logger_error("handshake trying again...");
		ssl->destroy();
		return NULL;
	}

	logger_debug(DEBUG_SSL, 2, "handshake_ok");
	return ssl;
}

void master_service::on_accept(acl::socket_stream& conn)
{
	logger_debug(DEBUG_CONN, 2, "connect from %s, fd %d",
		conn.get_peer(), conn.sock_handle());

	const char* local = conn.get_local(true);
	if (local == NULL || *local == 0) {
		logger_error("get_local null from fd=%d", conn.sock_handle());
		return;
	}

	bool use_ssl = false;
	acl::string buf(local);
	if (buf.end_with(var_cfg_https_port) && server_conf_) {
		if (setup_ssl(conn, *server_conf_) == NULL) {
			logger_error("setup ssl error");
			return;
		}
		use_ssl = true;
	} else {
		//printf("local=%s, https=%s, server_conf_=%p\n",
		//	local, var_cfg_https_port, server_conf_);
	}

	conn.set_rw_timeout(5);

	acl::memcache_session* session = new acl::memcache_session("127.0.0.1:11211");
	http_servlet* servlet = new http_servlet(&conn, session,
			use_ssl ? client_conf_ : NULL);

	// charset: big5, gb2312, gb18030, gbk, utf-8
	servlet->setLocalCharset("utf-8");
	servlet->setParseBody(false);

	while(servlet->doRun()) {}

	logger_debug(DEBUG_CONN, 2, "disconnect from %s, fd %d",
		conn.get_peer(), conn.sock_handle());

	delete session;
	delete servlet;
}

void master_service::proc_pre_jail(void)
{
	logger(">>>proc_pre_jail<<<");
}

void master_service::proc_on_listen(acl::server_socket& ss)
{
	logger(">>>listen %s ok<<<", ss.get_addr());
}

void master_service::proc_on_init(void)
{
	logger(">>>proc_on_init: shared stack size=%zd<<<",
		acl::fiber::get_shared_stack_size());

	// 下面用来初始化 SSL 功能

	if (var_cfg_crt_file == NULL || *var_cfg_crt_file == 0
		|| var_cfg_key_file == NULL || *var_cfg_key_file == 0) {
		logger("not use SSL mode");
		return;
	}

	if (strstr(var_cfg_libssl_path, "mbedtls")) {
		acl::mbedtls_conf::set_libpath(var_cfg_libcrypto_path,
			var_cfg_libx509_path, var_cfg_libssl_path);
		if (!acl::mbedtls_conf::load()) {
			logger_error("load %s error", var_cfg_libssl_path);
			return;
		}

		logger("MbedTLS loaded, crypto=%s, x509=%s, ssl=%s",
			var_cfg_libcrypto_path, var_cfg_libx509_path,
			var_cfg_libssl_path);

		server_conf_ = new acl::mbedtls_conf(true);
		client_conf_ = new acl::mbedtls_conf(false);
	} else if (strstr(var_cfg_libssl_path, "polarssl")) {
		acl::polarssl_conf::set_libpath(var_cfg_libssl_path);
		if (!acl::polarssl_conf::load()) {
			logger_error("load %s error", var_cfg_libssl_path);
			return;
		}

		logger("PolarSSL loaded, ssl=%s", var_cfg_libssl_path);

		server_conf_ = new acl::polarssl_conf();
		client_conf_ = new acl::polarssl_conf();
	} else if (strstr(var_cfg_libssl_path, "libssl")) {
		acl::openssl_conf::set_libpath(var_cfg_libcrypto_path,
			var_cfg_libssl_path);
		if (!acl::openssl_conf::load()) {
			logger_error("load %s error", var_cfg_libssl_path);
			return;
		}

		logger("OpenSSL loaded, crypto=%s, ssl=%s",
			var_cfg_libcrypto_path, var_cfg_libssl_path);

		server_conf_ = new acl::openssl_conf(true, 5);
		client_conf_ = new acl::openssl_conf(false, 5);
	} else {
		logger("unsupported ssl=%s", var_cfg_libssl_path);
		return;
	}

	// 允许服务端的 SSL 会话缓存功能
	//server_conf_->enable_cache(var_cfg_ssl_session_cache);

	// 添加本地服务的证书及服务密钥
	if (!server_conf_->add_cert(var_cfg_crt_file, var_cfg_key_file,
			var_cfg_key_pass)) {

		logger_error("add cert failed, crt: %s, key: %s",
			var_cfg_crt_file, var_cfg_key_file);
		delete server_conf_;
		server_conf_ = NULL;
		return;
	}

	logger("load cert ok, crt: %s, key: %s",
		var_cfg_crt_file, var_cfg_key_file);
}

void master_service::proc_on_exit(void)
{
	logger(">>>proc_on_exit<<<");
}

bool master_service::proc_on_sighup(acl::string&)
{
	logger(">>>proc_on_sighup<<<");
	return true;
}
