#pragma once

class redis_client;
class redis_object;

class redis_transfer {
public:
	redis_transfer(acl::socket_stream& conn, acl::redis_command& cmd,
		const redis_object& req);

	~redis_transfer(void);

	bool run(void);

private:
	acl::socket_stream& conn_;
	acl::redis_command& cmd_;
	const redis_object& req_;
};
