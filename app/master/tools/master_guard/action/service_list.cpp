#include "stdafx.h"
#include "http_request.h"
#include "json/service_struct.h"
#include "json/service_struct.gson.h"
#include "service_list.h"

service_list::service_list(const char* master_ctld, const char* guard_manager,
	int conn_timeout, int rw_timeout)
: master_ctld_(master_ctld)
, guard_manager_(guard_manager)
, conn_timeout_(conn_timeout)
, rw_timeout_(rw_timeout)
{
}

bool service_list::run(void)
{
	list_req_t req;
	req.cmd = "list";

	list_res_t res;
	if (!http_request<list_req_t, list_res_t>(master_ctld_, req, res))
		return false;

	service_list_res_t list_res;
	list_res.status = 200;
	list_res.msg    = "+ok";

	for (std::vector<serv_info_t>::const_iterator cit = res.data.begin();
		cit != res.data.end(); ++cit) {

		service_info_t info;
		info.status = (*cit).status;
		info.start  = (*cit).start;
		info.name   = (*cit).name;
		info.conf   = (*cit).conf;
		list_res.data.push_back(info);
	}

	acl::string body;
	acl::json json;
	acl::json_node& node = acl::gson(json, list_res);
	body = node.to_string();
//	logger("|%s|, len=%d", body.c_str(), (int) body.size());

	return report(body);
}

bool service_list::report(const acl::string& body)
{
	if (body.size() >= 1460)
		return tcp_report(body);
	else
		return udp_report(body);
}

bool service_list::tcp_report(const acl::string& body)
{
	acl::socket_stream conn;

	if (conn.open(guard_manager_, conn_timeout_, rw_timeout_) == false) {
		logger_error("connect %s error %s", guard_manager_.c_str(),
			acl::last_serror());
		return false;
	}

	if (conn.write(body) == -1) {
		logger_error("write to %s error %s", guard_manager_.c_str(),
			acl::last_serror());
		return false;
	}
	return true;
}

bool service_list::udp_report(const acl::string& body)
{
	acl::string domain(guard_manager_);
	char* ptr = domain.c_str();
	char* port = strchr(ptr, ':');
	if (port == NULL || port == ptr || *(port + 1) == 0) {
		logger_error("invalid guard_manager=%s", guard_manager_.c_str());
		return false;
	}
	*port++ = 0;

	int h_error;
	ACL_DNS_DB *db = acl_gethostbyname(domain.c_str(), &h_error);
	if (db == NULL) {
		logger_error("acl_gethostbyname error %s, domain=%s",
			acl_netdb_strerror(h_error), domain.c_str());
		return false;
	}

	acl::string ip;
	ACL_ITER iter;
	acl_foreach(iter, db) {
		ACL_HOSTNAME* host = (ACL_HOSTNAME *) iter.data;
		ip = host->ip;
	}
	acl_netdb_free(db);
	if (ip.empty()) {
		logger_error("no ip found, domain=%s", domain.c_str());
		return false;
	}

	ip << ":" << port;

//	printf("addr=%s, %s\n", guard_manager_.c_str(), ip.c_str());
	const char* local_addr = "0.0.0.0:0";
	acl::socket_stream stream;
	if (stream.bind_udp(local_addr) == false) {
		logger_error("bind_udp %s error %s", local_addr, acl::last_serror());
		return false;
	}
	stream.set_peer(ip);
	if (stream.write(body) == false) {
		logger_error("write %s error %s", body.c_str(), acl::last_serror());
		return false;
	}

	return true;
}
