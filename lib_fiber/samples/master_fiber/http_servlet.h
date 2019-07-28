#include "stdafx.h"

class http_servlet : public acl::HttpServlet
{
public:
	http_servlet(acl::socket_stream* stream, acl::session* session)
		: HttpServlet(stream, session)
	{
	}

	~http_servlet(void)
	{
	}

	// override
	bool doGet(acl::HttpServletRequest& req, acl::HttpServletResponse& res)
	{
		return doPost(req, res);
	}

	// override
	bool doPost(acl::HttpServletRequest&, acl::HttpServletResponse& res)
	{
		const char* buf = "hello world!";
		size_t len = strlen(buf);

		res.setContentLength(len);
		res.setKeepAlive(true);

		// ∑¢ÀÕ http œÏ”¶ÃÂ
		return res.write(buf, len) && res.write(NULL, 0);
	}
};
