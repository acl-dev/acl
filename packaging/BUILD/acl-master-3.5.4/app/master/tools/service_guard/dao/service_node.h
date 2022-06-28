#pragma once

class service_node
{
public:
	service_node(acl::redis_client_cluster& redis);
	~service_node(void) {}

	bool save(const char* ip, const service_list_res_t& res);

private:
	acl::redis_client_cluster& redis_;
	time_t when_;

	bool add_machine_services(const char* ip,
		const std::map<acl::string, double>& services);
	bool add_services(const std::map<acl::string, double>& services);
	bool add_machines(const char* ip);
	bool add_main_services(const char* ip,
		const std::map<acl::string, double>& services);
	bool add_main_service(const char* service, const char* ip);
};
