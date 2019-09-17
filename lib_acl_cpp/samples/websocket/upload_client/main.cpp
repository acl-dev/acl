#include "stdafx.h"

static bool handshake(acl::socket_stream& conn)
{
	acl::http_request req(&conn);
	acl::http_header& hdr = req.request_header();
	hdr.set_ws_key("123456789")
		.set_ws_version(13)
		.set_upgrade("websocket")
		.set_keep_alive(true);

	if (!req.request(NULL, 0)) {
		printf("request error\r\n");
		return false;
	}

	int status = req.http_status();
	if (status != 101) {
		printf("invalid http status: %d\r\n", status);
		return false;
	}

	return true;
}

static bool send_file(acl::websocket& ws, const char* filepath)
{
	acl::ifstream in;
	if (!in.open_read(filepath)) {
		printf("open %s error %s\r\n", filepath, acl::last_serror());
		return false;
	}

	long long size = in.fsize();
	if (size <= 0) {
		printf("filename: %s, invalid size: %lld\r\n", filepath, size);
		return false;
	}

	acl::string buf;
	buf.basename(filepath);

	unsigned mask = ~0;
	ws.set_frame_fin(true)
		.set_frame_opcode(acl::FRAME_TEXT)
		.set_frame_payload_len(buf.size())
		.set_frame_masking_key(mask);

	if (!ws.send_frame_data(buf, buf.size())) {
		printf("send filenam error %s\r\n", acl::last_serror());
		return false;
	}

	buf.format("%lld", size);
	ws.reset().set_frame_fin(true)
		.set_frame_opcode(acl::FRAME_TEXT)
		.set_frame_payload_len(buf.size());
	if (!ws.send_frame_data(buf, buf.size())) {
		printf("send file size error %s\r\n", acl::last_serror());
		return false;
	}

	long long total = 0;
	char cbuf[128000];
	while (!in.eof()) {
		int ret = in.read(cbuf, sizeof(cbuf), false);
		if (ret == -1) {
			break;
		}

		printf(">>send %d\r\n", ret);
		ws.reset().set_frame_fin(true)
			.set_frame_opcode(acl::FRAME_BINARY)
			.set_frame_payload_len(ret);
		if (!ws.send_frame_data(cbuf, ret)) {
			printf("send data error %s\r\n", acl::last_serror());
			return false;
		}
		total += ret;
		if (total % 10240000 == 0) {
			sleep(1);
		}
	}

	printf(">>total send: %lld\r\n", total);
	return true;
}

static bool read_reply(acl::websocket& ws)
{
	if (!ws.read_frame_head()) {
		printf("read_frame_head error %s\r\n", acl::last_serror());
		return false;
	}

	char cbuf[1024];
	unsigned char opcode = ws.get_frame_opcode();
	switch (opcode) {
	case acl::FRAME_TEXT:
	case acl::FRAME_BINARY:
		break;
	default:
		printf("invalid opcode: 0x%x\r\n", opcode);
		return false;
	}

	int ret = ws.read_frame_data(cbuf, sizeof(cbuf) - 1);
	if (ret <= 0) {
		printf("read_frame_data error\r\n");
		return false;
	}
	cbuf[ret] = 0;
	printf("reply from server: %s, len: %d\r\n", cbuf, ret);

	return true;
}

static bool upload(const char* addr, const char* filepath)
{
	acl::socket_stream conn;
	if (!conn.open(addr, 30, 30)) {
		printf("connect %s error %s\r\n", addr, acl::last_serror());
		return false;
	}

	if (!handshake(conn)) {
		return false;
	}

	acl::websocket ws(conn);

	if (!send_file(ws, filepath)) {
		return false;
	}

	if (!read_reply(ws)) {
		return false;
	}
	return true;
}

static void usage(const char* proc)
{
	printf("usage: %s -h [help] -s server_addr -f filename\r\n", proc);
}

int main(int argc, char* argv[])
{
	// 初始化 acl 库
	acl::acl_cpp_init();
	acl::log::stdout_open(true);  // 日志输出至标准输出
	int ch;

	acl::string addr, filename;

	while ((ch = getopt(argc, argv, "hf:s:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'f':
			filename = optarg;
			break;
		case 's':
			addr = optarg;
			break;
		default:
			break;
		}
	}

	if (addr.empty() || filename.empty()) {
		usage(argv[0]);
		return 1;
	}

	upload(addr, filename);
	return 0;
}
