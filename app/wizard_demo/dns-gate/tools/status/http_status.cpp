#include "stdafx.h"
#include "json/struct.h"
#include "json/struct.gson.h"
#include "acl_cpp/serialize/serialize.hpp"
#include "http_status.h"

http_status::http_status(const char* addr, acl::sslbase_conf& ssl_conf,
	const char* user, const char* pass)
: addr_(addr)
, ssl_conf_(ssl_conf)
, user_(user)
, pass_(pass)
{
}

http_status::~http_status(void) {}

void http_status::build_request(acl::string& buf) {
	request_t req;

	acl::serialize<request_t>(req, buf);
}

bool http_status::parse_response(const acl::string& data, response_t& res) {
	acl::json json;
	json.update(data);
	if (!json.finish()) {
		printf("invalid json=%s\r\n", data.c_str());
		return false;
	}

	acl::string err;
	if (!acl::deserialize<response_t>(json, res, &err)) {
		printf("parse json=%s\r\n\r\nerr=%s\r\n", data.c_str(), err.c_str());
		return false;
	}

	return true;
}

void http_status::show_status(const response_t& res) {
	printf("\r\n");
	if (res.host_management.host_info.empty()) {
		printf("host_info empty!\r\n");
	}

	for (auto it : res.host_management.host_info) {
		printf("name=%s\r\n", it.first.c_str());
		printf("hostname=%s, ip=%s, mac=%s, up_limit=%s, down_limit=%s\r\n",
			it.second.hostname.c_str(),
			it.second.ip.c_str(),
			it.second.mac.c_str(),
			it.second.up_limit.c_str(),
			it.second.down_limit.c_str());
	}

	printf("--------------------------------------------------------\r\n");

	//////////////////////////////////////////////////////////////////////

	if (res.wireless.sta_list.empty()) {
		printf("sta_list empty!\r\n");
	}

	for (auto it : res.wireless.sta_list) {
		printf("\r\n");
		printf("name=%s\n", it.first.c_str());
		printf("name=%s, mac=%s, ip=%s, tx_rate=%s, rx_rate=%s\r\n",
			it.second.name.c_str(),
			it.second.mac.c_str(),
			it.second.ip.c_str(),
			it.second.tx_rate.c_str(),
			it.second.rx_rate.c_str());
	}
	printf("========================================================\r\n");
}

bool http_status::get_status(const char* stok) {
	acl::http_request conn(addr_);
	conn.set_ssl(&ssl_conf_);

	acl::string url;
	url << "/stok=" << stok << "/ds";

	acl::string req;
	build_request(req);

	conn.request_header().set_url(url)
		.set_host(addr_)
		.set_content_type("application/json; charset=UTF-8")
		.add_entry("Origin", "https://192.168.1.65/")
		.add_entry("Referer", "https://192.168.1.65/")
		.set_content_length(req.size());

	//printf("url=%s\r\n", url.c_str());
	//printf("req=%s\r\n", req.c_str());

	if (!conn.request(req, req.size())) {
		printf("send request error, req=%s\r\n", req.c_str());
		return false;
	}

	acl::string buf;
	if (!conn.get_body(buf)) {
		printf("get response error\r\n");
		return false;
	}

	//printf("%s\n", buf.c_str());

	response_t res;
	if (!parse_response(buf, res)) {
		printf("parse response error\r\n");
		return false;
	}

	show_status(res);
	return true;
}

bool http_status::login(acl::string& out) {
	login_req_t req;
	req.method = "do";
	req.login.username = user_;
	req.login.password = pass_;

	acl::string buf;
	acl::serialize<login_req_t>(req, buf);

	acl::string url;
	url += "/";

	acl::http_request conn(addr_);
	conn.set_ssl(&ssl_conf_);

	conn.request_header().set_url(url)
		.set_host(addr_)
		.set_content_type("application/json");

	if (!conn.request(buf, buf.size())) {
		printf("send login request error, req=%s\r\n", buf.c_str());
		return false;
	}

	acl::json json;
	if (!conn.get_body(json)) {
		printf("get login res error\r\n");
		return false;
	}

	//////////////////////////////////////////////////////////////////////

	acl::http_client* client = conn.get_client();
	client->print_header("--response--");

	//////////////////////////////////////////////////////////////////////

	login_res_t res;
	acl::string err;
	if (!acl::deserialize<login_res_t>(json, res, &err)) {
		printf("parse login res error=%s\r\n", err.c_str());
		return false;
	}

	printf("login ok, stok=%s\r\n", res.stok.c_str());
	out = res.stok;
	return true;
}

bool http_status::start(void) {
	acl::string stok;
	if (!login(stok)) {
		printf("login error\r\n");
		return false;
	}

	while (true) {
		get_status(stok);
		sleep(1);
	}
	return true;
}
