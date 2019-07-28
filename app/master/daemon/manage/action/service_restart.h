#pragma once

struct restart_req_t;
struct restart_res_t;
class http_client;

class service_restart
{
public:
	service_restart(http_client& client) : client_(client) {}
	~service_restart(void) {}

	bool run(acl::json& json);

private:
	http_client& client_;

	bool handle(const restart_req_t& req, restart_res_t& res);
};
