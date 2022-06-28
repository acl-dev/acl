// main.cpp : 定义控制台应用程序的入口点。
//
#include "stdafx.h"
#include <assert.h>
#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"

class echo_thread : public acl::thread {
public:
	echo_thread(acl::sslbase_conf& ssl_conf, acl::socket_stream* conn)
	: ssl_conf_(ssl_conf), conn_(conn) {}

private:
	acl::sslbase_conf&  ssl_conf_;
	acl::socket_stream* conn_;

	~echo_thread(void) { delete conn_; }

	// @override
	void* run(void) {
		conn_->set_rw_timeout(60);

		// 给 socket 安装 SSL IO 过程
		if (!setup_ssl()) {
			return NULL;
		}

		do_echo();

		delete this;
		return NULL;
	}

	bool setup_ssl(void) {
		bool non_block = false;
		acl::sslbase_io* ssl = ssl_conf_.open(non_block);

		// 对于使用 SSL 方式的流对象，需要将 SSL IO 流对象注册至网络
		// 连接流对象中，即用 ssl io 替换 stream 中默认的底层 IO 过程
		if (conn_->setup_hook(ssl) == ssl) {
			printf("setup ssl IO hook error!\r\n");
			ssl->destroy();
			return false;
		}

		return true;
	}

	void do_echo(void) {
		char buf[4096];

		while (true) {
			int ret = conn_->read(buf, sizeof(buf), false);
			if (ret == -1) {
				break;
			}
			if (conn_->write(buf, ret) == -1) {
				break;
			}
		}
	}
};

static void start_server(const acl::string addr, acl::sslbase_conf& ssl_conf) {
	acl::server_socket ss;
	if (!ss.open(addr)) {
		printf("listen %s error %s\r\n", addr.c_str(), acl::last_serror());
		return;
	}

	while (true) {
		acl::socket_stream* conn = ss.accept();
		if (conn == NULL) {
			printf("accept error %s\r\n", acl::last_serror());
			break;
		}
		acl::thread* thr = new echo_thread(ssl_conf, conn);
		thr->set_detachable(true);
		thr->start();
	}
}

static bool ssl_init(const acl::string& ssl_crt, const acl::string& ssl_key,
	acl::mbedtls_conf& ssl_conf) {

	ssl_conf.enable_cache(true);

	// 加载 SSL 证书
	if (!ssl_conf.add_cert(ssl_crt)) {
		printf("add ssl crt=%s error\r\n", ssl_crt.c_str());
		return false;
	}

	// 设置 SSL 证书私钥
	if (!ssl_conf.set_key(ssl_key)) {
		printf("set ssl key=%s error\r\n", ssl_key.c_str());
		return false;
	}

	return true;
}

static void usage(const char* procname) {
	printf("usage: %s -h [help]\r\n"
		" -s listen_addr\r\n"
		" -L ssl_libs_path\r\n"
		" -c ssl_crt\r\n"
		" -k ssl_key\r\n"
		, procname);
}

int main(int argc, char* argv[]) {
	acl::string addr = "0.0.0.0|2443";
#if defined(__APPLE__)
	acl::string ssl_lib = "../libmbedtls.dylib";
#elif defined(__linux__)
	acl::string ssl_lib = "../libmbedtls.so";
#elif defined(_WIN32) || defined(_WIN64)
	acl::string ssl_path = "../mbedtls.dll";

	acl::acl_cpp_init();
#else
# error "unknown OS type"
#endif
	acl::string ssl_crt = "../ssl_crt.pem", ssl_key = "../ssl_key.pem";

	int ch;
	while ((ch = getopt(argc, argv, "hs:L:c:k:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'L':
			ssl_lib = optarg;
			break;
		case 'c':
			ssl_crt = optarg;
			break;
		case 'k':
			ssl_key = optarg;
			break;
		default:
			break;
		}
	}

	acl::log::stdout_open(true);

	// 设置 MbedTLS 动态库路径
	const std::vector<acl::string>& libs = ssl_lib.split2(",; \t");
	if (libs.size() == 1) {
		acl::mbedtls_conf::set_libpath(libs[0]);
	} else if (libs.size() == 3) {
		// libcrypto, libx509, libssl);
		acl::mbedtls_conf::set_libpath(libs[0], libs[1], libs[2]);
	} else {
		printf("invalid ssl_lib=%s\r\n", ssl_lib.c_str());
		return 1;
	}

	// 加载 MbedTLS 动态库
	if (!acl::mbedtls_conf::load()) {
		printf("load %s error\r\n", ssl_lib.c_str());
		return 1;
	}

	// 初始化服务端模式下的全局 SSL 配置对象
	bool server_side = true;

	// SSL 证书校验级别
	acl::mbedtls_verify_t verify_mode = acl::MBEDTLS_VERIFY_NONE;

	acl::mbedtls_conf ssl_conf(server_side, verify_mode);

	if (!ssl_init(ssl_crt, ssl_key, ssl_conf)) {
		printf("ssl_init failed\r\n");
		return 1;
	}

	start_server(addr, ssl_conf);
	return 0;
}
