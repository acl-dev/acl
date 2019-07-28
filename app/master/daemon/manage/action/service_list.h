#pragma once

struct ACL_MASTER_SERV;
struct list_req_t;
struct list_res_t;
class http_client;

class service_list
{
public:
	service_list(http_client& client) : client_(client) {}
	~service_list(void) {}

	bool run(acl::json& json);

private:
	http_client& client_;

	bool handle(const list_req_t& req, list_res_t& res);
	void add_one(list_res_t& res, const ACL_MASTER_SERV* ser);
};
