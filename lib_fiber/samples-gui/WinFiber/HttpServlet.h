#pragma once

class HttpServlet : public acl::HttpServlet {
public:
	HttpServlet(acl::socket_stream* stream, acl::session* session)
	: acl::HttpServlet(stream, session), i_(0) {}

	~HttpServlet(void) {}

	// override
	bool doGet(acl::HttpServletRequest& req, acl::HttpServletResponse& res) {
		return doPost(req, res);
	}

	// override
	bool doPost(acl::HttpServletRequest& req, acl::HttpServletResponse& res) {
		static int __n = 0;

		acl::string buf;
		buf.format("hello world, i=%d, n=%d", i_++, __n++);

		res.setContentLength(buf.size());
		res.setKeepAlive(req.isKeepAlive());

		// ∑¢ÀÕ http œÏ”¶ÃÂ
		return res.write(buf) && res.write(NULL, 0);
	}

private:
	int i_;
};
