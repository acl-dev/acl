#pragma once

struct stat_req_t;
struct stat_res_t;
struct serv_info_t;
class http_client;

class service_stat
{
public:
	service_stat(http_client& client) : client_(client) {}
	~service_stat(void) {}

	bool run(acl::json& json);
private:
	bool handle(const stat_req_t& req, stat_res_t& res);
	bool stat_one(const char* path, serv_info_t& info);

private:
	http_client& client_;
};
