#include "stdafx.h"
#include "configure.h"
#include "http_servlet.h"

static std::vector<chat_client*> __clients;

http_servlet::http_servlet(acl::socket_stream* stream, acl::session* session)
: acl::HttpServlet(stream, session)
, logined_(false)
{

}

http_servlet::~http_servlet(void)
{
	if (!username_.empty())
		oneLogout(username_);
}

bool http_servlet::doError(acl::HttpServletRequest&,
	acl::HttpServletResponse& res)
{
	res.setStatus(400);
	res.setContentType("text/html; charset=");
	// 发送 http 响应头
	if (res.sendHeader() == false)
		return false;

	// 发送 http 响应体
	acl::string buf;
	buf.format("<root error='some error happened!' />\r\n");
	(void) res.getOutputStream().write(buf);
	return false;
}

bool http_servlet::doOther(acl::HttpServletRequest&,
	acl::HttpServletResponse& res, const char* method)
{
	res.setStatus(400);
	res.setContentType("text/html; charset=");
	// 发送 http 响应头
	if (res.sendHeader() == false)
		return false;
	// 发送 http 响应体
	acl::string buf;
	buf.format("<root error='unkown request method %s' />\r\n", method);
	(void) res.getOutputStream().write(buf);
	return false;
}

bool http_servlet::doGet(acl::HttpServletRequest& req,
	acl::HttpServletResponse& res)
{
	return doPost(req, res);
}

bool http_servlet::doPost(acl::HttpServletRequest& req,
	acl::HttpServletResponse& res)
{
	// 如果需要 http session 控制，请打开下面注释，且需要保证
	// 在 master_service.cpp 的函数 thread_on_read 中设置的
	// memcached 服务正常工作
	/*
	const char* sid = req.getSession().getAttribute("sid");
	if (*sid == 0)
		req.getSession().setAttribute("sid", "xxxxxx");
	sid = req.getSession().getAttribute("sid");
	*/

	// 如果需要取得浏览器 cookie 请打开下面注释
	/*
	
	*/

	res.setContentType("text/html; charset=utf-8")	// 设置响应字符集
		.setKeepAlive(req.isKeepAlive())	// 设置是否保持长连接
		.setContentEncoding(true)		// 自动支持压缩传输
		.setChunkedTransferEncoding(true);	// 采用 chunk 传输方式

	acl::string path;
	path << var_cfg_html_path << "/" << "user.html";

	acl::string buf;
	if (acl::ifstream::load(path, &buf) == false)
	{
		logger_error("load %s error %s", path.c_str(), acl::last_serror());
		return doError(req, res);
	}

	// 发送 http 响应体，因为设置了 chunk 传输模式，所以需要多调用一次
	// res.write 且两个参数均为 0 以表示 chunk 传输数据结束
	bool ret = res.write(buf) && res.write(NULL, 0);
	if (ret == false)
		remove(req.getSocketStream());
	return ret;
}

bool http_servlet::doWebsocket(acl::HttpServletRequest& req,
	acl::HttpServletResponse&)
{
	acl::socket_stream& ss = req.getSocketStream();
	acl::websocket in(ss), out(ss);

	while (true)
	{
		if (in.read_frame_head() == false)
		{
			remove(ss);
			printf("read_frame_head error\r\n");
			return false;
		}

		bool ret;
		unsigned char opcode = in.get_frame_opcode();

		switch (opcode)
		{
		case acl::FRAME_PING:
			ret = doPing(in, out);
			break;
		case acl::FRAME_PONG:
			ret = doPong(in, out);
			break;
		case acl::FRAME_CLOSE:
			ret = doClose(in, out);
			break;
		case acl::FRAME_TEXT:
		case acl::FRAME_BINARY:
			ret = doMsg(in, out);
			break;
		case acl::FRAME_CONTINUATION:
			ret = false;
			printf("1-invalid opcode: 0x%x\r\n", opcode);
			break;
		default:
			printf("2-invalid opcode: 0x%x\r\n", opcode);
			ret = false;
			break;
		}

		if (ret == false)
		{
			printf("remove one, opcode: 0x%x\r\n", opcode);
			remove(ss);
			return false;
		}
	}

	// XXX: NOT REACHED
	return false;
}

