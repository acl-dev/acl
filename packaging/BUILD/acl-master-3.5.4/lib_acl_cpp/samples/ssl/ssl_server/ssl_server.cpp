// main.cpp : 定义控制台应用程序的入口点。
//
#include "stdafx.h"
#include <assert.h>
#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"

#define USE_MBEDTLS

//////////////////////////////////////////////////////////////////////////

class http_servlet : public acl::HttpServlet
{
public:
	http_servlet()
	{
		param1_ = NULL;
		param2_ = NULL;
		param3_ = NULL;
		file1_ = NULL;
		file2_ = NULL;
		file3_ = NULL;
		first_ = true;
	}

	~http_servlet(void)
	{
	}

	// @override
	bool doError(acl::HttpServletRequest&, acl::HttpServletResponse&)
	{
		if (first_) {
			logger_error("first request error");
		}
		return false;
	}

	// GET 方法
	// @override
	bool doGet(acl::HttpServletRequest& req, acl::HttpServletResponse& res)
	{
		bool ret = doPost(req, res);
		if (ret == false) {
			logger_error("doPost error!");
		} else {
			logger("doPost OK!");
		}
		return ret;
	}

	// POST 方法
	// @override
	bool doPost(acl::HttpServletRequest& req, acl::HttpServletResponse& res)
	{
		first_ = false;

		logger("request one now");

		// 创建 HTTP 响应头
		res.addCookie("name1", "value1");
		res.addCookie("name2", "value2", ".test.com", "/", 3600 * 24);
		res.setChunkedTransferEncoding(true);
		res.setKeepAlive(true);
		//		res.setStatus(400);  // 可以设置返回的状态码

		// 两种方式都可以设置字符集
		if (0) {
			res.setContentType("text/xml; charset=gb2312");
		} else {
			res.setContentType("text/xml");
			res.setCharacterEncoding("gb2312");
		}

		// 获得 HTTP 请求的数据类型，正常的参数类型，即 name&value 方式
		// 还是 MIME 数据类型，还是数据流类型
		acl::http_request_t request_type = req.getRequestType();
		if (request_type == acl::HTTP_REQUEST_NORMAL) {
			return doParams(req, res);
		} else if (request_type == acl::HTTP_REQUEST_MULTIPART_FORM) {
			return doUpload(req, res);
		}
		assert(request_type == acl::HTTP_REQUEST_OCTET_STREAM);
		return doOctetStream(req, res);
	}

	bool doResponse(acl::HttpServletRequest& req, acl::HttpServletResponse& res)
	{
		// 获得浏览器传来的 cookie 值
		const char* cookie1 = req.getCookieValue("name1");
		const char* cookie2 = req.getCookieValue("name2");

		// 创建 xml 格式的数据体
		acl::xml1 body;
		body.get_root().add_child("root", true)
			.add_child("content_type", true)
				.add_attr("type", (int) req.getRequestType())
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
		acl::string buf("<?xml version=\"1.0\"?>");
		body.build_xml(buf);

		//printf(">>>response: %s\r\n", buf.c_str());
		//res.setContentLength(buf.length());

		// 不必显示工调用下面过程来发送 http 响应头
		//if (res.sendHeader() == false)
		//	return false;
		// 发送 http 响应体，当使用 chunk 传输时，必须最后调用一次发送空数据
		if (res.write(buf) == false || res.write(NULL, 0) == false) {
			return false;
		}
		return true;
	}

	// GET 方式或 POST 方式且满足：
	// Content-Type: application/x-www-form-urlencoded
	bool doParams(acl::HttpServletRequest& req, acl::HttpServletResponse& res)
	{
		param1_ = req.getParameter("name1");
		param2_ = req.getParameter("name2");
		param3_ = req.getParameter("name3");
		return doResponse(req, res);
	}

