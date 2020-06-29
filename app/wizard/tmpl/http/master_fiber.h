#pragma once

class http_service;

class master_service : public acl::master_fiber
{
public:
	master_service(void);
	~master_service(void);

	http_service& get_service(void) const;

protected:
	// @override
	void on_accept(acl::socket_stream& conn);

	// @override
	void proc_pre_jail(void);

	// @override
	void proc_on_listen(acl::server_socket& ss);

	// @override
	void proc_on_init(void);

	// @override
	void proc_on_exit(void);

	// @override
	bool proc_on_sighup(acl::string&);

private:
	http_service* service_;
};
