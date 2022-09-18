#include "stdafx.h"
#include "user_login.h"

user_login::user_login(const char* addr, acl::sslbase_conf& ssl_conf,
	const char* user, const char* pass)
: addr_(addr)
, ssl_conf_(ssl_conf)
, user_(user)
, pass_(pass)
{
}

user_login::~user_login(void) {}

bool user_login::start(acl::string& stok) {
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
		logger_error("send login request error, req=%s", buf.c_str());
		return false;
	}

	acl::json json;
	if (!conn.get_body(json)) {
		logger_error("get login res error");
		return false;
	}

	//////////////////////////////////////////////////////////////////////

	acl::http_client* client = conn.get_client();
	client->print_header("--response--");

	//////////////////////////////////////////////////////////////////////

	login_res_t res;
	acl::string err;
	if (!acl::deserialize<login_res_t>(json, res, &err)) {
		logger_error("parse login res error=%s", err.c_str());
		return false;
	}

	logger("login ok, stok=%s", res.stok.c_str());
	stok = res.stok;
	return true;
}
