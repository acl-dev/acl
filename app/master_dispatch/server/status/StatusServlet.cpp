#include "stdafx.h"
#include "server/ServerManager.h"
#include "status/StatusServlet.h"

StatusServlet::StatusServlet()
	: keep_alive_(false)
{
}

StatusServlet::~StatusServlet()
{
}

bool StatusServlet::doGet(acl::HttpServletRequest& req,
	acl::HttpServletResponse& res)
{
	return doPost(req, res);
}

bool StatusServlet::doPost(acl::HttpServletRequest& req,
	acl::HttpServletResponse& res)
{
	bool use_xml;
	const char* type = req.getParameter("type");
	if (type && strcasecmp(type, "xml") == 0)
	{
		use_xml = true;
		res.setContentType("text/xml; charset=utf-8");
	}
	else
	{
		use_xml = false;
		res.setContentType("text/json; charset=utf-8");
	}

	res.setChunkedTransferEncoding(true);

	keep_alive_ = req.isKeepAlive() ? true : false;
	res.setKeepAlive(keep_alive_);

	// 调用单例服务器状态方法获得后端服务子进程实例的状态
	acl::string buf;
	if (use_xml)
	{
		buf << "<?xml version=\"1.0\"?>";
		ServerManager::get_instance().statusToXml(buf);
	}
	else
		ServerManager::get_instance().statusToJson(buf);

	buf += "\r\n";
	//printf(">>buf: %s\r\n", buf.c_str());

	if (res.write(buf) == false || res.write(0, 0) == false)
		keep_alive_ = false;
	return keep_alive_;
}
