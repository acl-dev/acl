#pragma once

template<typename TReq, typename TRes>
bool http_request(const char* addr, TReq& req, TRes& res)
{
	acl::string body;
	serialize<TReq>(req, body);

	acl::http_request conn(addr, 30, 30);
	acl::string url;
	url.format("/?cmd=%s", req.cmd.c_str());
	conn.request_header().set_url(url).set_keep_alive(false)
		.set_content_type("text/json");

	if (conn.request(body, body.size()) == false)
	{
		printf("request error, json=[%s]\r\n", body.c_str());
		return false;
	}

	acl::json json;
	if (conn.get_body(json) == false)
	{
		printf("get_body error, json=[%s]\r\n", body.c_str());
		return false;
	}

	if (deserialize<TRes>(json, res) == false)
	{
		printf("deserialize error, req json=[%s]\r\n", body.c_str());
		return false;
	}

	//printf(">>>%s\r\n", json.to_string().c_str());
	return true;
}
