#include "stdafx.h"
#include "master_service.h"
#include "http_servlet.h"

http_servlet::http_servlet(acl::socket_stream* stream, acl::session* session)
: acl::HttpServlet(stream, session)
{
	conns_ = new acl::http_request_pool(var_cfg_manager_addr, 100);
}

http_servlet::~http_servlet(void)
{
	delete conns_;
}

bool http_servlet::doReply(acl::HttpServletRequest&,
	acl::HttpServletResponse& res, int status, const char* fmt, ...)
{
	acl::string buf;
	va_list ap;
	va_start(ap, fmt);
	buf.vformat(fmt, ap);
	va_end(ap);

	res.setStatus(status);
	res.setContentLength(buf.size());
	return res.write(buf) && res.write(NULL, 0);
}

bool http_servlet::doReply(acl::HttpServletRequest& req,
	acl::HttpServletResponse& res, int status, acl::json& json)
{
	res.setStatus(status);
	res.setContentType("text/json");
	if (1)
		res.setKeepAlive(req.isKeepAlive());
	else
		res.setKeepAlive(false);
	const acl::string& data = json.to_string();
	res.setContentLength(data.size());

	return res.write(data) && res.write(NULL, 0);
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

bool http_servlet::doOther(acl::HttpServletRequest&,
	acl::HttpServletResponse& res, const char* method)
{
	res.setStatus(400);
	res.setContentType("text/html; charset=");
	// 发送 http 响应头
	if (res.sendHeader() == false)
		return false;
	// 发送 http 响应体
	acl::string buf;
	buf.format("<root error='unkown request method %s' />\r\n", method);
	(void) res.getOutputStream().write(buf);
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

	res.setCharacterEncoding("utf-8")		// 设置响应字符集
		.setKeepAlive(req.isKeepAlive())	// 设置是否保持长连接
		.setContentEncoding(false)		// 自动支持压缩传输
		.setChunkedTransferEncoding(false);	// 采用 chunk 传输方式

	acl::string path;
	path = req.getPathInfo();
	if (path.empty())
	{
		logger_error("getPathInfo NULL");
		return doReply(req, res, 400, "%s", "no PathInfo");
	}
	path.strip("..");
	if (path.empty())
	{
		logger_error("path empty");
		return doReply(req, res, 400, "%s", "path empty");
	}

	const std::vector<acl::string>& tokens = path.split2("/");
	// printf(">>>path: %s, size: %ld\r\n", path.c_str(), tokens.size());
	if (tokens.size() < 2 || !tokens[1].equal("website", false))
		return doApp(req, res);
	else
		return doDoc(req, res, path);
}

bool http_servlet::doApp(acl::HttpServletRequest& req,
	acl::HttpServletResponse& res)
{
	acl::http_request* conn = (acl::http_request *) conns_->peek();
	if (conn == NULL)
	{
		logger_error("no connection available, addr: %s",
			var_cfg_manager_addr);
		return doReply(req, res, 500, "%s", "no connection");
	}

	acl::http_header& hdr = conn->request_header();
	hdr.set_url(var_cfg_access_url)
		.set_keep_alive(true)
		.set_method(acl::HTTP_METHOD_GET)
		.accept_gzip(true);

	if (conn->request(NULL, 0) == false)
	{
		logger_error("send request to %s error", var_cfg_manager_addr);
		conns_->put(conn, false);
		return doReply(req, res, 505, "%s", "send request error");
	}

	acl::xml1 xml;
	char buf[8192];
	while (true)
	{
		int ret = conn->read_body(buf, sizeof(buf) - 1);
		if (ret == 0)
			break;
		else if (ret < 0)
		{
			conns_->put(conn, false);
			logger_error("read_body error from %s",
				var_cfg_manager_addr);
			return doReply(req, res, 505, "%s", "read_body error");
		}

		buf[ret] = 0;
		xml.update(buf);
	}

	conns_->put(conn, true);

	acl::json json;
	if (xmlToJson(xml, json) == false)
		return doReply(req, res, 505, "%s", "invalid xml data");

	return doReply(req, res, 200, json);
}

bool http_servlet::xmlToJson(acl::xml& xml, acl::json& json)
{
	/**
	 * 数据格式
	 * <servers>
	 *  <server conns='xxx' used='xxx' qlen='xxx' max_threads='xxx'
	 *    curr_threads='xxx' busy_threads='xxx' addr='xxx' load='xxx'>
	 *     <proc>
	 *       <conns>xxx</conns>
	 *       <used>xxx</used>
	 *       <pid>xxx</pid>
	 *       <max_threads>xxx</max_threads>
	 *       <curr_threads>xxx</curr_threads>
	 *       <busy_threads>xxx</busy_threads>
	 *       <qlen>xxx</qlen>
	 *       <type>xxx</type>
	 *     </proc>
	 *   </server>
	 * </servers>
	 */

	acl::xml_node& root = xml.get_root();
	acl::xml_node* servers = NULL;
	for (acl::xml_node* child = root.first_child();
		child != NULL; child = root.next_child())
	{
		const char* tag = child->tag_name();
		if (tag != NULL && strcasecmp(tag, "servers") == 0)
		{
			servers = child;
			break;
		}
	}

	if (servers == NULL)
	{
		logger_error("no servers found in: %s", xml.to_string());
		return false;
	}

	acl::json_node& a = json.create_array();
	json.get_root().add_child("servers", a);

	for (acl::xml_node* child = servers->first_child();
		child != NULL; child = servers->next_child())
	{
		const char* tag = child->tag_name();
		if (tag == NULL || strcasecmp(tag, "server") != 0)
			continue;

		const char* ptr = (*child)["conns"];
		if (ptr == NULL || *ptr == 0)
			continue;
		int conns = atoi(ptr);

		if ((ptr = (*child)["used"]) == NULL || *ptr == 0)
			continue;
		int used = atoi(ptr);

		if ((ptr = (*child)["qlen"]) == NULL || *ptr == 0)
			continue;
		int qlen = atoi(ptr);

		if ((ptr = (*child)["max_threads"]) == NULL || *ptr == 0)
			continue;
		int max_threads = atoi(ptr);

		if ((ptr = (*child)["curr_threads"]) == NULL || *ptr == 0)
			continue;
		int curr_threads = atoi(ptr);

		if ((ptr = (*child)["busy_threads"]) == NULL || *ptr == 0)
			continue;
		int busy_threads = atoi(ptr);

		const char* addr = (*child)["addr"];
		if (addr == NULL || *addr == 0)
			continue;

		if ((ptr = (*child)["load"]) == NULL || *ptr == 0)
			continue;
		double load = atof(ptr);

		acl::json_node& node = json.create_node();
		a.add_child(node);

		node.add_text("server", addr) 
			.add_number("conns", conns)
			.add_number("used", used)
			.add_number("qlen", qlen)
			.add_number("max_threads", max_threads)
			.add_number("curr_threads", curr_threads)
			.add_number("busy_threads", busy_threads)
			.add_double("load", load);
	}

	return true;
}

bool http_servlet::doDoc(acl::HttpServletRequest& req,
	acl::HttpServletResponse& res, const char* path)
{
	acl::string filepath(var_cfg_home_path);

	if (*(var_cfg_home_path + strlen(var_cfg_home_path) - 1) != '/'
		&& *path != '/')
	{
		filepath << '/';
	}

	filepath << path;

	if (*(path + strlen(path) - 1) == '/')
		filepath << "index.html";

	acl::ifstream in;
	if (in.open_read(filepath) == false)
	{
		logger_error("open %s error %s", filepath.c_str(),
			acl::last_serror());
		return doReply(req, res, 404, "%s", "Not found");
	}
	//printf("---open %s ok---\r\n", filepath.c_str());

	long long len = in.fsize();
	if (len <= 0)
	{
		logger_error("invalid fisze: %lld, file: %s",
			len, filepath.c_str());
		return doReply(req, res, 500, "%s", "Can't get file size");
	}
	res.setContentLength(len);

	acl::string ctype;
	getContentType(filepath, ctype);
	res.setContentType(ctype);

	char buf[8192];
	int  ret;
	long long n = 0;

	while (!in.eof())
	{
		if ((ret = in.read(buf, sizeof(buf), false)) == -1)
		{
			//logger_error("read from %s error %s", filepath.c_str(),
			//	acl::last_serror());
			break;
		}
		if (res.write(buf, ret) == false)
		{
			logger_error("write to client error, file %s",
				filepath.c_str());
			return false;
		}

		n += ret;
	}

	if (n != len)
	{
		logger_error("write length(%lld) != file size(%lld), file: %s",
			n, len, filepath.c_str());
		return false;
	}

	return res.write(NULL, 0);
}

void http_servlet::getContentType(const acl::string& filepath, acl::string& out)
{
	if (!filepath.rncompare(".js", sizeof(".js") - 1, false))
		out = "text/js";
	else if (!filepath.rncompare(".css", sizeof(".css") - 1, false))
		out = "text/css";
	else if (!filepath.rncompare(".html", sizeof(".html") - 1, false))
		out = "text/html";
	else if (!filepath.rncompare(".png", sizeof(".png") - 1, false))
		out = "image/png";
	else
		out = "application/octet-stream";
}
