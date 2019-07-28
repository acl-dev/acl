#include "stdafx.h"
#include "http_servlet.h"
#include "master_service.h"

//////////////////////////////////////////////////////////////////////////////
// 配置内容项

char *var_cfg_ssl_path;
char *var_cfg_crt_file;
char *var_cfg_key_file;
char *var_cfg_log_file;
char *var_cfg_addrs_map;
acl::master_str_tbl var_conf_str_tab[] = {
	{ "ssl_path", "../libpolarssl.so", &var_cfg_ssl_path },
	{ "crt_file", "", &var_cfg_crt_file },
	{ "key_file", "", &var_cfg_key_file },
	{ "log_file", "./log.txt", &var_cfg_log_file },
	{ "addrs_map", "", &var_cfg_addrs_map },

	{ 0, 0, 0 }
};

int  var_cfg_session_cache;
int  var_cfg_client_ssl;
acl::master_bool_tbl var_conf_bool_tab[] = {
	{ "session_cache", 1, &var_cfg_session_cache },
	{ "client_ssl", 1, &var_cfg_client_ssl },

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

//////////////////////////////////////////////////////////////////////////////

master_service::master_service()
: server_ssl_conf_(NULL)
, client_ssl_conf_(NULL)
{
}

master_service::~master_service()
{
	if (server_ssl_conf_)
		delete server_ssl_conf_;
	if (client_ssl_conf_)
		delete client_ssl_conf_;
}

acl::polarssl_io* master_service::setup_ssl(acl::socket_stream& conn,
	acl::polarssl_conf& conf)
{
	acl::polarssl_io* hook = (acl::polarssl_io*) conn.get_hook();
	if (hook != NULL)
		return hook;

	// 对于使用 SSL 方式的流对象，需要将 SSL IO 流对象注册至网络
	// 连接流对象中，即用 ssl io 替换 stream 中默认的底层 IO 过程

	out_.puts("begin setup ssl hook...");

	// 采用阻塞 SSL 握手方式
	acl::polarssl_io* ssl = new acl::polarssl_io(conf, true, false);
	if (conn.setup_hook(ssl) == ssl)
	{
		logger_error("setup_hook error!");
		ssl->destroy();
		return NULL;
	}

	out_.format("setup hook ok, tid: %lu", acl::thread::thread_self());
	return ssl;
}

bool master_service::thread_on_read(acl::socket_stream* conn)
{
	http_servlet* servlet = (http_servlet*) conn->get_ctx();
	if (servlet == NULL)
		logger_fatal("servlet null!");

	if (server_ssl_conf_ == NULL)
		return servlet->doRun("127.0.0.1:11211", conn);

	acl::polarssl_io* ssl = setup_ssl(*conn, *server_ssl_conf_);
	if (ssl == NULL)
		return false;

	if (ssl->handshake() == false)
	{
		out_.puts("ssl handshake failed");
		return false;
	}

	if (ssl->handshake_ok() == false)
	{
		out_.puts("handshake trying ...");
		return true;
	}

	return servlet->doRun("127.0.0.1:11211", conn);
}

bool master_service::thread_on_accept(acl::socket_stream* conn)
{
//	logger("connect from %s, fd: %d", conn->get_peer(true),
//		conn->sock_handle());

	conn->set_rw_timeout(var_cfg_io_timeout);

	http_servlet* servlet = new http_servlet(out_, client_ssl_conf_);
	servlet->setParseBody(false);
	conn->set_ctx(servlet);

	return true;
}

bool master_service::thread_on_timeout(acl::socket_stream* conn acl_unused)
{
//	logger("read timeout from %s, fd: %d", conn->get_peer(),
//		conn->sock_handle());
	return false;
}

void master_service::thread_on_close(acl::socket_stream* conn)
{
//	logger("disconnect from %s, fd: %d", conn->get_peer(),
//		conn->sock_handle());

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

const char* master_service::get_addr(const char* from) const
{
	if (from == NULL || *from == 0)
		return NULL;
	acl::string key(from);
	key.lower();
	std::map<acl::string, acl::string>::const_iterator cit =
		addrs_map_.find(key);
	if (cit != addrs_map_.end())
		return cit->second.c_str();
	else
	{
		logger("Local not exist from: %s\r\n", from);
		return NULL;
	}
}

void master_service::create_addrs_map()
{
	if (var_cfg_addrs_map == NULL || *var_cfg_addrs_map == 0)
		return;

	// 数据格式：domain11:port11|domain12:port12, ...
	acl::string buf(var_cfg_addrs_map);
	std::vector<acl::string>& addrs = buf.split2(" \t,;");
	for (std::vector<acl::string>::iterator it = addrs.begin();
		it != addrs.end(); ++it)
	{
		std::vector<acl::string> pair = (*it).split2("|");
		if (pair.size() == 2)
		{
			addrs_map_[pair[0].lower()] = pair[1].lower();
			logger("add addr map: %s->%s", pair[0].c_str(),
				pair[1].c_str());
		}
	}
}

void master_service::proc_on_init()
{
	create_addrs_map();

	if (var_cfg_log_file != NULL && *var_cfg_log_file != 0)
	{
		if (out_.open_write(var_cfg_log_file) == false)
			logger_error("open %s error %s", var_cfg_log_file,
				acl::last_serror());
		else
			logger("open %s ok", var_cfg_log_file);
	}
	else
		logger("no log file open");

	if (var_cfg_client_ssl)
		client_ssl_conf_ = new acl::polarssl_conf;
	else
		client_ssl_conf_ = NULL;

	if (var_cfg_crt_file == NULL || *var_cfg_crt_file == 0
		|| var_cfg_key_file == NULL || *var_cfg_key_file == 0)
	{
		return;
	}

	// 加载服务端 SSL 证书
	server_ssl_conf_ = new acl::polarssl_conf();

	// 允许服务端的 SSL 会话缓存功能
	server_ssl_conf_->enable_cache(var_cfg_session_cache ? true : false);

	// 添加本地服务的证书
	if (server_ssl_conf_->add_cert(var_cfg_crt_file) == false)
	{
		logger_error("add cert failed, crt: %s, key: %s",
			var_cfg_crt_file, var_cfg_key_file);
		delete server_ssl_conf_;
		server_ssl_conf_ = NULL;
		return;
	}
	logger("load cert ok, crt: %s, key: %s",
		var_cfg_crt_file, var_cfg_key_file);

	// 添加本地服务密钥
	if (server_ssl_conf_->set_key(var_cfg_key_file) == false)
	{
		logger_error("add key failed, crt: %s, key: %s",
			var_cfg_crt_file, var_cfg_key_file);
		delete server_ssl_conf_;
		server_ssl_conf_ = NULL;
	}
	else
	{
		acl::polarssl_conf::set_libpath(var_cfg_ssl_path);
		acl::polarssl_conf::load();
	}
}

void master_service::proc_on_exit()
{
}
