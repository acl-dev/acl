#include "stdafx.h"
#include "http_servlet.h"

static std::vector<chat_client*> clients_;

http_servlet::http_servlet(acl::socket_stream* stream, acl::session* session)
: acl::HttpServlet(stream, session)
{

}

http_servlet::~http_servlet(void)
{

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

	res.setContentType("text/xml; charset=utf-8")	// 设置响应字符集
		.setKeepAlive(req.isKeepAlive())	// 设置是否保持长连接
		.setContentEncoding(true)		// 自动支持压缩传输
		.setChunkedTransferEncoding(true);	// 采用 chunk 传输方式

	const char* param1 = req.getParameter("name1");
	const char* param2 = req.getParameter("name2");

	// 创建 xml 格式的数据体
	acl::xml1 body;
	body.get_root()
		.add_child("root", true)
			.add_child("params", true)
				.add_child("param", true)
					.add_attr("name1", param1 ? param1 : "null")
				.get_parent()
				.add_child("param", true)
					.add_attr("name2", param2 ? param2 : "null");
	acl::string buf;
	body.build_xml(buf);

	// 发送 http 响应体，因为设置了 chunk 传输模式，所以需要多调用一次
	// res.write 且两个参数均为 0 以表示 chunk 传输数据结束
	return res.write(buf) && res.write(NULL, 0);
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
			printf("read_frame_head error\r\n");
			return false;
		}

		bool ret;
		unsigned char opcode = in.get_frame_opcode();

		printf("opcode: 0x%x\r\n", opcode);

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
			break;
		default:
			ret = false;
			break;
		}

		if (ret == false)
			return false;
	}

	// XXX: NOT REACHED
	return false;
}

bool http_servlet::doPing(acl::websocket& in, acl::websocket& out)
{
	unsigned long long len = in.get_frame_payload_len();
	if (len == 0)
	{
		if (out.send_frame_pong(NULL, 0) == false)
		{
			remove(out.get_stream());
			return false;
		}
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
			remove(out.get_stream());
			return false;
		}

		buf[ret] = 0;
		printf("read: [%s]\r\n", buf);
		if (out.send_frame_data(buf, ret) == false)
		{
			printf("send_frame_data error\r\n");
			remove(out.get_stream());
			return false;
		}
	}

	return true;
}

bool http_servlet::doPong(acl::websocket& in, acl::websocket& out)
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
			remove(out.get_stream());
			return false;
		}

		buf[ret] = 0;
		printf("read: [%s]\r\n", buf);
	}

	return true;
}

bool http_servlet::doClose(acl::websocket&, acl::websocket& out)
{
	acl::socket_stream& conn = out.get_stream();
	remove(conn);
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
			remove(conn);
			return false;
		}

		tbuf.append(buf, ret);
	}

	std::vector<acl::string>& tokens = tbuf.split2("|");
	const acl::string& cmd = tokens[0];

	if (cmd.equal("login", false))
	{
		if (doLogin(conn, tokens) == false)
		{
			remove(conn);
			return false;
		}
		else
			return true;
	}
	else if (cmd.equal("chat", false))
	{
		if (doChat(conn, tokens) == false)
		{
			remove(conn);
			return false;
		}
		else
			return true;
	}
	else
	{
		printf("unknown msg: %s\r\n", tbuf.c_str());
		remove(conn);
		return false;
	}

	return true;
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

	chat_client* client = new chat_client(user, conn);
	clients_.push_back(client);

	printf("status: ok, cmd: login, user: %s\r\n", user.c_str());
	return true;
}

bool http_servlet::doChat(acl::socket_stream&,
	const std::vector<acl::string>& tokens)
{
	// format: chat|msg|to_user
	if (tokens.size() != 3)
	{
		printf("format: chat|msg|to_user\r\n");
		return false;
	}

	const acl::string& msg = tokens[1];
	const acl::string& to_user = tokens[2];

	chat_client* to_client = find(to_user);
	if (to_client == NULL)
	{
		printf("no exist to_user: %s\r\n", to_user.c_str());
		return true;
	}

	acl::websocket out((to_client->get_conn()));
	out.set_frame_fin(true)
		.set_frame_opcode(acl::FRAME_TEXT)
		.set_frame_payload_len(msg.size());

	(void) out.send_frame_data(msg.c_str(), msg.size());

	printf("status: ok, cmd: chat, msg: %s, to_user: %s\r\n",
		msg.c_str(), to_user.c_str());

	return true;
}

chat_client* http_servlet::find(const char* user)
{
	for (std::vector<chat_client*>::iterator it = clients_.begin();
		it != clients_.end(); ++it)
	{
		printf("user: %s, %s\r\n", user, (*it)->get_name().c_str());
		if (*(*it) == user)
			return *it;
	}

	return NULL;
}

void http_servlet::remove(acl::socket_stream& conn)
{
	for (std::vector<chat_client*>::iterator it = clients_.begin();
		it != clients_.end(); ++it)
	{
		acl::socket_stream* ss = &(*it)->get_conn();
		if (ss == &conn)
		{
			clients_.erase(it);
			return;
		}
	}

	printf("not found connection: %p\r\n", &conn);
}
