#pragma once
#include "master/master.h"

class http_client;
struct signal_req_t;

class service_signal {
public:
	service_signal(http_client& client);

	bool run(acl::json& json);

private:
	~service_signal(void) {}

	bool handle(const signal_req_t& req);

private:
	http_client& client_;
	signal_res_t res_;
	int    proc_count_;
	int    proc_signaled_;
	std::map<acl::string, signal_res_data_t> servers_;

	bool signal_one(const char* path, int signum, signal_res_data_t& data);
};
