#pragma once

class guard_report
{
public:
	guard_report(const char* guard_manager, acl::tcp_ipc& ipc,
		int conn_timeout = 10, int rw_timeout = 10);
	~guard_report(void) {}

	bool report(const acl::string& body);

private:
	acl::string guard_manager_;
	acl::tcp_ipc& ipc_;
	int conn_timeout_;
	int rw_timeout_;

	bool tcp_report(const acl::string& body);
	bool udp_report(const acl::string& body);

	bool get_one_addr(acl::string& addr);
	bool resolve_domain(std::vector<acl::string>& ips, int& port);
};
