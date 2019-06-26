#pragma once

class guard_action : public acl::thread_job
{
public:
	guard_action(const char* ip, const char* data);

	// @override
	void* run(void);

private:
	~guard_action(void);

private:
	acl::string ip_;
	acl::string data_;

	bool do_run(void);

	bool on_service_list(acl::json& json);
	bool on_service_dead(acl::json& json);
};
