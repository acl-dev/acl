#include "stdafx.h"

static acl::string __keypre("test_key");

static bool test_set(acl::redis& cmd, int n)
{
	acl::string key, val;

	for (int i = 0; i < n; i++) {
		key.format("%s_%d", __keypre.c_str(), i);
		val.format("val_%s", key.c_str());
		cmd.clear();
		if (cmd.set(key, val) == false) {
			printf("set key: %s error: %s\r\n", key.c_str(),
				cmd.result_error());
			return false;
		} else if (i < 10)
			printf("set key: %s ok\r\n", key.c_str());
	}

	return true;
}

static bool test_get(acl::redis& cmd, int n)
{
	acl::string key, val;

	for (int i = 0; i < n; i++) {
		key.format("%s_%d", __keypre.c_str(), i);
		cmd.clear();
		val.clear();
		if (cmd.get(key, val) == false) {
			printf("get key: %s error: %s\r\n", key.c_str(),
				cmd.result_error());
			return false;
		} else if (i < 10)
			printf("get key: %s ok, val: %s\r\n", key.c_str(),
				val.c_str());
	}

	return true;
}

static bool test_del(acl::redis& cmd, int n)
{
	acl::string key;

	for (int i = 0; i < n; i++) {
		key.format("%s_%d", __keypre.c_str(), i);
		cmd.clear();
		int ret = cmd.del_one(key.c_str());
		if (ret < 0) {
			printf("del key: %s error: %s\r\n", key.c_str(),
				cmd.result_error());
			return false;
		} else if (i < 10)
			printf("del ok, key: %s\r\n", key.c_str());
	}

	return true;
}

static bool test_expire(acl::redis& cmd, int n)
{
	acl::string key;

	for (int i = 0; i < n; i++) {
		key.format("%s_%d", __keypre.c_str(), i);
		cmd.clear();
		if (cmd.expire(key.c_str(), 100) < 0) {
			printf("expire key: %s error: %s\r\n", key.c_str(),
				cmd.result_error());
			return false;
		} else if (i < 10)
			printf("expire ok, key: %s\r\n", key.c_str());
	}

	return true;
}

static bool test_ttl(acl::redis& cmd, int n)
{
	acl::string key;
	int ttl;

	for (int i = 0; i < n; i++) {
		key.format("%s_%d", __keypre.c_str(), i);
		cmd.clear();
		if ((ttl = cmd.ttl(key.c_str())) < 0) {
			printf("get ttl key: %s error: %s\r\n", key.c_str(),
				cmd.result_error());
			return false;
		} else if (i < 10)
			printf("ttl ok, key: %s, ttl: %d\r\n",
				key.c_str(), ttl);
	}

	return true;
}

static bool test_exists(acl::redis& cmd, int n)
{
	acl::string key;

	for (int i = 0; i < n; i++) {
		key.format("%s_%d", __keypre.c_str(), i);
		cmd.clear();
		if (cmd.exists(key.c_str()) == false) {
			if (i < 10)
				printf("no exists key: %s\r\n", key.c_str());
		} else if (i < 10)
			printf("exists key: %s\r\n", key.c_str());
	}

	return true;
}

static bool test_type(acl::redis& cmd, int n)
{
	acl::string key;

	for (int i = 0; i < n; i++) {
		key.format("%s_%d", __keypre.c_str(), i);
		cmd.clear();
		acl::redis_key_t ret = cmd.type(key.c_str());
		if (ret == acl::REDIS_KEY_NONE) {
			printf("unknown type key: %s error: %s\r\n",
				key.c_str(), cmd.result_error());
			return false;
		} else if (i < 10)
			printf("type ok, key: %s, ret: %d\r\n",
				key.c_str(), ret);
	}

	return true;
}

