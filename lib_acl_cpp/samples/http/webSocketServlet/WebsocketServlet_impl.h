#pragma once

class WebsocketServlet_impl : public acl::WebSocketServlet
{
public:
	WebsocketServlet_impl(acl::redis_client_cluster& cluster, size_t max_conns);
	~WebsocketServlet_impl();

	acl::session& get_session() const
	{
		return *session_;
	}

protected:
	virtual bool doUnknown(acl::HttpServletRequest&,
		acl::HttpServletResponse& res);
	virtual bool doGet(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res);
	virtual bool doPost(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res);


	//for websocket
	/**
	* websocket 关闭消息回调
	* @return {void}
	*/
	virtual void on_close()
	{

	}
	/**
	* websocket ping 消息回调.
	* @param {const char *} buf 消息数据
	* @param {int} len 消息数据长度
	* @return {bool} false 断开连接。
	*/
	virtual bool on_ping(const char *buf, unsigned long long len);
	/**
	* websocket pong 消息回调.
	* @param {const char *} buf 消息数据
	* @param {int} len 消息数据长度
	* @return {bool} false 断开连接。
	*/
	virtual bool on_pong(const char *buf, unsigned long long len);

	/**
	* websocket ping 消息回调.
	* @param data{char *} 回调数据缓存区。
	* @param len{unsigned long long}回调数据缓存区长度。
	* @param text{bool } true 为文本数据。否则是 二进制数据。
	* @return {bool} false 断开连接。
	*/
	virtual bool on_message(char *data, unsigned long long len, bool text);

private:
	acl::session* session_;

	int step_;

	acl::string filename_;

	acl::ofstream *file_;

	int filesize_;

	int current_filesize_;
};
