#pragma once

struct kill_req_t;
struct kill_res_t;
struct kill_res_data_t;
class http_client;

class service_kill
{
public:
	service_kill(http_client& client) : client_(client) {}
	~service_kill(void) {}

	bool run(acl::json& json);

private:
	bool handle(const kill_req_t& req, kill_res_t& res);
	bool kill_one(const char* path, kill_res_data_t& data);

private:
	http_client& client_;
};
