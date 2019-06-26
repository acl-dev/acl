#pragma once

class service_dead
{
public:
	service_dead(acl::redis_client_cluster& redis) : redis_(redis) {}
	~service_dead(void) {}

	bool save(const char* ip, const service_dead_res_t& res);

private:
	acl::redis_client_cluster& redis_;
};
