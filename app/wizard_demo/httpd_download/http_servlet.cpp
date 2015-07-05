#include "stdafx.h"
#include "http_servlet.h"

http_servlet::http_servlet(const char* filepath)
{
	if (filepath && *filepath)
		filepath_ = filepath;
}

http_servlet::~http_servlet(void)
{

}

bool http_servlet::reply(acl::HttpServletRequest& req,
	acl::HttpServletResponse& res, int status, const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	acl::string  buf;
	buf.vformat(fmt, ap);
	va_end(ap);
	bool keep_alive = req.isKeepAlive();
	res.setStatus(status)
		.setKeepAlive(keep_alive)
		.setContentType("text/html; charset=utf-8")
		.setContentLength(buf.length());
	return res.write(buf) && keep_alive;
}

bool http_servlet::doError(acl::HttpServletRequest& req,
	acl::HttpServletResponse& res)
{
	acl::string hdr;
	req.getClient()->sprint_header(hdr);
	logger("error request head:\r\n%s\r\n", hdr.c_str());

	(void) reply(req, res, 400, "unknown request method\r\n");
	return false;
}

bool http_servlet::doUnknown(acl::HttpServletRequest& req,
	acl::HttpServletResponse& res)
{
	acl::string hdr;
	req.getClient()->sprint_header(hdr);
	logger("request head:\r\n%s\r\n", hdr.c_str());

	(void) reply(req, res, 400, "unknown request method\r\n");
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
	acl::string hdr;
	req.getClient()->sprint_header(hdr);
	logger("request head:\r\n%s\r\n", hdr.c_str());

	long long int range_from, range_to;
	if (req.getRange(range_from, range_to) == false)
		return transfer_file(req, res);
	else
		return transfer_file(req, res, range_from, range_to);
}

bool http_servlet::transfer_file(acl::HttpServletRequest& req,
	acl::HttpServletResponse& res)
{
	acl::ifstream in;

	// 只读方式打开文件流
	if (in.open_read(filepath_) == false)
	{
		logger_error("open file %s error %s", filepath_.c_str(),
			acl::last_serror());
		return reply(req, res, 500, "open %s error %s",
				filepath_.c_str(), acl::last_serror());
	}

	long long int fsize = in.fsize();
	if (fsize <= 0)
		return reply(req, res, 500, "invalid file size: %lld", fsize);

	bool keep_alive = req.isKeepAlive();

	// 设置 HTTP 响应头中的字段
	res.setStatus(200)
		.setKeepAlive(keep_alive)
		.setContentLength(fsize)
		.setContentType("application/octet-stream");

	acl::string hdr;
	res.getHttpHeader().build_response(hdr);
	logger("response head:\r\n%s\r\n", hdr.c_str());

	long long n = 0;
	char  buf[8192];

	// 从文件流中读取数据并将数据发给客户端
	while (!in.eof())
	{
		int ret = in.read(buf, sizeof(buf), false);
		if (ret == -1)
			break;
		if (res.write(buf, ret) == false)
			return false;
		n += ret;
		//acl_doze(100);  // 休息 100 ms 便于测试
	}

	return n == fsize ? true : false;
}

// 支持断点续传的数据传输过程
bool http_servlet::transfer_file(acl::HttpServletRequest& req,
	acl::HttpServletResponse& res, long long range_from, long long range_to)
{
	if (range_from < 0)
		return reply(req, res, 400, "invalid range_from: %lld",
				range_from);
	if (range_to > 0 && range_to < range_from)
		return reply(req, res, 400, "range_from(%lld) > range_to(%lld)",
				range_from, range_to);

	long long length;
	if (range_to >= range_from)
		length = range_to - range_from + 1;
	else
		length = -1;

	acl::ifstream in;
	// 只读方式打开文件流
	if (in.open_read(filepath_) == false)
	{
		logger_error("open file %s error %s", filepath_.c_str(),
			acl::last_serror());
		return reply(req, res, 500, "open %s error %s",
			filepath_.c_str(), acl::last_serror());
	}

	long long int fsize = in.fsize();
	if (fsize <= 0)
		return reply(req, res, 500, "invalid file size: %lld", fsize);

	// 如果偏移位置超过了文件总长度，则返回错误
	if (range_to >= fsize)
		return reply(req, res, 400, "range_to(%lld) >= fsize(%lld)",
			range_to, fsize);

	// 如果客户端要求从指定偏移位置至文件尾，则重新计算需要读取的数据长度，
	// 此值作为实际要传输给客户端的长度
	if (length == -1)
		length = fsize - range_from;

	// 定位文件偏移位置
	if (in.fseek(range_from, SEEK_SET) < 0)
		return reply(req, res, 500, "fseek(%lld) error %s",
			range_from, acl::last_serror());

	bool keep_alive = req.isKeepAlive();

	// 设置 HTTP 响应头中的字段
	res.setStatus(206)			// 响应状态必须为 206
		.setKeepAlive(keep_alive)	// 是否保持长连接
		.setContentLength(length)	// 实际要传输的数据长度
		.setContentType("application/octet-stream");  // 数据类型

	// 设置分段传输数据的范围
	res.getHttpHeader()
		// 设置本次传输区间
		.set_range(range_from, range_to > 0 ? range_to : fsize - 1)
		// 设置数据总长度
		.set_range_total(fsize);

	acl::string hdr;
	res.getHttpHeader().build_response(hdr);
	logger("response head:\r\n%s\r\n", hdr.c_str());

	char  buf[8192];
	int   ret;
	size_t size;

	while (!in.eof() && length > 0)
	{
		size = sizeof(buf) > (size_t) length ? (size_t) length : sizeof(buf);
		ret = in.read(buf, size, false);
		if (ret == -1)
		{
			printf("read over: %s\r\n", acl::last_serror());
			break;
		}
		if (res.write(buf, ret) == false)
			return false;
		length -= ret;
		//acl_doze(100);  // 休息 100 ms 便于测试
	}

	if (length != 0)
		return false;
	return true;
}