	// POST 方式且满足：
	// Content-Type: multipart/form-data; boundary=xxx
	bool doUpload(acl::HttpServletRequest& req, acl::HttpServletResponse& res)
	{
		// 先获得 Content-Type 对应的 http_ctype 对象
		acl::http_mime* mime = req.getHttpMime();
		if (mime == NULL) {
			logger_error("http_mime null");
			return false;
		}

		// 获得数据体的长度
		long long int len = req.getContentLength();
		if (len <= 0) {
			logger_error("body empty");
			return false;
		}

		// 获得输入流
		acl::istream& in = req.getInputStream();
		char  buf[8192];
		int   ret;
		bool  finish = false;

		const char* filepath = "./var/mime_file";
		acl::ofstream out;
		out.open_write(filepath);

		// 设置原始文件存入路径
		mime->set_saved_path(filepath);

		size_t k;

		// 读取 HTTP 客户端请求数据
		while (len > 0) {
			k = (size_t) len > sizeof(buf) ? sizeof(buf) : (size_t) len;
			ret = in.read(buf, k, false);
			if (ret == -1) {
				logger_error("read POST data error");
				return false;
			}
			out.write(buf, ret);

			len -= ret;

			// 将读得到的数据输入至解析器进行解析
			if (!finish && mime->update(buf, ret) == true) {
				finish = true;
			}
		}
		out.close();

		if (len != 0 || finish == false) {
			logger_warn("not read all data from client");
		}

		param1_ = req.getParameter("name1");
		param2_ = req.getParameter("name2");
		param3_ = req.getParameter("name3");

		acl::string path;

		// 遍历所有的 MIME 结点，找出其中为文件结点的部分进行转储
		const std::list<acl::http_mime_node*>& nodes = mime->get_nodes();
		std::list<acl::http_mime_node*>::const_iterator cit = nodes.begin();
		for (; cit != nodes.end(); ++cit) {
			const char* name = (*cit)->get_name();
			if (name == NULL) {
				continue;
			}

			acl::http_mime_t mime_type = (*cit)->get_mime_type();
			if (mime_type == acl::HTTP_MIME_FILE) {
				const char* filename = (*cit)->get_filename();
				if (filename == NULL) {
					logger("filename null");
					continue;
				}

				if (strcmp(name, "file1") == 0) {
					file1_ = filename;
				} else if (strcmp(name, "file2") == 0) {
					file2_ = filename;
				} else if (strcmp(name, "file3") == 0) {
					file3_ = filename;
				}

				// 有的浏览器（如IE）上传文件时会带着文件路径，所以
				// 需要先将路径去掉
				filename = acl_safe_basename(filename);
#ifdef WIN32
				path.format("var\\%s", filename);
#else
				path.format("./var/%s", filename);
#endif
				(void) (*cit)->save(path.c_str());
			}
		}

		// 查找上载的某个文件并转储
		const acl::http_mime_node* node = mime->get_node("file1");
		if (node && node->get_mime_type() == acl::HTTP_MIME_FILE) {
			const char* ptr = node->get_filename();
			if (ptr) {
				// 有的浏览器（如IE）上传文件时会带着文件路径，所以
				// 需要先将路径去掉
				ptr = acl_safe_basename(ptr);
#ifdef WIN32
				path.format(".\\var\\1_%s", ptr);
#else
				path.format("./var/1_%s", ptr);
#endif
				(void) node->save(path.c_str());
			}
		}
		return doResponse(req, res);
	}

	// POST 方式且满足：
	// Content-Type: application/octet-stream
	bool doOctetStream(acl::HttpServletRequest&, acl::HttpServletResponse&)
	{
		logger_error("not support now!");
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
	bool first_;
};

//////////////////////////////////////////////////////////////////////////

static void do_run(acl::socket_stream* stream)
{
	acl::memcache_session session("127.0.0.1:11211");
	http_servlet servlet;
	servlet.setLocalCharset("gb2312");
	servlet.doRun(session, stream);
}

// 服务器方式运行时的服务类
class master_service : public acl::master_proc
{
public:
	master_service(const char* crt_file, const char* key_file,
#ifdef USE_MBEDTLS
		acl::mbedtls_verify_t verify_mode)
#else
		acl::polarssl_verify_t verify_mode)
#endif
	{
		if (crt_file && *crt_file && key_file && *key_file) {
			crt_file_ = crt_file;
			key_file_ = key_file;
#ifdef USE_MBEDTLS
			conf_ = new acl::mbedtls_conf(true);
#else
			conf_ = new acl::polarssl_conf(true);
#endif
		} else {
			conf_ = NULL;
		}
		verify_mode_ = verify_mode;
	}

	~master_service()
	{
		delete conf_;
	}

protected:
	// @override
	void on_accept(acl::socket_stream* stream)
	{
		stream->set_rw_timeout(60);
		printf("accept one fd=%d\r\n", stream->sock_handle());

		if (conf_) {
			// 对于使用 SSL 方式的流对象，需要将 SSL IO 流对象注册
			// 至网络连接流对象中，即用 ssl io 替换 stream 中默认
			// 的底层 IO 过程

			logger("begin setup ssl hook...");

			acl::sslbase_io* ssl = conf_->create(false);
			if (stream->setup_hook(ssl) == ssl) {
				logger_error("setup_hook error!");
				ssl->destroy();
			} else {
				logger("setup ssl hook ok");
			}
		}

		do_run(stream);
	}

