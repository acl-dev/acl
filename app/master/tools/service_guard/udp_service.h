#pragma once

class udp_service : public acl::master_udp
{
public:
	udp_service(void);
	~udp_service(void);

protected:
	// @override
	void on_read(acl::socket_stream* stream);

	// @override
	void thread_on_init(void);

	// @override
	void proc_on_bind(acl::socket_stream& stream);

	// @override
	void proc_on_init(void);

	// @override
	void proc_on_exit(void);

	// @override
	bool proc_on_sighup(acl::string&);

private:
	acl::thread_pool* threads_;
};
