#pragma once

int http_forward(const char* addr, const char* cmd,
	const acl::string& body, acl::string& out);

template<typename TReq, typename TRes>
int http_forward(const char* addr, acl::json& in, acl::string& out)
{
	TReq req;
	TRes res;

	if (deserialize<TReq>(in, req) == false)
	{
		logger_error("invalid json=%s", in.to_string().c_str());
		return 400;
	}

	acl::string body;
	serialize<TReq>(req, body);

	return http_forward(addr, req.cmd, body, out);
}
