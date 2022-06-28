#include "stdafx.h"
#include "acl_cpp/http/websocket.hpp"
#include "http_servlet.h"

http_servlet::http_servlet(acl::redis_client_cluster& cluster, size_t max_conns)
{
	// 创建 session 存储对象
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
	// 发送 http 响应头
	if (res.sendHeader() == false)
		return false;

	// 发送 http 响应体
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
	// 发送 http 响应头
	if (res.sendHeader() == false)
		return false;
	// 发送 http 响应体
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
	while (true) {
		int ret = in.read_frame_data(buf, sizeof(buf) - 1);
		if (ret == 0)
			break;
		if (ret < 0) {
			printf("read_frame_data error\r\n");
			return false;
		}

		buf[ret] = 0;
		printf("read: [%s]\r\n", buf);
		if (out.send_frame_data(buf, ret) == false) {
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
	while (true) {
		int ret = in.read_frame_data(buf, sizeof(buf) - 1);
		if (ret == 0)
			break;
		if (ret < 0) {
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
	while (true) {
		int ret = in.read_frame_data(buf, sizeof(buf) - 1);
		if (ret == 0)
			break;
		if (ret < 0) {
			printf("read_frame_data error\r\n");
			return false;
		}

		buf[ret] = 0;
		printf("read: [%s]\r\n", buf);
		if (out.send_frame_data(buf, ret) == false) {
			printf("send_frame_data error\r\n");
			return false;
		}
	}

#if 1
	if (!sendBannder(out)) {
		return false;
	}
#endif

	sleep(1);
	char info[256];
	snprintf(info, sizeof(info), "hello world!\r\n");
	out.reset().set_frame_fin(true)
		.set_frame_opcode(acl::FRAME_TEXT)
		.set_frame_payload_len(strlen(info));
	if (out.send_frame_data(info, strlen(info)) == false) {
		printf("send_frame_data error\r\n");
		return false;
	}

	sleep(1);
	snprintf(info, sizeof(info), "hello zsx!\r\n");
	out.reset().set_frame_fin(true)
		.set_frame_opcode(acl::FRAME_TEXT)
		.set_frame_payload_len(strlen(info));
	if (out.send_frame_data(info, strlen(info)) == false) {
		printf("send_frame_data error\r\n");
		return false;
	}

	sleep(1);
	snprintf(info, sizeof(info), "GoodBye!\r\n");
	out.reset().set_frame_fin(true)
		.set_frame_opcode(acl::FRAME_TEXT)
		.set_frame_payload_len(strlen(info));
	if (out.send_frame_data(info, strlen(info)) == false) {
		printf("send_frame_data error\r\n");
		return false;
	}

	return false;
}

bool http_servlet::sendBannder(acl::websocket& out)
{
	char banner[256];
	snprintf(banner, sizeof(banner), "Welcome!\r\n");

	for (int i = 0; i < 5; i++) {
		out.reset().set_frame_fin(true)
			.set_frame_opcode(acl::FRAME_TEXT)
			.set_frame_payload_len(strlen(banner));
		if (!out.send_frame_data(banner, strlen(banner))) {
			printf("send_frame_data error\r\n");
			return false;
		}
	}

	return true;
}

bool http_servlet::doWebSocket(acl::HttpServletRequest& req,
	acl::HttpServletResponse&)
{
	acl::socket_stream& ss = req.getSocketStream();
	acl::websocket in(ss), out(ss);

#if 1
	if (!sendBannder(out)) {
		return false;
	}
#endif

	while (true) {
		if (in.read_frame_head() == false) {
			printf("read_frame_head error\r\n");
			return false;
		}

		bool ret;
		unsigned char opcode = in.get_frame_opcode();

		printf("opcode: 0x%x\r\n", opcode);

		switch (opcode) {
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
			ret = doMsg(in, out);
			break;
		case acl::FRAME_BINARY:
			ret = doMsg(in, out);
			break;
		case acl::FRAME_CONTINUATION:
			ret = false;
			break;
		default:
			printf(">>got invalid\r\n");
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
	res.setContentType("text/html; charset=utf-8")	// 设置响应字符集
		.setContentEncoding(true)		// 设置是否压缩数据
		.setChunkedTransferEncoding(false);	// 采用 chunk 传输方式

	acl::string html_file;
	html_file << var_cfg_html_path << "/client.html";
	acl::string buf;
	if (acl::ifstream::load(html_file, &buf) == false) {
		logger_error("load %s error %s",
			html_file.c_str(), acl::last_serror());
		return doError(req, res);
	}

	// 发送 http 响应体，因为设置了 chunk 传输模式，所以需要多调用一次
	// res.write 且两个参数均为 0 以表示 chunk 传输数据结束
	return res.write(buf) && res.write(NULL, 0);
}
