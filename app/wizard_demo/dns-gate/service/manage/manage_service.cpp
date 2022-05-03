#include "stdafx.h"
#include "acl_cpp/serialize/serialize.hpp"
#include "rules/rules_option.h"
#include "http_servlet.h"
#include "http_service.h"
#include "manage_service.h"

static http_service __service;
static acl::server_socket __server(acl::OPEN_FLAG_REUSEPORT, 128);

static bool do_none(HttpResponse& res) {
	acl::string buf;
	buf = "None!\r\n";
	res.setContentLength(buf.size());
	return res.write(buf);
}

static bool do_list(HttpRequest&, HttpResponse& res) {
	if (var_rules_option == NULL) {
		return do_none(res);
	}

	acl::string buf;
	filter_rules rules;
	var_rules_option->get_rules(rules);
	acl::serialize<filter_rules>(rules, buf);
	res.setContentLength(buf.size());
	res.setContentType("application/json");
	return res.write(buf);
}

static bool do_remove(HttpRequest& req, HttpResponse& res) {
	if (var_rules_option == NULL) {
		return do_none(res);
	}

	acl::string buf;
	const char* name = req.getParameter("name");
	if (name == NULL || *name == 0) {
		logger_error("no name");
		buf = "No name";
		res.setContentLength(buf.size());
		return res.write(buf);
	}


	var_rules_option->remove(name);
	buf = "OK!\r\n";
	res.setContentLength(buf.size());
	return res.write(buf);
}

static bool do_add(HttpRequest& req, HttpResponse& res) {
	if (var_rules_option == NULL) {
		return do_none(res);
	}

	acl::string buf;
	const char* name = req.getParameter("name");
	if (name == NULL || *name == 0) {
		logger_error("no name");
		buf = "No name";
		res.setContentLength(buf.size());
		return res.write(buf);
	}

	const char* ptr = req.getParameter("from_hour");
	if (ptr == NULL || *ptr == 0) {
		logger_error("no from_hour");
		buf = "No from_hour";
		res.setContentLength(buf.size());
		return res.write(buf);
	}
	int from_hour = atoi(ptr);

	int from_min;
	ptr = req.getParameter("from_min");
	if (ptr && *ptr) {
		from_min = atoi(ptr);
	} else {
		from_min = 0;
	}

	ptr = req.getParameter("to_hour");
	if (ptr == NULL || *ptr == 0) {
		logger_error("no to_hour");
		buf = "No to_hour";
		res.setContentLength(buf.size());
		return res.write(buf);
	}
	int to_hour = atoi(ptr);

	int to_min;
	ptr = req.getParameter("to_min");
	if (ptr && *ptr) {
		to_min = atoi(ptr);
	} else {
		to_min = 0;
	}

	var_rules_option->add(name, from_hour, from_min, to_hour, to_min);
	buf = "OK!\r\n";
	res.setContentLength(buf.size());
	return res.write(buf);
}

static bool do_reload(HttpRequest&, HttpResponse& res) {
	if (var_rules_option == NULL) {
		return do_none(res);
	}

	bool ret = var_rules_option->reload(var_cfg_rules_file);
	acl::string buf = ret ? "OK!\r\n" : "Error!\r\n";
	res.setContentLength(buf.size());
	return res.write(buf);
}

static void handle_client(acl::socket_stream& conn) {
	acl::memcache_session session("127.0.0.1:11211");
	http_servlet servlet(__service, &conn, &session);

	servlet.setLocalCharset("utf-8");

	while(servlet.doRun()) {}
}

void manage_service_start(const char* addr) {
	if (!__server.open(addr)) {
		logger_error("bind %s error %s", addr, acl::last_serror());
		return;
	}

	__service.Get("/list", [](HttpRequest& req, HttpResponse& res) {
		return do_list(req, res);
	}).Get("/remove", [](HttpRequest& req, HttpResponse& res) {
		return do_remove(req, res);
	}).Get("/add", [](HttpRequest& req, HttpResponse& res) {
		return do_add(req, res);
	}).Get("/reload", [](HttpRequest& req, HttpResponse& res) {
		return do_reload(req, res);
	/*
	}).Get("/restart", [](HttpRequest&, HttpResponse& res) {
		acl::string buf("OK!\r\n");
		res.setContentLength(buf.size());
		res.write(buf);
		exit (0);
		return false;
	*/
	}).Default([](const char*, HttpRequest&, HttpResponse& res) {
		acl::string buf("OK!\r\n");
		res.setContentLength(buf.size());
		return res.write(buf);
	});

	while (true) {
		acl::socket_stream* conn = __server.accept();
		if (conn == NULL) {
			logger_error("accept error %s", acl::last_serror());
			break;
		}

		go[=] {
			handle_client(*conn);
			delete conn;
		};
	}
}
