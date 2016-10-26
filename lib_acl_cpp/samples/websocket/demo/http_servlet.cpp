#include "stdafx.h"
#include "acl_cpp/http/websocket.hpp"
#include "http_servlet.h"

http_servlet::http_servlet(acl::redis_client_cluster& cluster, size_t max_conns)
{
	// ���� session �洢����
	session_ = new acl::redis_session(cluster, max_conns);
}

http_servlet::~http_servlet(void)
{
	delete session_;
}

bool http_servlet::doError(acl::HttpServletRequest&,
	acl::HttpServletResponse& res)
{
	res.setStatus(400);
	res.setContentType("text/html; charset=");
	// ���� http ��Ӧͷ
	if (res.sendHeader() == false)
		return false;

	// ���� http ��Ӧ��
	acl::string buf;
	buf.format("<root error='some error happened!' />\r\n");
	(void) res.getOutputStream().write(buf);
	return false;
}

bool http_servlet::doUnknown(acl::HttpServletRequest&,
	acl::HttpServletResponse& res)
{
	res.setStatus(400);
	res.setContentType("text/html; charset=");
	// ���� http ��Ӧͷ
	if (res.sendHeader() == false)
		return false;
	// ���� http ��Ӧ��
	acl::string buf("<root error='unkown request method' />\r\n");
	(void) res.getOutputStream().write(buf);
	return false;
}

bool http_servlet::doGet(acl::HttpServletRequest& req,
	acl::HttpServletResponse& res)
{
	printf("in doGet\r\n");
	return doPost(req, res);
}

bool http_servlet::doPing(acl::websocket& in, acl::websocket& out)
{
	unsigned long long len = in.get_frame_payload_len();
	if (len == 0)
		return out.send_frame_pong((const void*) NULL, 0);

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
	unsigned long long len = in.get_frame_payload_len();
	out.reset().set_frame_fin(true)
		.set_frame_opcode(acl::FRAME_TEXT)
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

	sleep(1);
	char info[256];
	snprintf(info, sizeof(info), "hello world!");
	out.reset().set_frame_fin(true)
		.set_frame_opcode(acl::FRAME_TEXT)
		.set_frame_payload_len(strlen(info));
	if (out.send_frame_data(info, strlen(info)) == false)
	{
		printf("send_frame_data error\r\n");
		return false;
	}

	sleep(1);
	snprintf(info, sizeof(info), "hello zsx!");
	out.reset().set_frame_fin(true)
		.set_frame_opcode(acl::FRAME_TEXT)
		.set_frame_payload_len(strlen(info));
	if (out.send_frame_data(info, strlen(info)) == false)
	{
		printf("send_frame_data error\r\n");
		return false;
	}

	sleep(1);
	snprintf(info, sizeof(info), "GoodBye!");
	out.reset().set_frame_fin(true)
		.set_frame_opcode(acl::FRAME_TEXT)
		.set_frame_payload_len(strlen(info));
	if (out.send_frame_data(info, strlen(info)) == false)
	{
		printf("send_frame_data error\r\n");
		return false;
	}

	return true;
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

bool http_servlet::doPost(acl::HttpServletRequest& req,
	acl::HttpServletResponse& res)
{
	res.setContentType("text/html; charset=utf-8")	// ������Ӧ�ַ���
		.setContentEncoding(true)		// �����Ƿ�ѹ������
		.setChunkedTransferEncoding(false);	// ���� chunk ���䷽ʽ

	acl::string html_file;
	html_file << var_cfg_html_path << "/client.html";
	acl::string buf;
	if (acl::ifstream::load(html_file, &buf) == false)
	{
		logger_error("load %s error %s",
			html_file.c_str(), acl::last_serror());
		return doError(req, res);
	}

	// ���� http ��Ӧ�壬��Ϊ������ chunk ����ģʽ��������Ҫ�����һ��
	// res.write ������������Ϊ 0 �Ա�ʾ chunk �������ݽ���
	return res.write(buf) && res.write(NULL, 0);
}
