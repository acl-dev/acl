#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>

#define	 STACK_SIZE	128000

static int __rw_timeout = 5;
static acl::string __ssl_crt("./ssl_crt.pem");
static acl::string __ssl_key("./ssl_key.pem");
static acl::sslbase_conf *__ssl_conf = NULL;
static bool __check_ssl = false;

static void echo_fiber(ACL_FIBER *, void *ctx)
{
	acl::socket_stream *conn = (acl::socket_stream *) ctx;

	printf("start one server\r\n");

	if (__ssl_conf != NULL) {
		acl::sslbase_io* ssl = __ssl_conf->create(false);

		if (conn->setup_hook(ssl) == ssl) {
			printf("setup_hook error: %s\r\n", acl::last_serror());
			ssl->destroy();
			delete conn;
			return;
		}

		if (__check_ssl) {
			if (!ssl->handshake()) {
				printf("ssl handshake error: %s\r\n", acl::last_serror());
				delete conn;
				return;
			}
			if (!ssl->handshake_ok()) {
				printf("ssl handshake error: %s\r\n", acl::last_serror());
				delete conn;
				return;
			}
			printf(">>>check ssl ok<<<\n");
		}
	}

	printf("At last, ssl handshake_ok\r\n");

	char buf[2048];
	size_t n = 0, eagain = 0;

	while (true) {
		int ret = conn->read(buf, sizeof(buf) - 1, false);
		if (ret < 0) {
			if (errno != EAGAIN) {
				printf(">>>read error: %s\r\n", acl::last_serror());
				break;
			}

			printf(">>>read timeout %zd times\r\n", ++eagain);
			if (eagain >= 3) {
				break;
			}
			continue;
		}

		buf[ret] = 0;

		if (n++ < 5) {
			printf(">>>read cnt=%d, data=%s\r\n", ret, buf);
		}

		if (conn->write(buf, ret) == -1) {
			printf("write error: %s\r\n", acl::last_serror());
			break;
		}
	}

	printf("close one connection: %d, %s\r\n", conn->sock_handle(),
		acl::last_serror());
	delete conn;
}

enum ssl_type_t {
	ssl_type_openssl,
	ssl_type_mbedtls,
	ssl_type_polarssl,
};

static acl::sslbase_conf* ssl_init(ssl_type_t type, const acl::string& crt,
	const acl::string& key)
{
	acl::sslbase_conf* conf;
	switch (type) {
	case ssl_type_openssl:
		conf = new acl::openssl_conf(true);
		break;
	case ssl_type_mbedtls:
		conf = new acl::mbedtls_conf(true);
		break;
	case ssl_type_polarssl:
		conf = new acl::polarssl_conf;
		break;
	default:
		printf("Unknown ssl type=%d\r\n", (int) type);
		return NULL;
	}

	conf->enable_cache(1);

	if (!conf->add_cert(crt)) {
		printf("load %s error\r\n", crt.c_str());
		delete conf;
		return NULL;
	}

	if (!conf->set_key(key)) {
		printf("set_key %s error\r\n", key.c_str());
		delete conf;
		return NULL;
	}

	printf(">>> ssl_init ok, ssl_crt: %s, ssl_key: %s\r\n",
		crt.c_str(), key.c_str());

	return conf;
}

static void fiber_accept(ACL_FIBER *, void *ctx)
{
	const char* addr = (const char* ) ctx;
	acl::server_socket server;

	if (!server.open(addr)) {
		printf("open %s error\r\n", addr);
		return;
	}

	printf(">>> listen %s ok\r\n", addr);

	while (true) {
		acl::socket_stream* client = server.accept();
		if (client == NULL) {
			printf("accept failed: %s\r\n", acl::last_serror());
			break;
		}

		client->set_rw_timeout(__rw_timeout);
		printf("accept one: %d\r\n", client->sock_handle());
		acl_fiber_create(echo_fiber, client, STACK_SIZE);
	}
}

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		" -l path_to_ssl\r\n"
		" -s listen_addr\r\n"
		" -r rw_timeout\r\n"
		" -c ssl_crt.pem\r\n"
		" -C [if check ssl status]\r\n"
		" -k ssl_key.pem\r\n", procname);
}

int main(int argc, char *argv[])
{
	acl::string addr(":9001");
#if defined(__APPLE__)
	acl::string libpath("../libmbedtls_all.dylib");
#else
	acl::string libpath("../libmbedtls_all.so");
#endif
	int  ch;

	while ((ch = getopt(argc, argv, "hl:s:r:c:k:C")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'l':
			libpath = optarg;
			break;
		case 's':
			addr = optarg;
			break;
		case 'r':
			__rw_timeout = atoi(optarg);
			break;
		case 'c':
			__ssl_crt = optarg;
			break;
		case 'k':
			__ssl_key = optarg;
			break;
		case 'C':
			__check_ssl = true;
			break;
		default:
			break;
		}
	}

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	if (libpath.find("mbedtls") != NULL) {
		const std::vector<acl::string>& libs = libpath.split2("; \t");
		if (libs.size() != 3) {
			printf("invalid libpath=%s\r\n", libpath.c_str());
			return 1;
		}
		acl::mbedtls_conf::set_libpath(libs[0], libs[1], libs[2]);
		if (!acl::mbedtls_conf::load()) {
			printf("load %s error\r\n", libpath.c_str());
			return 1;
		}
		__ssl_conf = ssl_init(ssl_type_mbedtls, __ssl_crt, __ssl_key);
	} else if (libpath.find("polarssl") != NULL) {
		acl::polarssl_conf::set_libpath(libpath);
		if (!acl::polarssl_conf::load()) {
			printf("load %s error\r\n", libpath.c_str());
			return 1;
		}
		__ssl_conf = ssl_init(ssl_type_polarssl, __ssl_crt, __ssl_key);
	} else if (libpath.find("crypto") != NULL) {
		const std::vector<acl::string>& libs = libpath.split2("; \t");
		if (libs.size() != 2) {
			printf("invalid libpath=%s\r\n", libpath.c_str());
			return 1;
		}

		printf(">>>cryp=%s, ssl=%s\n", libs[0].c_str(), libs[1].c_str());
		acl::openssl_conf::set_libpath(libs[0], libs[1]);
		if (!acl::openssl_conf::load()) {
			printf("load %s error\r\n", libpath.c_str());
			return 1;
		}
		__ssl_conf = ssl_init(ssl_type_openssl, __ssl_crt, __ssl_key);
		((acl::openssl_conf*) __ssl_conf)->use_sockopt_timeout(true);
	} else {
		printf("invalid libpath=%s\r\n", libpath.c_str());
		return 1;
	}

	if (__ssl_conf == NULL) {
		printf("create sslbase_conf error\r\n");
		return 1;
	}

	acl_fiber_create(fiber_accept, addr.c_str(), STACK_SIZE);

	acl_fiber_schedule();
	delete __ssl_conf;
	return 0;
}
