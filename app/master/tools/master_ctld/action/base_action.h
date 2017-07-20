#pragma once

class base_action
{
public:
	base_action(void);
	virtual ~base_action(void) {}

	virtual int run(acl::string& json) = 0;

	void prepare(const acl::string&, acl::HttpServletRequest&,
		acl::HttpServletResponse&);

private:
	acl::string addr_;
	acl::HttpServletRequest* req_;
	acl::HttpServletResponse* res_;

	acl::json* get_json(void);

protected:
	template<typename TReq, typename TRes>
	int forward(TReq& req, TRes& res, acl::string& out)
	{
		acl::json* json = get_json();
		if (json == NULL)
		{
			logger_error("no json request");
			return 400;
		}

		//printf(">>>req: [%s]\r\n", json->to_string().c_str());

		if (deserialize<TReq>(*json, req) == false)
		{
			logger_error("invalid json=%s",
				json->to_string().c_str());
			return 400;
		}

		if (!http_request<TReq, TRes>(addr_, req, res))
		{
			logger_error("http_request error, json=%s, cmd=%s",
				json->to_string().c_str(), req.cmd.c_str());
			return 503;
		}

		serialize<TRes>(res, out);
		return 200;
	}
};
