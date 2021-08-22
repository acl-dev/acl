#include "stdafx.h"

class http_servlet : public acl::HttpServlet
{
public:
	http_servlet(acl::socket_stream* stream, acl::session* session)
		: HttpServlet(stream, session), i_(0)
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
		static int __n = 0;
		acl::string buf;
		buf.format("hello world-%d, total=%d", i_++, __n++);

		res.setContentLength(buf.size());
		res.setKeepAlive(true);

		// ∑¢ÀÕ http œÏ”¶ÃÂ
		return res.write(buf) && res.write(NULL, 0);
	}

private:
	int i_;
};