bool http_servlet::doPing(acl::websocket& in, acl::websocket& out)
{
	unsigned long long len = in.get_frame_payload_len();
	if (len == 0)
	{
		if (out.send_frame_pong((const void*) NULL, 0) == false)
			return false;
		else
			return true;
	}

	out.reset().set_frame_fin(true)
		.set_frame_opcode(acl::FRAME_PONG)
		.set_frame_payload_len(len);

	char buf[8192];
	while (true)
	{
		int ret = in.read_frame_data(buf, sizeof(buf) - 1);
		if (ret == 0)
			break;
		if (ret < 0)
		{
			printf("read_frame_data error\r\n");
			return false;
		}

		buf[ret] = 0;
		printf("read: [%s]\r\n", buf);
		if (out.send_frame_data(buf, ret) == false)
		{
			printf("send_frame_data error\r\n");
			return false;
		}
	}

	return true;
}

bool http_servlet::doPong(acl::websocket& in, acl::websocket&)
{
	unsigned long long len = in.get_frame_payload_len();
	if (len == 0)
		return true;

	char buf[8192];
	while (true)
	{
		int ret = in.read_frame_data(buf, sizeof(buf) - 1);
		if (ret == 0)
			break;
		if (ret < 0)
		{
			printf("read_frame_data error\r\n");
			return false;
		}

		buf[ret] = 0;
		printf("read: [%s]\r\n", buf);
	}

	return true;
}

bool http_servlet::doClose(acl::websocket&, acl::websocket&)
{
	return false;
}

bool http_servlet::doMsg(acl::websocket& in, acl::websocket& out)
{
	acl::string tbuf((size_t) in.get_frame_payload_len() + 1);
	acl::socket_stream& conn = out.get_stream();

	char buf[8192];
	while (true)
	{
		int ret = in.read_frame_data(buf, sizeof(buf) - 1);
		if (ret == 0)
			break;
		if (ret < 0)
		{
			printf("read_frame_data error\r\n");
			return false;
		}

		tbuf.append(buf, ret);
	}

	std::vector<acl::string>& tokens = tbuf.split2("|");
	const acl::string& cmd = tokens[0];

	while (!logined_)
	{
		if (cmd.equal("login", false))
		{
			if (doLogin(conn, tokens) == false)
				return false;
			else
				return true;
		}
		else
		{
			printf("invalid cmd: %s, tbuf: %s\r\n",
				cmd.c_str(), tbuf.c_str());
			return doStatus(conn, "please login first!");
		}
	}

	if (cmd.equal("login", false))
		return doStatus(conn, "already logined");
	if (cmd.equal("chat", false))
	{
		if (doChat(conn, tokens) == false)
			return false;
		else
			return true;
	}
	else
	{
		printf("unknown cmd: %s, msg: %s\r\n",
			cmd.c_str(), tbuf.c_str());
		acl::string info;
		info.format("unknown cmd: %s", cmd.c_str());
		return doStatus(conn, info);
	}

	return true;
}

bool http_servlet::doStatus(acl::socket_stream& conn, const char* info)
{
	acl::string buf;
	buf.format("status|%s", info);

	acl::websocket out(conn);
	out.set_frame_fin(true)
		.set_frame_opcode(acl::FRAME_TEXT)
		.set_frame_payload_len(buf.size());
	return out.send_frame_data(buf.c_str(), buf.size());
}

bool http_servlet::doLogin(acl::socket_stream& conn,
	const std::vector<acl::string>& tokens)
{
	// format: login|user
	if (tokens.size() != 2)
	{
		printf("format: login|user\r\n");
		return false;
	}

	const acl::string& user = tokens[1];
	chat_client* client = find(user);
	if (client != NULL)
	{
		if (&client->get_conn() == &conn)
		{
			printf("user: %s has beean already logined\r\n",
				user.c_str());
			return doStatus(conn, "already logined");
		}
		else
		{
			printf("ERROR, user: %s has beean already logined\r\n",
				user.c_str());
			const char* status = "login|error";
			acl::websocket out(conn);
			out.set_frame_fin(true)
				.set_frame_opcode(acl::FRAME_TEXT)
				.set_frame_payload_len(strlen(status));
			out.send_frame_data(status);
			return false;
		}
	}

	client = new chat_client(user, conn);
	__clients.push_back(client);
	username_ = user;

	printf("status: ok, cmd: login, user: %s\r\n", user.c_str());

	logined_ = true;

	acl::string buf("users");

	for (std::vector<chat_client*>::iterator it = __clients.begin();
		it != __clients.end(); ++it)
	{
		client = *it;
		buf << "|" << (*it)->get_name();
		oneLogin(**it, user);
	}

	acl::websocket out(conn);
	out.set_frame_fin(true)
		.set_frame_opcode(acl::FRAME_TEXT)
		.set_frame_payload_len(buf.size());

	return out.send_frame_data(buf.c_str(), buf.size());
}

