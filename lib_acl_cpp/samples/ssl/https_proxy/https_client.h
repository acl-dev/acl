#pragma once

class https_client
{
public:
	https_client(acl::ostream& out, acl::sslbase_conf* conf);
	~https_client();

	bool http_request(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res);

private:
	acl::ostream& out_;
	acl::sslbase_conf* ssl_conf_;

	bool connect_server(const acl::string& server_addr,
		acl::http_client& client);
};
