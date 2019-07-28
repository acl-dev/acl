#include "stdafx.h"
#include "qr.h"
#include "http_servlet.h"

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
	res.setContentType("text/xml; charset=utf-8");

	// 发送 http 响应体
	acl::string buf;
	buf.format("<root error='some error happened!' />\r\n");
	res.write(buf);
	res.write(NULL, 0);
	return false;
}

bool http_servlet::doOther(acl::HttpServletRequest&,
	acl::HttpServletResponse& res, const char* method)
{
	res.setStatus(400);
	res.setContentType("text/xml; charset=utf-8");
	// 发送 http 响应体
	acl::string buf;
	buf.format("<root error='unkown request method %s' />\r\n", method);
	res.write(buf);
	res.write(NULL, 0);
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
	int errcode = QR_ERR_NONE;
	QRCode* qr = qrInit(10, QR_EM_8BIT, 2, -1, &errcode);
	if (qr == NULL)
	{
		logger_error("qrInit error!");
		return replyf(req, res, "qrInit error");
	}

	const char* url = req.getParameter("url");
	if (url == NULL || *url == 0)
		url = "http://www.qq.com";

	qrAddData(qr, (const qr_byte_t*) url, strlen(url));
	if (!qrFinalize(qr))
	{
		logger_error("qrFinalize error!");
		qrDestroy(qr);
		return replyf(req, res, "qrFinalize error");
	}

	int size = 0;
	qr_byte_t *buffer = qrSymbolToPNG(qr, 5, 5, &size);
	if (buffer == NULL)
	{
		logger_error("qrSymbolToPNG error");
		qrDestroy(qr);
		return replyf(req, res, "qrSymbolToPNG error");
	}

	res.setContentType("image/png")			// 设置响应字符集
		.setKeepAlive(req.isKeepAlive())	// 设置是否保持长连接
		.setContentEncoding(true)		// 自动支持压缩传输
		.setContentLength(size);
	bool ret = res.write(buffer, size) && res.write(NULL, 0);

	qrDestroy(qr);

	return ret;
}

bool http_servlet::replyf(acl::HttpServletRequest& req,
	acl::HttpServletResponse& res,
	const char* fmt, ...)
{
	acl::string buf;
	va_list ap;
	va_start(ap, fmt);
	buf.vformat(fmt, ap);
	va_end(ap);

	res.setContentType("text/plain; charset=utf-8")	// 设置响应字符集
		.setKeepAlive(req.isKeepAlive())	// 设置是否保持长连接
		.setContentEncoding(true)		// 自动支持压缩传输
		.setContentLength(buf.size());

	return res.write(buf) && res.write(NULL, 0);
}