	// @override
	void proc_on_init()
	{
		if (conf_ == NULL) {
			return;
		}

		// 允许服务端的 SSL 会话缓存功能
		conf_->enable_cache(true);

		// 添加本地服务的证书
		if (conf_->add_cert(crt_file_.c_str()) == false) {
			logger_error("add cert failed, crt: %s, key: %s",
				crt_file_.c_str(), key_file_.c_str());
			delete conf_;
			conf_ = NULL;
			return;
		}
		logger("load cert ok, crt: %s", crt_file_.c_str());

		// 添加本地服务密钥
		if (conf_->set_key(key_file_.c_str()) == false) {
			logger_error("set private key error");
			delete conf_;
			conf_ = NULL;
			return;
		}
		logger("set key ok, key: %s", key_file_.c_str());

#ifndef USE_MBEDTLS
		conf_->set_authmode(verify_mode_);
#endif
	}

private:
#ifdef USE_MBEDTLS
	acl::mbedtls_conf*     conf_;
	acl::mbedtls_verify_t  verify_mode_;
#else
	acl::polarssl_conf*    conf_;
	acl::polarssl_verify_t verify_mode_;
#endif
	acl::string crt_file_;
	acl::string key_file_;
};

// WEB 服务模式
static void do_alone(const char* crt_file, const char* key_file,
#ifdef USE_MBEDTLS
	acl::mbedtls_verify_t verify_mode)
#else
	acl::polarssl_verify_t verify_mode)
#endif
{
	master_service service(crt_file, key_file, verify_mode);
	acl::log::stdout_open(true);
	const char* addr = "0.0.0.0:2443";
	printf("listen: %s ...\r\n", addr);
	service.run_alone(addr, NULL, 0);  // 单独运行方式
}

// WEB CGI 模式
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
	acl::log::stdout_open(true);

#ifdef USE_MBEDTLS
# ifdef __APPLE__
#  define USE_ONE
#  ifdef USE_ONE
	const char* libssl_path    = "../libmbedtls_all.dylib";
	acl::mbedtls_conf::set_libpath(libssl_path);
#  else
	const char* libcrypto_path = "../libmbedcrypto.dylib";
	const char* libx509_path   = "../libmbedx509.dylib";
	const char* libssl_path    = "../libmbedtls.dylib";
	acl::mbedtls_conf::set_libpath(libcrypto_path, libx509_path, libssl_path);
#  endif
# elif defined(_WIN32) || defined(_WIN64)
	const char* libssl_path    = "../mbedtls.dll";
	acl::mbedtls_conf::set_libpath(libssl_path);  // 一个总的动态库
# elif defined(__linux__)
#  define USE_ONE
#  ifdef USE_ONE
	const char* libssl_path    = "../libmbedtls_all.so";
	acl::mbedtls_conf::set_libpath(libssl_path);
#  else
	const char* libcrypto_path = "../libmbedcrypto.so";
	const char* libx509_path   = "../libmbedx509.so";
	const char* libssl_path    = "../libmbedtls.so";
	acl::mbedtls_conf::set_libpath(libcrypto_path, libx509_path, libssl_path);
#  endif
# endif
	if (!acl::mbedtls_conf::load()) {
		printf("load %s error\r\n", libssl_path);
		return 1;
	}
#else
# ifdef __APPLE__
	const char* libssl_path = "../libpolarssl.dylib";
	acl::polarssl_conf::set_libpath(libssl_path);
# else
	const char* libssl_path = "../libpolarssl.so";
	acl::polarssl_conf::set_libpath(libssl_path);
# endif
	if (!acl::polarssl_conf::load()) {
		printf("load %s error\r\n", libssl_path);
		return 1;
	}
#endif

	// 开始运行
	if (argc >= 2 && strcmp(argv[1], "alone") == 0) {
#ifdef USE_MBEDTLS
		acl::mbedtls_verify_t verify_mode;
#else
		acl::polarssl_verify_t verify_mode;
#endif
		const char* crt_file, *key_file;

		if (argc >= 3) {
			crt_file = argv[2];
		} else {
			crt_file = NULL;
		}
		if (argc >= 4) {
			key_file = argv[3];
		} else {
			key_file = NULL;
		}
#ifdef USE_MBEDTLS
		if (argc >= 5) {
			if (strcasecmp(argv[4], "req") == 0) {
				verify_mode = acl::MBEDTLS_VERIFY_REQ;
			} else if (strcasecmp(argv[4], "opt") == 0) {
				verify_mode = acl::MBEDTLS_VERIFY_OPT;
			} else {
				verify_mode = acl::MBEDTLS_VERIFY_NONE;
			}
		} else {
			verify_mode = acl::MBEDTLS_VERIFY_NONE;
		}
#else
		if (argc >= 5) {
			if (strcasecmp(argv[4], "req") == 0) {
				verify_mode = acl::POLARSSL_VERIFY_REQ;
			} else if (strcasecmp(argv[4], "opt") == 0) {
				verify_mode = acl::POLARSSL_VERIFY_OPT;
			} else {
				verify_mode = acl::POLARSSL_VERIFY_NONE;
			}
		} else {
			verify_mode = acl::POLARSSL_VERIFY_NONE;
		}
#endif

		do_alone(crt_file, key_file, verify_mode);
	} else if (argc >= 2) {
		printf("usage: %s alone cert_file key_file verify[none|req|opt]\r\n", argv[0]);
	} else {
		do_cgi();
	}

	return 0;
}
