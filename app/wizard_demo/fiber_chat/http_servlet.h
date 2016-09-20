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
	bool doLogin(acl::socket_stream&, const std::vector<acl::string>&);
	bool doChat(acl::socket_stream&, const std::vector<acl::string>& tokens);

	chat_client* find(const char* user);
	void remove(acl::socket_stream& conn);

};
