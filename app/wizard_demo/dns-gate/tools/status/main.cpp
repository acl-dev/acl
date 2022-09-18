#include "stdafx.h"
#include "user_login.h"
#include "user_status.h"
#include "limit_speed.h"

static void do_status(const char* addr, acl::sslbase_conf& ssl_conf,
	const char* user, const char* pass) {

	while (true) {
		acl::string stok;
		user_login login(addr, ssl_conf, user, pass);
		if (!login.start(stok)) {
			logger_error("login failed, user=%s, pass=%s",
				user, pass);
			sleep(1);
			continue;
		}

		user_status status(addr, ssl_conf, stok);
		status.start();
		sleep(1);
	}
}

static void do_limit_speed(const char* addr, acl::sslbase_conf& ssl_conf,
	const char* hostname, const char* user, const char* pass,
	const char* mac, const char* ip, int up, int down) {

	acl::string stok;
	user_login login(addr, ssl_conf, user, pass);
	if (!login.start(stok)) {
		logger_error("login failed, user=%s, pass=%s", user, pass);
		return;
	}

	limit_speed limit(addr, ssl_conf, stok, mac, ip);
	limit.set_hostname(hostname);
	limit.start(up, down);
}

static void usage(const char* procname) {
	printf("usage: %s -h[help]\r\n"
		" -s server_address\r\n"
		" -a action[status|limit_speed]\r\n"
		" -U up_limit[default: 15]\r\n"
		" -D down_limit[default: 15]\r\n"
		" -H hostname\r\n"
		" -M mac\r\n"
		" -I ip\r\n"
		" -u username\r\n"
		" -p password\r\n"
		" -C path_to_crypto\r\n"
		" -S path_to_ssl\r\n"
		" -L logfile\r\n"
		, procname);
}

int main(int argc, char* argv[]) {
	acl::string addr("192.168.1.65:443");
	int ch, up = 15, down = 15;
	acl::sslbase_conf* ssl_conf;
	acl::string crypto_path = "/usr/local/lib/libcrypto.dylib";
	acl::string ssl_path = "/usr/local/lib/libssl.dylib";
	acl::string user, pass, logfile, action = "status";
	acl::string mac, ip, hostname = "---";

	while ((ch = getopt(argc, argv, "hs:a:C:S:u:p:L:U:D:M:I:H:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'a':
			action = optarg;
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
		case 'U':
			up = atoi(optarg);
			break;
		case 'D':
			down = atoi(optarg);
			break;
		case 'M':
			mac = optarg;
			break;
		case 'I':
			ip = optarg;
			break;
		case 'H':
			hostname = optarg;
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
		logger_error("load ssl error, crypto=%s, ssl=%s",
			crypto_path.c_str(), ssl_path.c_str());
		return 1;
	}

	ssl_conf = new acl::openssl_conf(false);

	if (action == "status") {
		do_status(addr, *ssl_conf, user, pass);
	} else if (action == "limit_speed") {
		do_limit_speed(addr, *ssl_conf, hostname, user, pass, mac, ip, up, down);
	} else {
		printf("unknown action=%s\r\n", action.c_str());
	}

	printf("OVER!\n");

	delete ssl_conf;

	return 0;
}