void http_servlet::oneLogin(const acl::string& user)
{
	for (std::vector<chat_client*>::iterator it = __clients.begin();
		it != __clients.end(); ++it)
	{
		if (user.equal((*it)->get_name()))
			continue;
		oneLogin(**it, user);
	}
}

void http_servlet::oneLogin(chat_client& client, const acl::string& user)
{
	acl::string buf;
	buf << "login|" << user;
	acl::websocket out(client.get_conn());
	out.set_frame_fin(true)
		.set_frame_opcode(acl::FRAME_TEXT)
		.set_frame_payload_len(buf.size());
	out.send_frame_data(buf.c_str(), buf.size());
}

void http_servlet::oneLogout(const acl::string& user)
{
	for (std::vector<chat_client*>::iterator it = __clients.begin();
		it != __clients.end(); ++it)
	{
		if (user.equal((*it)->get_name()))
			continue;
		oneLogout(**it, user);
	}
}

void http_servlet::oneLogout(chat_client& client, const acl::string& user)
{
	acl::string buf;
	buf << "logout|" << user;
	acl::websocket out(client.get_conn());
	out.set_frame_fin(true)
		.set_frame_opcode(acl::FRAME_TEXT)
		.set_frame_payload_len(buf.size());
	out.send_frame_data(buf.c_str(), buf.size());
}

bool http_servlet::doChat(acl::socket_stream& from_conn,
	const std::vector<acl::string>& tokens)
{
	// format: chat|msg|to_user
	if (tokens.size() != 3)
	{
		printf("format: chat|msg|to_user\r\n");
		return false;
	}

	chat_client* from = find(from_conn);
	if (from == NULL)
	{
		printf("not found from: %p\r\n", &from_conn);
		(void) doStatus(from_conn, "not logined!");
		return false;
	}

	const acl::string& msg = tokens[1];
	const acl::string& to_user = tokens[2];

	if (to_user.equal("all", false))
	{
		for (std::vector<chat_client*>::iterator it = __clients.begin();
			it != __clients.end(); ++it)
		{
			doChat(from->get_name(), **it, msg);
		}
		return true;
	}

	chat_client* to_client = find(to_user);
	if (to_client == NULL)
	{
		printf("no exist to_user: %s\r\n", to_user.c_str());
		return true;
	}

	doChat(from->get_name(), *to_client, msg);
	return true;
}

bool http_servlet::doChat(const acl::string& from_user,
	chat_client& to_client, const acl::string& msg)
{
	acl::string buf;
	buf.format("chat|%s|%s", from_user.c_str(), msg.c_str());

	acl::websocket out(to_client.get_conn());
	out.set_frame_fin(true)
		.set_frame_opcode(acl::FRAME_TEXT)
		.set_frame_payload_len(buf.size());

	if (out.send_frame_data(buf.c_str(), buf.size()) == false)
	{
		printf("----close socket: %d----\r\n",
			to_client.get_conn().sock_handle());
		shutdown(to_client.get_conn().sock_handle(), SHUT_RDWR);
		//close(to_client.get_conn().sock_handle());
	}

	printf("status: ok, cmd: chat, msg: %s, to_user: %s\r\n",
		msg.c_str(), to_client.get_name().c_str());

	return true;
}

chat_client* http_servlet::find(const char* user)
{
	for (std::vector<chat_client*>::iterator it = __clients.begin();
		it != __clients.end(); ++it)
	{
		printf("user: %s, %s\r\n", user, (*it)->get_name().c_str());
		if (*(*it) == user)
			return *it;
	}

	return NULL;
}

chat_client* http_servlet::find(acl::socket_stream& conn)
{
	for (std::vector<chat_client*>::iterator it = __clients.begin();
		it != __clients.end(); ++it)
	{
		acl::socket_stream* ss = &(*it)->get_conn();
		if (ss == &conn)
			return *it;
	}

	printf("not found connection: %p\r\n", &conn);
	return NULL;
}

void http_servlet::remove(acl::socket_stream& conn)
{
	for (std::vector<chat_client*>::iterator it = __clients.begin();
		it != __clients.end(); ++it)
	{
		acl::socket_stream* ss = &(*it)->get_conn();
		if (ss == &conn)
		{
			printf("removed: %s\r\n", (*it)->get_name().c_str());
			__clients.erase(it);
			return;
		}
	}

	printf("not found connection: %p\r\n", &conn);
}
