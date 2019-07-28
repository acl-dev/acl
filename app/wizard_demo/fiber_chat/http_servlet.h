#pragma once
#include <vector>

class chat_client
{
public:
	chat_client(const char* name, acl::socket_stream& conn)
		: name_(name), conn_(conn) {}
	~chat_client(void);

	const acl::string& get_name(void) const
	{
		return name_;
	}

	acl::socket_stream& get_conn(void) const
	{
		return conn_;
	}

	bool operator == (const char* name) const
	{
		return name_.equal(name, false);
	}

private:
	acl::string name_;
	acl::socket_stream& conn_;
};

class http_servlet : public acl::HttpServlet
{
public:
	http_servlet(acl::socket_stream* stream, acl::session* session);
	~http_servlet();

protected:
	// @override
	bool doError(acl::HttpServletRequest&, acl::HttpServletResponse&);

	// @override
	bool doOther(acl::HttpServletRequest&,
		acl::HttpServletResponse&, const char* method);

	// @override
	bool doGet(acl::HttpServletRequest&, acl::HttpServletResponse&);

	// @override
	bool doPost(acl::HttpServletRequest&, acl::HttpServletResponse&);

	// @override
	bool doWebsocket(acl::HttpServletRequest&, acl::HttpServletResponse&);

private:
	bool doPing(acl::websocket&, acl::websocket&);
	bool doPong(acl::websocket&, acl::websocket&);
	bool doClose(acl::websocket&, acl::websocket&);
	bool doMsg(acl::websocket&, acl::websocket&);

private:
	acl::string username_;
	bool logined_;
	bool doStatus(acl::socket_stream& conn, const char* info);
	bool doLogin(acl::socket_stream&, const std::vector<acl::string>&);
	bool doChat(acl::socket_stream&, const std::vector<acl::string>& tokens);
	bool doChat(const acl::string& from_user, chat_client& to_client, 
		const acl::string& msg);

	void oneLogin(const acl::string& user);
	void oneLogin(chat_client& client, const acl::string& user);
	void oneLogout(const acl::string& user);
	void oneLogout(chat_client& client, const acl::string& user);

	chat_client* find(const char* user);
	chat_client* find(acl::socket_stream& conn);
	void remove(acl::socket_stream& conn);
};
