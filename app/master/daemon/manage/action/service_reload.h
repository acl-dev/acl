#pragma once
#include "master/master.h"

class  http_client;
struct reload_req_t;
struct reload_res_t;
struct reload_res_data_t;

class service_reload
{
public:
	service_reload(http_client& client);

	bool run(acl::json& json);

private:
	~service_reload(void) {}

	bool handle(const reload_req_t& req);

private:
	http_client&  client_;
	reload_res_t  res_;
	long long     timeout_;
	int    proc_count_;
	int    proc_signaled_;
	size_t servers_finished_;
	std::map<acl::string, reload_res_data_t> servers_;

	bool reload_one(const char* path, reload_res_data_t& data,
		bool sync_wait);

	void reload_callback(ACL_MASTER_PROC* proc, int status);
	void timeout_callback(void);
	void reload_finish(void);

	static void service_reload_timer(int, ACL_EVENT* event, void* ctx);
	static void service_reload_callback(ACL_MASTER_PROC* proc,
		int status, void* ctx);
};
