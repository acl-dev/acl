#pragma once

class service_app
{
public:
	service_app(acl::redis_client_cluster& redis) : redis_(redis) {}
	~service_app(void) {}

	bool save(const char* ip, const service_list_res_t& res);

private:
	acl::redis_client_cluster& redis_;

	bool save_one(const char* ip, const service_info_t& info);
};
