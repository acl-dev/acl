#pragma once

class https_request
{
public:
	https_request(const char* addr, acl::polarssl_conf* ssl_conf);
	~https_request(void);

	void* run(void);

private:
	acl::http_request request_;
	acl::string to_charset_;

	// 澶勭悊 text/plain 绫诲瀷鏁版嵁
	bool do_plain(acl::http_request& req);

	// 澶勭悊 text/xml 绫诲瀷鏁版嵁
	bool do_xml(acl::http_request& req);

	// 澶勭悊 text/json 绫诲瀷鏁版嵁
	bool do_json(acl::http_request& req);
};
