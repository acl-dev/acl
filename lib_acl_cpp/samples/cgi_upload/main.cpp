// main.cpp : �������̨Ӧ�ó������ڵ㡣
//
#include "stdafx.h"
#include <assert.h>
#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"

using namespace acl;

//////////////////////////////////////////////////////////////////////////

class http_servlet : public HttpServlet
{
public:
	http_servlet(void)
	{
		param1_ = NULL;
		param2_ = NULL;
		param3_ = NULL;
		file1_ = NULL;
		file2_ = NULL;
		file3_ = NULL;
	}

	~http_servlet(void)
	{
	}

	// GET ����
	virtual bool doGet(HttpServletRequest& req, HttpServletResponse& res)
	{
		return doPost(req, res);
	}

	// POST ����
	virtual bool doPost(HttpServletRequest& req, HttpServletResponse& res)
	{
		// ��� session ����ڣ�������
#if 0
		const char* sid = req.getSession().getAttribute("sid");
		if (*sid == 0)
			req.getSession().setAttribute("sid", "xxxxxx");
#endif

		// ���� HTTP ��Ӧͷ
                res.addCookie("name1", "value1");
		res.addCookie("name2", "value2", ".test.com", "/", 3600 * 24);
//		res.setStatus(400);  // �������÷��ص�״̬��

		// ���ַ�ʽ�����������ַ���
		if (0)
			res.setContentType("text/xml; charset=gb2312");
		else
		{
			res.setContentType("text/xml");
			res.setCharacterEncoding("gb2312");
		}

		// ��� HTTP ������������ͣ������Ĳ������ͣ��� name&value ��ʽ
		// ���� MIME �������ͣ���������������
		http_request_t request_type = req.getRequestType();
		if (request_type == HTTP_REQUEST_NORMAL)
			return doParams(req, res);
		else if (request_type == HTTP_REQUEST_MULTIPART_FORM)
			return doUpload(req, res);
		assert(request_type == HTTP_REQUEST_OCTET_STREAM);
		return doOctetStream(req, res);
	}

	bool doResponse(HttpServletRequest& req, HttpServletResponse& res)
	{
		// �������������� cookie ֵ
		const char* cookie1 = req.getCookieValue("name1");
		const char* cookie2 = req.getCookieValue("name2");

		// ��� sid session ֵ
#if 0
		const char* sid = req.getSession().getAttribute("sid");
#else
		const char* sid = "test_sid";
#endif

		// ���� xml ��ʽ��������
		xml body;
		body.get_root().add_child("root", true)
			.add_child("content_type", true)
				.add_attr("type", (int) req.getRequestType())
				.get_parent()
			.add_child("sessions", true)
				.add_child("session", true)
					.add_attr("sid", sid ? sid : "null")
					.get_parent()
				.get_parent()
			.add_child("cookies", true)
				.add_child("cookie", true)
					.add_attr("name1", cookie1 ? cookie1 : "null")
					.get_parent()
				.add_child("cookie", true)
					.add_attr("name2", cookie2 ? cookie2 : "null")
					.get_parent()
				.get_parent()
			.add_child("params", true)
				.add_child("param", true)
					.add_attr("name1", param1_ ? param1_ : "null")
					.get_parent()
				.add_child("param", true)
					.add_attr("name2", param2_ ? param2_ : "null")
					.get_parent()
				.add_child("param", true)
					.add_attr("name3", param3_ ? param3_ : "null")
					.get_parent()
				.get_parent()
			.add_child("files", true)
				.add_child("file", true)
					.add_attr("filename", file1_ ? file1_ : "null")
					.get_parent()
				.add_child("file", true)
					.add_attr("filename", file2_ ? file2_ : "null")
					.get_parent()
				.add_child("file", true)
					.add_attr("filename", file3_ ? file3_ : "null");
		string buf;
		body.build_xml(buf);

		// ���� http ��Ӧͷ
		if (res.sendHeader() == false)
			return false;
		// ���� http ��Ӧ��
		if (res.getOutputStream().write(buf) == -1)
			return false;
		return true;
	}

	// GET ��ʽ�� POST ��ʽ�����㣺
	// Content-Type: application/x-www-form-urlencoded
	bool doParams(HttpServletRequest& req, HttpServletResponse& res)
	{
		param1_ = req.getParameter("name1");
		param2_ = req.getParameter("name2");
		param3_ = req.getParameter("name2");
		return doResponse(req, res);
	}

