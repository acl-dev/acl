#include "stdafx.h"
#include "WebsocketServlet_impl.h"

WebsocketServlet_impl::WebsocketServlet_impl(acl::redis_client_cluster& cluster, size_t max_conns)
{
	// 创建 session 存储对象
	session_ = new acl::redis_session(cluster, max_conns);
	step_ = 0;
}

WebsocketServlet_impl::~WebsocketServlet_impl(void)
{
	delete session_;
}

bool WebsocketServlet_impl::doUnknown(acl::HttpServletRequest&,
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

bool WebsocketServlet_impl::doGet(acl::HttpServletRequest& req,
	acl::HttpServletResponse& res)
{
	return doPost(req, res);
}

bool WebsocketServlet_impl::doPost(acl::HttpServletRequest& req,
	acl::HttpServletResponse& res)
{
	res.setContentType("text/html; charset=utf-8")	// 设置响应字符集
		.setContentEncoding(false)		// 设置是否压缩数据
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
	html_file << "www/upload.html";
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

bool WebsocketServlet_impl::on_ping(const char *buf, unsigned long long  len)
{
	return send_pong();
}


bool WebsocketServlet_impl::on_pong(const char *buf, unsigned long long  len)
{
	return send_ping();
}
bool WebsocketServlet_impl::on_message(char *data, unsigned long long len, bool text)
{
	switch (step_)
	{
	case 0:
	{
		printf("FileName:%s\n",data);
		filename_.append(data, len);
		step_++;
	}
	break;
	case 1:
	{
		printf("FileSize:%s\n", data);
		filesize_ = std::strtol(data, 0, 10);
		step_++;
	}
	break;
	case 2:
	{
		if (!file_)
		{
			file_ = new acl::ofstream();
			file_->open_trunc(filename_);
		}
		file_->write(data, len);
		current_filesize_ += len;
		if (current_filesize_ == filesize_)
		{
			printf("upload done\n");

			file_->close();
			delete file_;
			file_ = NULL;
			filename_.clear();
			filesize_ = 0;
			current_filesize_ = 0;
			step_ = 0;
		}
	}
	break;
	default:
		break;
	}
	return true;
}
