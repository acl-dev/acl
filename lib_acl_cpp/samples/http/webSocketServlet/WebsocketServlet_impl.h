#pragma once

class WebsocketServlet_impl : public acl::WebSocketServlet
{
public:
	WebsocketServlet_impl(acl::redis_client_cluster& cluster, size_t max_conns);
	~WebsocketServlet_impl(void);

	acl::session& get_session(void) const
	{
		return *session_;
	}

protected:
	// @override
	bool doUnknown(acl::HttpServletRequest&, acl::HttpServletResponse&);
	// @override
	bool doGet(acl::HttpServletRequest&, acl::HttpServletResponse&);
	// @override
	bool doPost(acl::HttpServletRequest&, acl::HttpServletResponse&);

	//for websocket
	/**
	 * @override
	 * websocket 关闭消息回调
	 * @return {void}
	 */
	void onClose(void) {}

	/**
	 * @override
	 */
	bool onPing(unsigned long long len, bool finish);

	/**
	 * @override
	 */
	bool onPong(unsigned long long len, bool finish);

	/**
	 * @override
	 */
	bool onMessage(unsigned long long len, bool text, bool finish);

private:
	acl::session* session_;
	int step_;
	acl::string filename_;
	long long filesize_;
	long long nread_;
	acl::ofstream fp_;

	bool getFilename(unsigned long long payload_len);
	bool getFilesize(unsigned long long payload_len);
	bool readData(unsigned long long len, acl::string& path);
	bool saveFile(unsigned long long len);
};
