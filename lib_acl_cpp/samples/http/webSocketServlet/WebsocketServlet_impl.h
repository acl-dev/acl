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
	 * websocket ping 消息回调.
	 * @param {const char *} buf 消息数据
	 * @param {int} len 消息数据长度
	 * @return {bool} false 断开连接。
	 */
	bool onPing(const char *buf, unsigned long long len);

	/**
	 * @override
	 * websocket pong 消息回调.
	 * @param {const char *} buf 消息数据
	 * @param {int} len 消息数据长度
	 * @return {bool} false 断开连接。
	 */
	bool onPong(const char *buf, unsigned long long len);

	/**
	 * @override
	 * websocket ping 消息回调.
	 * @param data{char *} 回调数据缓存区。
	 * @param len{unsigned long long}回调数据缓存区长度。
	 * @param text{bool } true 为文本数据。否则是 二进制数据。
	 * @return {bool} false 断开连接。
	 */
	bool onMessage(char *data, unsigned long long len, bool text);

private:
	acl::session* session_;
	int step_;
	acl::string filename_;
	acl::ofstream *file_;
	int filesize_;
	int current_filesize_;
};
