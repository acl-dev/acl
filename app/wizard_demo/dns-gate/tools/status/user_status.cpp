#include "stdafx.h"
#include "user_status.h"

user_status::user_status(const char* addr, acl::sslbase_conf& ssl_conf,
	const char* stok)
: addr_(addr)
, ssl_conf_(ssl_conf)
, stok_(stok)
{
}

user_status::~user_status(void) {}

void user_status::build_request(acl::string& buf) {
	request_t req;

	acl::serialize<request_t>(req, buf);
}

bool user_status::parse_response(const acl::string& data, response_t& res) {
	acl::json json;
	json.update(data);
	if (!json.finish()) {
		logger_error("invalid json=%s", data.c_str());
		return false;
	}

	acl::string err;
	if (!acl::deserialize<response_t>(json, res, &err)) {
		logger_error("parse json=%s\r\n\r\nerr=%s\r\n",
			data.c_str(), err.c_str());
		return false;
	}

	return true;
}

void user_status::show_status(const response_t& res) {
	//logger("");
	if (res.host_management.host_info.empty()) {
		logger("host_info empty!");
	}

#if 0
	for (auto it : res.host_management.host_info) {
		logger("name=%s", it.first.c_str());
		logger("hostname=%s, ip=%s, mac=%s, up_limit=%s, down_limit=%s",
			it.second.hostname.c_str(),
			it.second.ip.c_str(),
			it.second.mac.c_str(),
			it.second.up_limit.c_str(),
			it.second.down_limit.c_str());
	}

	logger("--------------------------------------------------------");
#endif

	//////////////////////////////////////////////////////////////////////

	if (res.wireless.sta_list.empty()) {
		logger("sta_list empty!\r\n");
	}

	int i = 0;
	for (auto it : res.wireless.sta_list) {
		if (atoi(it.second.rx_rate.c_str()) >= 5) {
			logger("");
			logger("name=%s", it.first.c_str());
			logger("name=%s, mac=%s, ip=%s, tx_rate=%s, rx_rate=%s",
				it.second.name.c_str(),
				it.second.mac.c_str(),
				it.second.ip.c_str(),
				it.second.tx_rate.c_str(),
				it.second.rx_rate.c_str());
			i++;
		}
	}
	if (i > 0) {
		logger("========================================================");
	}
}

bool user_status::get_status(const char* stok) {
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
		logger_error("send request error, req=%s", req.c_str());
		return false;
	}

	acl::string buf;
	if (!conn.get_body(buf)) {
		logger_error("get response error");
		return false;
	}

	//printf("%s\n", buf.c_str());

	response_t res;
	if (!parse_response(buf, res)) {
		logger_error("parse response error");
		return false;
	}

	show_status(res);
	return true;
}

bool user_status::start(void) {
	while (true) {
		bool ret = get_status(stok_);
		sleep(1);

		if (!ret) {
			return false;
		}
	}
	return true;
}
