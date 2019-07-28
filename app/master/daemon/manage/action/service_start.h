#pragma once
#include "master/master.h"

struct start_req_t;
struct start_res_t;
class http_client;

class service_start
{
public:
	service_start(http_client& client);

	bool run(acl::json& json);

private:
	~service_start(void) {}

	bool handle(const start_req_t& req);

private:
	http_client& client_;
	start_res_t  res_;
	long long    timeout_;
	int    proc_count_;
	int    proc_signaled_;
	size_t servers_finished_;
	std::map<acl::string, start_res_data_t> servers_;

	bool start_one(const char* path, start_res_data_t& data,
		bool status, const char* ext);

	void start_callback(ACL_MASTER_PROC* proc, int status);
	void timeout_callback(void);
	void start_finish(void);

	static void service_start_timer(int, ACL_EVENT* event, void* ctx);
	static void service_start_callback(ACL_MASTER_PROC* proc,
		int status, void *ctx);
};
