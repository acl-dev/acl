#include <getopt.h>
#include <cstdlib>
#include <cstdio>

#include "acl_cpp/lib_acl.hpp"		// must before http_server.hpp
#include "fiber/http_server.hpp"

static char *var_cfg_debug_msg;

static acl::master_str_tbl var_conf_str_tab[] = {
	{ "debug_msg", "test_msg", &var_cfg_debug_msg },

	{ 0, 0, 0 }
};

static int  var_cfg_debug_enable;

static acl::master_bool_tbl var_conf_bool_tab[] = {
	{ "debug_enable", 1, &var_cfg_debug_enable },

	{ 0, 0, 0 }
};

static int  var_cfg_io_timeout;

static acl::master_int_tbl var_conf_int_tab[] = {
	{ "io_timeout", 120, &var_cfg_io_timeout, 0, 0 },

	{ 0, 0 , 0 , 0, 0 }
};

static bool http_test(acl::HttpRequest&, acl::HttpResponse& res) {
	acl::string buf("hello test!\r\n");
	res.setContentLength(buf.size());
	return res.write(buf);
}

int main(int argc, char *argv[]) {
	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	acl::http_server server;

	// set the configure variables

	server.set_cfg_int(var_conf_int_tab)
		.set_cfg_int64(NULL)
		.set_cfg_str(var_conf_str_tab)
		.set_cfg_bool(var_conf_bool_tab);

	// set process running status callback

	server.before_proc_jail([] {
		printf("---> before process jail\r\n");
	}).on_proc_init([] {
		printf("---> after process init:\r\n");
		printf("\tdebug_msg=%s\r\n", var_cfg_debug_msg);
		printf("\tdebug_enable=%d\r\n", var_cfg_debug_enable);
		printf("\tio_timeout=%d\r\n", var_cfg_io_timeout);
	}).on_proc_exit([] {
		printf("---> before process exit\r\n");
	}).on_proc_sighup([] (acl::string& s) {
		s = "+ok";
		printf("---> process got sighup\r\n");
		return true;
	}).on_thread_init([] {
		printf("---> thread-%lu on init\r\n", acl::thread::self());
	}).on_thread_accept([] (acl::socket_stream& conn) {
		printf("---> thread-%lu on accept %d\r\n",
			acl::thread::self(), conn.sock_handle());
		return true;
	});

	// set http route

	int i = 0;
	server.Get("/", [](acl::HttpRequest&, acl::HttpResponse& res) {
		acl::string buf("hello world1!\r\n");
		res.setContentLength(buf.size());
		return res.write(buf.c_str(), buf.size());
	}).Get("/hello", [](acl::HttpRequest&, acl::HttpResponse& res) {
		acl::string buf("hello world2!\r\n");
		res.setContentLength(buf.size());
		return res.write(buf.c_str(), buf.size());
	}).Post("/ok", [](acl::HttpRequest& req, acl::HttpResponse& res) {
		acl::string buf;
		req.getBody(buf);
		res.setContentLength(buf.size());
		return res.write(buf.c_str(), buf.size());
	}).Get("/json", [=](acl::HttpRequest&, acl::HttpResponse& res) {
		acl::json json;
		acl::json_node& root = json.get_root();
		root.add_number("code", 200)
			.add_text("status", "+ok")
			.add_child("data",
				json.create_node()
					.add_text("name", "value")
					.add_bool("success", true)
					.add_number("number", i));
		return res.write(json);
	}).Get("/test", http_test);

	// start the server in alone or daemon mode

	if (argc >= 2 && strcasecmp(argv[1], "alone") == 0) {
		const char* addr = "0.0.0.0|8194";
		const char* conf = argc >= 3 ? argv[2] : NULL;

		// run in alone mode
		server.run_alone(addr, conf);
	} else {
		// run in daemon mode
		server.run_daemon(argc, argv);
	}

	return 0;
}