static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n"
		"-s redis_addr[127.0.0.1:6379]\r\n"
		"-L path_to_libpolarssl.so [if be set ssl will be used, default: false]\r\n"
		"-c [if using redis cluster mode, default: false]\r\n"
		"-p password[default: \"\"]\r\n"
		"-d dbnum[default: 0]\r\n"
		"-n count\r\n"
		"-t connect_timeout and rw_timeout[default: 10]\r\n"
		"-a cmd[set|del|expire|ttl|exists|type|all]\r\n",
		procname);
	printf("sample:\r\n\r\n"
		"%s -s 127.0.0.1:6379 -c -L ./libpolarssl.so -a set\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	int  ch, n = 1, conn_timeout = 10, rw_timeout = 10, dbnum = 0;
	acl::string addr("127.0.0.1:6379"), command, passwd;
	acl::string ssl_libpath, ca_file, crt_file, key_file;
	bool cluster_mode = false;

	while ((ch = getopt(argc, argv, "hs:n:t:a:p:d:L:cC:S:K:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'n':
			n = atoi(optarg);
			break;
		case 't':
			conn_timeout = atoi(optarg);
			rw_timeout = atoi(optarg);
			break;
		case 'a':
			command = optarg;
			break;
		case 'p':
			passwd = optarg;
			break;
		case 'd':
			dbnum = atoi(optarg);
			break;
		case 'L':
			ssl_libpath = optarg;
			break;
		case 'c':
			cluster_mode = true;
			break;
		case 'C':
			ca_file = optarg;
			break;
		case 'S':
			crt_file = optarg;
			break;
		case 'K':
			key_file = optarg;
			break;
		default:
			break;
		}
	}

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	acl::redis cmd;

	acl::redis_client client(addr.c_str(), conn_timeout, rw_timeout);
	client.set_password(passwd);
	if (dbnum > 0)
		client.set_db(dbnum);

	acl::redis_client_cluster cluster;
	cluster.set(addr.c_str(), 0, conn_timeout, rw_timeout);

	acl::sslbase_conf* ssl_conf = NULL; // the global variable.

	//////////////////////////////////////////////////////////////////////
	// load ssl library dynamically

	if (ssl_libpath.find("mbedtls") != NULL) {
		acl::mbedtls_conf::set_libpath(ssl_libpath);

		// load libpolarssl.so dynamically
		if (!acl::mbedtls_conf::load()) {
			printf("load %s error %s\r\n", ssl_libpath.c_str(),
				acl::last_serror());
			return 1;
		} else {
			ssl_conf = new acl::mbedtls_conf(false);
		}
	} else if (ssl_libpath.find("polarssl") != NULL) {
		acl::polarssl_conf::set_libpath(ssl_libpath);

		// load libpolarssl.so dynamically
		if (!acl::polarssl_conf::load()) {
			printf("load %s error %s\r\n", ssl_libpath.c_str(),
				acl::last_serror());
			return 1;
		} else {
			ssl_conf = new acl::polarssl_conf(false);
		}
	} else {
		printf("invalid ssl path=%s\r\n", ssl_libpath.c_str());
	}


	//////////////////////////////////////////////////////////////////////

	// load ssl cert if needed

	if (ssl_conf && !ca_file.empty() && !crt_file.empty()
		&& !key_file.empty()) {
		if (!ssl_conf->load_ca(ca_file, NULL)) {
			printf("load %s error\r\n", ca_file.c_str());
			return 1;
		}
		if (!ssl_conf->add_cert(crt_file)) {
			printf("add cert %s error\r\n", crt_file.c_str());
			return 1;
		}
		if (!ssl_conf->set_key(key_file)) {
			printf("set key %s error\r\n", key_file.c_str());
			return 1;
		}
	}

	// set redis communication in ssl mode
	if (ssl_conf) {
		// bind polarssl or mbedtls
		client.set_ssl_conf(ssl_conf);
		cluster.set_ssl_conf(ssl_conf);

		printf("SSL communication will be used\r\n");
	} else {
		printf("no SSL\r\n");
	}

	//////////////////////////////////////////////////////////////////////

	if (cluster_mode) {
		cmd.set_cluster(&cluster);
	} else  {
		cmd.set_client(&client);
	}

	bool ret;

	if (command == "set") {
		ret = test_set(cmd, n);
	} else if (command == "get") {
		ret = test_get(cmd, n);
	} else if (command == "del") {
		ret = test_del(cmd, n);
	} else if (command == "expire") {
		ret = test_expire(cmd, n);
	} else if (command == "ttl") {
		ret = test_ttl(cmd, n);
	} else if (command == "exists") {
		ret = test_exists(cmd, n);
	} else if (command == "type") {
		ret = test_type(cmd, n);
	} else if (command == "all") {
		ret = test_set(cmd, n)
			&& test_get(cmd, n)
			&& test_expire(cmd, n)
			&& test_ttl(cmd, n)
			&& test_exists(cmd, n)
			&& test_type(cmd, n)
			&& test_del(cmd, n);
	} else {
		ret = false;
		printf("unknown cmd: %s\r\n", command.c_str());
	}

	if (ret == true) {
		printf("test OK!\r\n");
	} else {
		printf("test failed!\r\n");
	}

#ifdef WIN32
	printf("enter any key to exit\r\n");
	getchar();
#endif
	delete ssl_conf;
	return 0;
}