	// POST ��ʽ�����㣺
	// Content-Type: multipart/form-data; boundary=xxx
	bool doUpload(HttpServletRequest& req, HttpServletResponse& res)
	{
		// �Ȼ�� Content-Type ��Ӧ�� http_ctype ����
		http_mime* mime = req.getHttpMime();
		if (mime == NULL)
		{
			logger_error("http_mime null");
			return false;
		}

		// ���������ĳ���
		long long int len = req.getContentLength();
		if (len <= 0)
		{
			logger_error("body empty");
			return false;
		}

		// ���������
		istream& in = req.getInputStream();
		char  buf[8192];
		int   ret;
		bool  n = false;

		const char* filepath = "./var/mime_file";
		ofstream out;
		out.open_write(filepath);

		// ����ԭʼ�ļ�����·��
		mime->set_saved_path(filepath);

		// ��ȡ HTTP �ͻ�����������
		while (len > 0)
		{
			ret = in.read(buf, sizeof(buf), false);
			if (ret == -1)
			{
				logger_error("read POST data error");
				return false;
			}
			out.write(buf, ret);

			len -= ret;

			// �����õ����������������������н���
			if (mime->update(buf, ret) == true)
			{
				n = true;
				break;
			}
		}
		out.close();

		if (len != 0 || n == false)
			logger_warn("not read all data from client");

		param1_ = req.getParameter("name1");
		param2_ = req.getParameter("name2");
		param3_ = req.getParameter("name3");

		string path;

		// �������е� MIME ��㣬�ҳ�����Ϊ�ļ����Ĳ��ֽ���ת��
		const std::list<http_mime_node*>& nodes = mime->get_nodes();
		std::list<http_mime_node*>::const_iterator cit = nodes.begin();
		for (; cit != nodes.end(); ++cit)
		{
			const char* name = (*cit)->get_name();
			http_mime_t mime_type = (*cit)->get_mime_type();
			if (mime_type == HTTP_MIME_FILE)
			{
				const char* filename = (*cit)->get_filename();
				if (filename == NULL)
				{
					logger("filename null");
					continue;
				}

				if (strcmp(name, "file1") == 0)
					file1_ = filename;
				else if (strcmp(name, "file2") == 0)
					file2_ = filename;
				else if (strcmp(name, "file3") == 0)
					file3_ = filename;
				filename = acl_safe_basename(filename);
#ifdef WIN32
				path.format("var\\%s", filename);
#else
				path.format("./var/%s", filename);
#endif
				(void) (*cit)->save(path.c_str());
			}
		}

		// �������ص�ĳ���ļ���ת��
		const http_mime_node* node = mime->get_node("file1");
		if (node && node->get_mime_type() == HTTP_MIME_FILE)
		{
			const char* ptr = node->get_filename();
			if (ptr)
			{
				path.format("./var/1_%s", ptr);
				(void) node->save(path.c_str());
			}
		}
		return doResponse(req, res);
	}

	// POST ��ʽ�����㣺
	// Content-Type: application/octet-stream
	bool doOctetStream(HttpServletRequest&, HttpServletResponse&)
	{
		return false;
	}

protected:
private:
	const char* param1_;
	const char* param2_;
	const char* param3_;
	const char* file1_;
	const char* file2_;
	const char* file3_;
};

//////////////////////////////////////////////////////////////////////////

static void do_run(socket_stream* stream)
{
	memcache_session session("127.0.0.1:11211");
	http_servlet servlet;
	servlet.setLocalCharset("gb2312");
	servlet.doRun(session, stream);
}

// ��������ʽ����ʱ�ķ�����
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

// WEB ����ģʽ
static void do_alone(void)
{
	master_service service;
	acl::log::stdout_open(true);
	const char* addr = "0.0.0.0:8081";
	printf("listen: %s ...\r\n", addr);
	service.run_alone(addr, NULL, 1);  // �������з�ʽ
}

// WEB CGI ģʽ
static void do_cgi(void)
{
	do_run(NULL);
}

//////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
#ifdef WIN32
	acl::acl_cpp_init();
#endif

	// ��ʼ����
	if (argc >= 2 && strcmp(argv[1], "alone") == 0)
		do_alone();
	else
		do_cgi();

	return 0;
}

