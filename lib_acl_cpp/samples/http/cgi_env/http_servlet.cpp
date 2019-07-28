// http_servlet.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

using namespace acl;

//////////////////////////////////////////////////////////////////////////

class http_servlet : public HttpServlet
{
public:
	http_servlet(void)
	{

	}

	~http_servlet(void)
	{

	}

	virtual bool doUnknown(HttpServletRequest&, HttpServletResponse& res)
	{
		res.setStatus(400);
		res.setContentType("text/xml; charset=gb2312");
		// 发送 http 响应头
		if (res.sendHeader() == false)
			return false;
		// 发送 http 响应体
		string buf("<root error='unkown request method' />\r\n");
		(void) res.getOutputStream().write(buf);
		return false;
	}

	virtual bool doGet(HttpServletRequest& req, HttpServletResponse& res)
	{
		return doPost(req, res);
	}

	virtual bool doPost(HttpServletRequest& req, HttpServletResponse& res)
	{
		extern char **environ;
		if (environ == NULL)
			return doError(req, res);

		acl::string buf;
		for (size_t i = 0; environ[i] != NULL; i++)
			buf << environ[i] << "\r\n";

		// 创建 HTTP 响应头
		res.setStatus(200);
		res.setContentType("text/plain");
		res.setCharacterEncoding("gb2312");
		res.setContentEncoding(true);
		//res.setContentLength(buf.size());
		res.setChunkedTransferEncoding(true);

		return res.write(buf) && res.write(NULL, 0);
	}
};

//////////////////////////////////////////////////////////////////////////

static void do_run(socket_stream* stream)
{
	memcache_session session("127.0.0.1:11211");
	http_servlet servlet;
	servlet.setLocalCharset("gb2312");
	servlet.doRun(session, stream);
}

// 服务器方式运行时的服务类
class master_service : public master_proc
{
public:
	master_service() {}
	~master_service() {}
protected:
	virtual void on_accept(socket_stream* stream)
	{
		do_run(stream);
	}
};

// WEB 服务模式
static void do_alone(void)
{
	master_service service;
	printf("listen: 0.0.0.0:8888 ...\r\n");
	service.run_alone("0.0.0.0:8888", NULL, 0);  // 单独运行方式
}

// WEB CGI 模式
static void do_cgi(void)
{
	do_run(NULL);
}

int main(int argc, char* argv[])
{
#ifdef WIN32
	acl::acl_cpp_init();
#endif

	// 开始运行
	if (argc >= 2 && strcmp(argv[1], "alone") == 0)
		do_alone();
	else
		do_cgi();

	return 0;
}

