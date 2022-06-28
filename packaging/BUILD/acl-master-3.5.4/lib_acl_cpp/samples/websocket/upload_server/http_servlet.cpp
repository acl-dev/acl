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

bool http_servlet::doPost(acl::HttpServletRequest& req,
	acl::HttpServletResponse& res)
{
	res.setContentType("text/html; charset=utf-8")	// 设置响应字符集
		.setContentEncoding(true)		// 设置是否压缩数据
		.setChunkedTransferEncoding(true);	// 采用 chunk 传输方式

	const char* ip = req.getLocalAddr();
	if (ip == NULL || *ip == 0)
	{
		logger_error("getLocalAddr error");
		return false;
	}
	unsigned short port = req.getLocalPort();
	if (port == 0)
	{
		logger_error("getLocalPort error");
		return false;
	}

	acl::string local_addr;
	local_addr << ip << ":" << port;

	printf("getLocalAddr: %s\r\n", local_addr.c_str());

	acl::string html_file;
	html_file << var_cfg_html_path << "/upload.html";
	acl::string buf;
	if (acl::ifstream::load(html_file, &buf) == false)
	{
		logger_error("load %s error %s",
			html_file.c_str(), acl::last_serror());
		return doError(req, res);
	}

	buf << "<script>g_url='ws://" << local_addr << "/'</script>";

	// 发送 http 响应体，因为设置了 chunk 传输模式，所以需要多调用一次
	// res.write 且两个参数均为 0 以表示 chunk 传输数据结束
	return res.write(buf) && res.write(NULL, 0);
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
	acl::string filename;
	if (getFilename(in, filename) == false)
		return false;

	long long len = getFilesize(in);
	if (len <= 0)
		return false;

	if (saveFile(in, filename, len) == false)
		return false;

	printf("------------------- upload ok ---------------------\r\n");

	char res[256];
	snprintf(res, sizeof(res), "+ok");
	out.reset().set_frame_fin(true)
		.set_frame_opcode(acl::FRAME_TEXT)
		.set_frame_payload_len(strlen(res));
	if (out.send_frame_data(res, strlen(res)) == false)
	{
		printf("send_frame_data error\r\n");
		return false;
	}

	return true;
}

bool http_servlet::getFilename(acl::websocket& in, acl::string& filename)
{
	char buf[8192];
	int ret = in.read_frame_data(buf, sizeof(buf) - 1);
	if (ret < 0)
	{
		printf("read_frame_data for filename error\r\n");
		return false;
	}
	buf[ret] = 0;
	printf("read: [%s]\r\n", buf);
	filename = buf;
	return true;
}

long long http_servlet::getFilesize(acl::websocket& in)
{
	if (in.read_frame_head() == false)
	{
		printf("read_frame_head error\r\n");
		return -1;
	}

	unsigned char opcode = in.get_frame_opcode();
	switch (opcode)
	{
		case acl::FRAME_TEXT:
		case acl::FRAME_BINARY:
			break;
		default:
			printf("invalid opcode: 0x%0x\r\n", opcode);
			return -1;
	}

	char buf[256];
	int ret = in.read_frame_data(buf, sizeof(buf) - 1);
	if (ret < 0)
	{
		printf("read_frame_data for filesize error\r\n");
		return -1;
	}
	buf[ret] = 0;
	long long len = atoll(buf);
	printf("file size: %lld, buf: %s\r\n", len, buf);
	if (len <= 0)
	{
		printf("invalid file length: %lld\r\n", len);
		return -1;
	}

	return len;
}

bool http_servlet::saveFile(acl::websocket& in, const acl::string& filename,
	long long len)
{
	acl::string filepath, tmppath;
	filepath << var_cfg_upload_path << "/" << filename;
	tmppath << filepath << ".tmp";
	printf("%s, %s\n", tmppath.c_str(), filepath.c_str());

	acl::ofstream fp;
	if (fp.open_trunc(tmppath) == false)
	{
		printf("open_write %s error %s\r\n",
			tmppath.c_str(), acl::last_serror());
		return false;
	}

	if (saveFile(in, fp, len) == false)
	{
		fp.remove();
		return false;
	}

	fp.close();
	if (fp.rename(tmppath, filepath) == false)
	{
		fp.remove();
		logger_error("rename from %s to %s error %s",
			tmppath.c_str(), filepath.c_str(), acl::last_serror());
		return false;
	}

	return true;
}

bool http_servlet::saveFile(acl::websocket& in, acl::ofstream& fp,
	long long len)
{
	while (len > 0)
	{
		if (in.read_frame_head() == false)
		{
			printf("read_frame_head error\r\n");
			return false;
		}
		unsigned char opcode = in.get_frame_opcode();
		switch (opcode)
		{
		case acl::FRAME_TEXT:
		case acl::FRAME_BINARY:
			break;
		default:
			printf("invalid opcode: 0x%x\r\n", opcode);
			return false;
		}

		int ret = saveFile(in, fp);
		if (ret == 0)
			break;
		else if (ret < 0)
			return false;
		len -= ret;
	}

	return true;
}

int http_servlet::saveFile(acl::websocket& in, acl::ofstream& out)
{
	char  buf[8192];
	int   len = 0;

	while (true)
	{
		int ret = in.read_frame_data(buf, sizeof(buf));
		if (ret == 0)
			break;
		else if (ret < 0)
		{
			printf("read_frame_data error %s\r\n",
				acl::last_serror());
			return -1;
		}

		//printf(">>>read size: %d\r\n", ret);
		if (out.write(buf, ret) == -1)
		{
			printf("write to %s error %s\r\n",
				out.file_path(), acl::last_serror());
			return -1;
		}

		len += ret;
	}

	return len;
}

bool http_servlet::doWebSocket(acl::HttpServletRequest& req,
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
