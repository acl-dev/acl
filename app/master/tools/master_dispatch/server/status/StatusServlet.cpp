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
	bool use_xml, xml_meta = true;
	const char* ptr = req.getParameter("type");
	if (ptr && strcasecmp(ptr, "xml") == 0)
	{
		use_xml = true;
		res.setContentType("text/xml; charset=utf-8");
		ptr = req.getParameter("xml_meta");
		if (ptr != NULL && strcasecmp(ptr, "false") == 0)
			xml_meta = false;
		else
			xml_meta = true;
	}
	else
	{
		use_xml = false;
		xml_meta = false;
		res.setContentType("text/json; charset=utf-8");
	}

	res.setChunkedTransferEncoding(true);

	keep_alive_ = req.isKeepAlive() ? true : false;
	res.setKeepAlive(keep_alive_);

	// 调用单例服务器状态方法获得后端服务子进程实例的状态
	acl::string buf;
	if (use_xml)
	{
		if (xml_meta)
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
