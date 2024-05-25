// main.cpp : 定义控制台应用程序的入口点。
//
#include "stdafx.h"
#include <assert.h>
#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"

class echo_thread : public acl::thread {
public:
	echo_thread(acl::sslbase_conf& ssl_conf, const char* addr, int count)
	: ssl_conf_(ssl_conf), addr_(addr), count_(count) {}

	~echo_thread(void) {}

private:
	acl::sslbase_conf&  ssl_conf_;
	acl::string addr_;
	int count_;

private:
	// @override
	void* run(void) {
		acl::socket_stream conn;
		conn.set_rw_timeout(60);
		if (!conn.open(addr_, 10, 10)) {
			printf("connect %s error %s\r\n",
				addr_.c_str(), acl::last_serror());
			return NULL;
		}

		// 给 socket 安装 SSL IO 过程
		if (!setup_ssl(conn)) {
			return NULL;
		}

		do_echo(conn);
		return NULL;
	}

	bool setup_ssl(acl::socket_stream& conn) {
		bool non_block = false;
		acl::sslbase_io* ssl = ssl_conf_.create(non_block);

		ssl->set_sni_host(addr_);

		// 对于使用 SSL 方式的流对象，需要将 SSL IO 流对象注册至网络
		// 连接流对象中，即用 ssl io 替换 stream 中默认的底层 IO 过程
		if (conn.setup_hook(ssl) == ssl) {
			printf("setup ssl IO hook error!\r\n");
			ssl->destroy();
			return false;
		}

		printf("ssl setup ok!\r\n");
		return true;
	}

	void do_echo(acl::socket_stream& conn) {
		const char* data = "hello world!\r\n";
		int i;
		for (i = 0; i < count_; i++) {
			//sleep(8);
			if (conn.write(data, strlen(data)) == -1) {
				break;
			}

			char buf[4096];
			int ret = conn.read(buf, sizeof(buf) - 1, false);
			if (ret == -1) {
				printf("read over, count=%d\r\n", i + 1);
				break;
			}
			buf[ret] = 0;
			if (i == 0) {
				printf("read: %s", buf);
			}
		}
		printf("thread-%lu: count=%d\n", acl::thread::self(), i);
	}
};

static void start_clients(acl::sslbase_conf& ssl_conf, const acl::string addr,
	int cocurrent, int count) {

	std::vector<acl::thread*> threads;
	for (int i = 0; i < cocurrent; i++) {
		acl::thread* thr = new echo_thread(ssl_conf, addr, count);
		threads.push_back(thr);
		thr->start();
	}

	for (std::vector<acl::thread*>::iterator it = threads.begin();
		it != threads.end(); ++it) {
		(*it)->wait(NULL);
		delete *it;
	}
}

static acl::sslbase_conf* load_mbedtls(acl::string& ssl_libs)
{
	if (ssl_libs.empty()) {
#if defined(__APPLE__)
		ssl_libs = "../libmbedtls.dylib";
#elif defined(__linux__)
		ssl_libs = "../libmbedtls.so";
#elif defined(_WIN32) || defined(_WIN64)
		ssl_libs = "../mbedtls.dll";
#else
# error "unknown OS type"
#endif
	}

	// 设置 MbedTLS 动态库路径
	const std::vector<acl::string>& libs = ssl_libs.split2(",; \t");
	if (libs.size() == 1) {
		acl::mbedtls_conf::set_libpath(libs[0]);
	} else if (libs.size() == 3) {
		// libcrypto, libx509, libssl);
		acl::mbedtls_conf::set_libpath(libs[0], libs[1], libs[2]);
	} else {
		printf("invalid ssl_lib=%s\r\n", ssl_libs.c_str());
		return NULL;
	}

	// 加载 MbedTLS 动态库
	if (!acl::mbedtls_conf::load()) {
		printf("load %s error\r\n", ssl_libs.c_str());
		return NULL;
	}

	// 初始化服务端模式下的全局 SSL 配置对象
	bool server_side = false;

	// SSL 证书校验级别
	acl::mbedtls_verify_t verify_mode = acl::MBEDTLS_VERIFY_NONE;

	return new acl::mbedtls_conf(server_side, verify_mode);
}

static acl::sslbase_conf* load_openssl(acl::string& ssl_libs)
{
#if defined(__APPLE__)
	acl::string libcrypto = "/usr/local/lib/libcrypto.dylib";
	acl::string libssl    = "/usr/local/lib/libssl.dylib";
#elif defined(__linux__)
	acl::string libcrypto = "/usr/local/lib64/libcrypto.so";
	acl::string libssl    = "/usr/local/lib64/libssl.so";
#else
# error "Unsupport OS!"
#endif
	if (!ssl_libs.empty()) {
		const std::vector<acl::string>& libs = ssl_libs.split2(",; \t");
		if (libs.size() >= 2) {
			libcrypto = libs[0];
			libssl = libs[1];
		} else {
			libssl = libs[0];
		}
	}

	// 设置 OpenSSL 动态库的加载路径
	acl::openssl_conf::set_libpath(libcrypto, libssl);

	// 动态加载 OpenSSL 动态库
	if (!acl::openssl_conf::load()) {
		printf("load ssl error=%s, crypto=%s, ssl=%s\r\n",
				acl::last_serror(), libcrypto.c_str(), libssl.c_str());
		return NULL;
	}

	// 初始化客户端模式下的全局 SSL 配置对象
	bool server_side = false;
	return new acl::openssl_conf(server_side);
}

static void usage(const char* procname) {
	printf("usage: %s -h [help]\r\n"
		" -s listen_addr\r\n"
		" -t ssl_type[default: openssl]\r\n"
		" -l ssl_libs_path[default: /usr/local/lib64/libcrypto.so;/usr/local/lib64/libssl.so]\r\n"
		" -c cocurrent[default: 10]\r\n"
		" -n count[default: 10]\r\n"
		, procname);
}

int main(int argc, char* argv[]) {
	acl::string addr = "0.0.0.0|2443";
	acl::string ssl_libs, ssl_type = "openssl";
	int ch, cocurrent = 10, count = 10;

	while ((ch = getopt(argc, argv, "hs:l:c:n:t:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'l':
			ssl_libs = optarg;
			break;
		case 'c':
			cocurrent = atoi(optarg);
			break;
		case 'n':
			count = atoi(optarg);
			break;
		case 't':
			ssl_type = optarg;
			break;
		default:
			break;
		}
	}

	acl::log::stdout_open(true);

	acl::sslbase_conf* ssl_conf;

	if (ssl_type == "mbedtls") {
		ssl_conf = load_mbedtls(ssl_libs);
	} else if (ssl_type == "openssl") {
		ssl_conf = load_openssl(ssl_libs);
	} else {
		printf("Not support ssl type=%s\r\n", ssl_type.c_str());
		return 1;
	}

	if (!ssl_conf) {
		printf("Load ssl error, libs=%s\r\n", ssl_libs.c_str());
		return 1;
	}

	start_clients(*ssl_conf, addr, cocurrent, count);
	return 0;
}
