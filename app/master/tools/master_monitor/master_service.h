#pragma once

extern acl::master_str_tbl var_conf_str_tab[];
extern acl::master_bool_tbl var_conf_bool_tab[];
extern acl::master_int_tbl var_conf_int_tab[];
extern acl::master_int64_tbl var_conf_int64_tab[];

//////////////////////////////////////////////////////////////////////////

class master_service : public acl::master_fiber
{
public:
	master_service(void);
	~master_service(void);

protected:
	// @override
	void on_accept(acl::socket_stream& conn);

	// @override
	void proc_on_listen(acl::server_socket& ss);

	// @override
	void proc_pre_jail(void);

	// @override
	void proc_on_init(void);

	// @override
	void proc_on_exit(void);

	// @override
	bool proc_on_sighup(acl::string&);

private:
	bool service_exit_;
	acl::thread* monitor_;
	acl::tcp_ipc ipc_;

	void handle(const acl::string& data);
};
