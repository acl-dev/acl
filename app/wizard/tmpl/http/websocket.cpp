#include "stdafx.h"
#include "websocket.h"

static bool ws_ping(acl::websocket& in, acl::websocket& out)
{
	unsigned long long len = in.get_frame_payload_len();
	if (len == 0) {
		if (out.send_frame_pong((const void*) NULL, 0) == false) {
			return false;
		} else {
			return true;
		}
	}

	out.reset().set_frame_fin(true)
		.set_frame_opcode(acl::FRAME_PONG)
		.set_frame_payload_len(len);

	char buf[4096];
	while (true) {
		int ret = in.read_frame_data(buf, sizeof(buf) - 1);
		if (ret == 0) {
			break;
		}

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

static bool ws_pong(acl::websocket& in, acl::websocket&)
{
	unsigned long long len = in.get_frame_payload_len();
	if (len == 0) {
		return true;
	}

	char buf[4096];
	while (true) {
		int ret = in.read_frame_data(buf, sizeof(buf) - 1);
		if (ret == 0) {
			break;
		}

		if (ret < 0) {
			printf("read_frame_data error\r\n");
			return false;
		}

		buf[ret] = 0;
		printf("read: [%s]\r\n", buf);
	}

	return true;
}

static bool ws_close(acl::websocket&, acl::websocket&)
{
	return false;
}

static bool ws_msg(acl::websocket& in, acl::websocket& out)
{
	acl::string tbuf((size_t) in.get_frame_payload_len() + 1);

	char buf[4096];
	while (true) {
		int ret = in.read_frame_data(buf, sizeof(buf) - 1);
		if (ret == 0) {
			break;
		}

		if (ret < 0) {
			printf("read_frame_data error\r\n");
			return false;
		}

		tbuf.append(buf, ret);
	}

	printf(">>>%s\r\n", tbuf.c_str());

	// Just echo the received data to client.
	unsigned mask = ~0;
	out.reset()	// Must reset the websocket before each writing.
		.set_frame_fin(true)
		.set_frame_opcode(acl::FRAME_BINARY)
		.set_frame_payload_len(tbuf.size())
		.set_frame_masking_key(mask);

	if (out.send_frame_data(tbuf.c_str(), tbuf.size())) {
		printf("send_frame_data ok\r\n");
		return true;
	}

	tbuf += "\0";
	printf("send_frame_data error, data size=%zd\r\n", tbuf.size());
	return false;
}

bool websocket_run(HttpRequest& req, HttpResponse&)
{
	acl::socket_stream& conn = req.getSocketStream();
	acl::websocket in(conn), out(conn);

	while (true) {
		if (!in.read_frame_head()) {
			printf("read_frame_head error\r\n");
			return false;
		}

		bool ret;
		unsigned char opcode = in.get_frame_opcode();
		switch (opcode) {
		case acl::FRAME_PING:
			ret = ws_ping(in, out);
			break;
		case acl::FRAME_PONG:
			ret = ws_pong(in, out);
			break;
		case acl::FRAME_CLOSE:
			ret = ws_close(in, out);
			break;
		case acl::FRAME_TEXT:
		case acl::FRAME_BINARY:
			ret = ws_msg(in, out);
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

		if (ret == false) {
			break;
		}
	}

	return false;
}
