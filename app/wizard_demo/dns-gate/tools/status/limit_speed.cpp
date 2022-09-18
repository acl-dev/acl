#include "stdafx.h"
#include "limit_speed.h"

limit_speed::limit_speed(const char* addr, acl::sslbase_conf& ssl_conf,
	const char* stok, const char* mac, const char* ip)
: addr_(addr)
, ssl_conf_(ssl_conf)
, stok_(stok)
, mac_(mac)
, ip_(ip)
, hostname_("---")
, ssid_("TP_LINK_866D")
{
}

limit_speed::~limit_speed(void) {}

limit_speed& limit_speed::set_hostname(const char* val) {
	hostname_ = val;
	return *this;
}

limit_speed& limit_speed::set_ssid(const char* val) {
	ssid_ = val;
	return *this;
}

bool limit_speed::start(int up, int down) {
	acl::http_request conn(addr_);
	conn.set_ssl(&ssl_conf_);

	acl::string url;
	url << "/stok=" << stok_ << "/ds";

	acl::string buf;
	build_request(buf, up, down);

	conn.request_header().set_url(url)
		.set_host(addr_)
		.set_content_type("application/json; charset=UTF-8")
		.add_entry("Origin", "https://192.168.1.65/")
		.add_entry("Referer", "https://192.168.1.65/")
		.set_content_length(buf.size());

	time_t begin = time(NULL);

	if (!conn.request(buf, buf.size())) {
		logger_error("send request error, req=%s", buf.c_str());
		return false;
	}

	buf.clear();
	if (!conn.get_body(buf)) {
		logger_error("get respone error");
		return false;
	}

	logger("result: %s, cost: %ld seconds", buf.c_str(), time(NULL) - begin);
	return true;
}

void limit_speed::build_request(acl::string& buf, int up, int down) {
	limit_speed_req_t req;

	filter_mac_t mac;
	mac.mac = mac_;

	req.host_management.filter.push_back(mac);
	req.host_management.para.ip = ip_;
	req.host_management.para.mac = mac_;
	req.host_management.para.up_limit = acl::string::parse_int(up);
	req.host_management.para.down_limit = acl::string::parse_int(down);
	req.host_management.para.name = "";
	req.host_management.para.hostname = hostname_;

	acl::serialize<limit_speed_req_t>(req, buf);
}
