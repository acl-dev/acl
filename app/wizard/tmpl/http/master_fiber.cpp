#include "stdafx.h"
#include "http_service.h"
#include "http_servlet.h"
#include "master_service.h"

char *var_cfg_libcrypto_path;	// For OpenSSL, MbedTLS
char *var_cfg_libx509_path;	// For MbedTLS
char *var_cfg_libssl_path;	// For OpenSSL, MbedTLS, and PolarSSL
char *var_cfg_crt_file;		// For OpenSSL, MbedTLS, and PolarSSL
char *var_cfg_key_file;		// For OpenSSL, MbedTLS, and PolarSSL
char *var_cfg_key_pass;		// For OpenSSL, MbedTLS, and PolarSSL

acl::master_str_tbl var_conf_str_tab[] = {
	{ "libcrypto_path",	"",	&var_cfg_libcrypto_path	},
	{ "libx509_path",	"",	&var_cfg_libx509_path	},
	{ "libssl_path",	"",	&var_cfg_libssl_path	},
	{ "crt_file",		"",	&var_cfg_crt_file	},
	{ "key_file",		"",	&var_cfg_key_file	},
	{ "key_pass",		"",	&var_cfg_key_pass	},

	{ 0, 0, 0 }
};

int   var_cfg_ssl_session_cache;

acl::master_bool_tbl var_conf_bool_tab[] = {
	{ "ssl_session_cache",	1,	&var_cfg_ssl_session_cache },

	{ 0, 0, 0 }
};

static int  var_cfg_io_timeout;

acl::master_int_tbl var_conf_int_tab[] = {
	{ "io_timeout",		120,	&var_cfg_io_timeout, 0, 0 },

	{ 0, 0 , 0 , 0, 0 }
};

acl::master_int64_tbl var_conf_int64_tab[] = {
	{ 0, 0 , 0 , 0, 0 }
};


//////////////////////////////////////////////////////////////////////////

master_service::master_service(void)
: conf_(NULL)
{
	service_ = new http_service;
}

master_service::~master_service(void)
{
}

http_service& master_service::get_service(void) const
{
	return *service_;
}

acl::sslbase_io* master_service::setup_ssl(acl::socket_stream& conn,
		acl::sslbase_conf& conf)
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
		ssl->destroy();
		return NULL;
	}

	if (!ssl->handshake_ok()) {
		logger("handshake trying again...");
		ssl->destroy();
		return NULL;
	}

	logger("handshake_ok");

	return ssl;
}

void master_service::on_accept(acl::socket_stream& conn)
{
	logger("connect from %s, fd %d", conn.get_peer(), conn.sock_handle());

	if (conf_) {
		acl::sslbase_io* ssl = setup_ssl(conn, *conf_);
		if (ssl == NULL) {
			return;
		}
	}

	conn.set_rw_timeout(120);

	acl::memcache_session session("127.0.0.1:11211");
	http_servlet servlet(*service_, &conn, &session);

	// charset: big5, gb2312, gb18030, gbk, utf-8
	servlet.setLocalCharset("utf-8");

	while(servlet.doRun()) {}

	logger("disconnect from %s", conn.get_peer());
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
	logger(">>>proc_on_init<<<");

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

		conf_ = new acl::mbedtls_conf(true);
	} else if (strstr(var_cfg_libssl_path, "polarssl")) {
		acl::polarssl_conf::set_libpath(var_cfg_libssl_path);
		if (!acl::polarssl_conf::load()) {
			logger_error("load %s error", var_cfg_libssl_path);
			return;
		}

		logger("PolarSSL loaded, ssl=%s", var_cfg_libssl_path);

		conf_ = new acl::polarssl_conf();
	} else if (strstr(var_cfg_libssl_path, "libssl")) {
		acl::openssl_conf::set_libpath(var_cfg_libcrypto_path,
			var_cfg_libssl_path);
		if (!acl::openssl_conf::load()) {
			logger_error("load %s error", var_cfg_libssl_path);
			return;
		}

		logger("OpenSSL loaded, crypto=%s, ssl=%s",
			var_cfg_libcrypto_path, var_cfg_libssl_path);

		conf_ = new acl::openssl_conf(true);
	} else {
		logger("unsupported ssl=%s", var_cfg_libssl_path);
		return;
	}

	// 允许服务端的 SSL 会话缓存功能
	conf_->enable_cache(var_cfg_ssl_session_cache);

	// 添加本地服务的证书及服务密钥
	if (!conf_->add_cert(var_cfg_crt_file, var_cfg_key_file,
			var_cfg_key_pass)) {

		logger_error("add cert failed, crt: %s, key: %s",
			var_cfg_crt_file, var_cfg_key_file);
		delete conf_;
		conf_ = NULL;
		return;
	}

	logger("load cert ok, crt: %s, key: %s",
		var_cfg_crt_file, var_cfg_key_file);
}

void master_service::proc_on_exit(void)
{
	logger(">>>proc_on_exit<<<");

	delete conf_;
	delete service_;
}

bool master_service::proc_on_sighup(acl::string&)
{
	logger(">>>proc_on_sighup<<<");
	return true;
}
