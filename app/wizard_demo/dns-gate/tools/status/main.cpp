#include "stdafx.h"
#include "http_status.h"

static void usage(const char* procname) {
	printf("usage: %s -h[help]\r\n"
		" -s server_address\r\n"
		" -u username\r\n"
		" -p password\r\n"
		" -C path_to_crypto\r\n"
		" -S path_to_ssl\r\n"
		" -L logfile\r\n"
		, procname);
}

int main(int argc, char* argv[]) {
	acl::string addr("192.168.1.65:443");
	int ch;
	acl::sslbase_conf* ssl_conf;
	acl::string crypto_path = "/usr/local/lib/libcrypto.dylib";
	acl::string ssl_path = "/usr/local/lib/libssl.dylib";
	acl::string user, pass, logfile;

	while ((ch = getopt(argc, argv, "hs:C:S:u:p:L:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'C':
			crypto_path = optarg;
			break;
		case 'S':
			ssl_path = optarg;
			break;
		case 'u':
			user = optarg;
			break;
		case 'p':
			pass = optarg;
			break;
		case 'L':
			logfile = optarg;
			break;
		default:
			break;
		}
	}

	if (user.empty() || pass.empty()) {
		usage(argv[0]);
		printf("\r\nusername or password empty!\r\n\r\n");
		return 1;
	}

	if (logfile.empty()) {
		acl::log::stdout_open(true);
	} else {
		acl::log::open(logfile, "status");
	}

	acl::openssl_conf::set_libpath(crypto_path, ssl_path);

	if (!acl::openssl_conf::load()) {
		printf("load ssl error, crypto=%s, ssl=%s\r\n",
			crypto_path.c_str(), ssl_path.c_str());
		return 1;
	}

	ssl_conf = new acl::openssl_conf(false);
	http_status status(addr, *ssl_conf, user, pass);
	status.start();

	printf("OVER!\n");

	delete ssl_conf;

	return 0;
}
