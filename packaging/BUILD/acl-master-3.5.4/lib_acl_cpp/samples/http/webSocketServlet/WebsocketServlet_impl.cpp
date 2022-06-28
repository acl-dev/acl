#include "stdafx.h"
#include "WebsocketServlet_impl.h"

WebsocketServlet_impl::WebsocketServlet_impl(acl::redis_client_cluster& cluster,
	size_t max_conns)
{
	// 创建 session 存储对象
	session_ = new acl::redis_session(cluster, max_conns);
	step_    = 0;
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
	if (ip == NULL || *ip == 0) {
		logger_error("getLocalAddr error");
		return false;
	}
	unsigned short port = req.getLocalPort();
	if (port == 0) {
		logger_error("getLocalPort error");
		return false;
	}

	acl::string local_addr;
	local_addr << ip << ":" << port;

	printf("getLocalAddr: %s\r\n", local_addr.c_str());

	acl::string html_file;
	html_file << "www/upload.html";
	acl::string buf;
	if (acl::ifstream::load(html_file, &buf) == false) {
		logger_error("load %s error %s",
			html_file.c_str(), acl::last_serror());
		return doError(req, res);
	}

	buf << "<script>g_url='ws://" << local_addr << "/'</script>";

	// 发送 http 响应体，因为设置了 chunk 传输模式，所以需要多调用一次
	// res.write 且两个参数均为 0 以表示 chunk 传输数据结束
	return res.write(buf) && res.write(NULL, 0);
}

bool WebsocketServlet_impl::onPing(unsigned long long, bool)
{
	return sendPong();
}


bool WebsocketServlet_impl::onPong(unsigned long long, bool)
{
	return sendPing();
}

bool WebsocketServlet_impl::onMessage(unsigned long long payload_len,
	bool text, bool finish)
{
	(void) text;
	(void) finish;

	switch (step_) {
	case 0:
		step_++;
		return getFilename(payload_len);
	case 1:
		step_++;
		if (getFilesize(payload_len) == false) {
			fp_.close();
			return false;
		}
		return true;
	case 2:
		if (saveFile(payload_len) == false) {
			fp_.close();
			return false;
		}
		return true;
	default:
		return true;
	}
}

bool WebsocketServlet_impl::getFilename(unsigned long long payload_len)
{
	if (readData(payload_len, filename_) == false) {
		printf("read filename error\r\n");
		return false;
	}

	printf("get filename=%s\r\n", filename_.c_str());
	if (fp_.open_trunc(filename_) == false) {
		printf("open %s error %s\r\n", filename_.c_str(),
			acl::last_serror());
		return false;
	}
	return true;
}

bool WebsocketServlet_impl::getFilesize(unsigned long long payload_len)
{
	acl::string buf;
	if (readData(payload_len, buf) == false) {
		printf("read filesize error\r\n");
		fp_.close();
		return false;
	}
	filesize_ = atoll(buf.c_str());
	nread_    = 0;
	printf("get filesize=%lld\r\n", filesize_);
	if (filesize_ <= 0) {
		fp_.close();
	}

	return filesize_ > 0;
}

bool WebsocketServlet_impl::readData(unsigned long long len, acl::string& path)
{
	char buf[8192];
	while (len > 0) {
		size_t n = len > sizeof(buf) ? sizeof(buf) : len;
		int ret = readPayload(buf, n);
		if (ret == -1) {
			printf("readPayload error\r\n");
			return false;
		}
		path.append(buf, ret);
		len -= ret;
	}

	return true;
}

bool WebsocketServlet_impl::saveFile(unsigned long long len)
{
	char buf[8192];
	while (nread_ < filesize_) {
		size_t left = (size_t) (filesize_ - nread_);
		size_t n = left > sizeof(buf) ? sizeof(buf) : left;
		int ret = readPayload(buf, n);
		if (ret == -1) {
			printf("readPayload error\r\n");
			return false;
		}
		if (ret == 0) {
			break;
		}

		nread_ += ret;
		if (fp_.write(buf, ret) == -1) {
			printf("write data to %s error %s\r\n",
				filename_.c_str(), acl::last_serror());
			return false;
		}
	}

	printf("file=%s, payload_len=%llu, filesize_=%lld, nread_=%lld\r\n",
		filename_.c_str(), len, filesize_, nread_);

	if (nread_ == filesize_) {
		printf("READ OVER FOR FILE %s\r\n", filename_.c_str());

		step_     = 0;
		filesize_ = 0;
		nread_    = 0;
		filename_.clear();
		fp_.close();
		return sendText("+ok");
	} else {
		return true;
	}
}
