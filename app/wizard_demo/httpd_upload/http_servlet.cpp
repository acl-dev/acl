#include "stdafx.h"
#include "master_service.h"
#include "http_servlet.h"
#if defined(_WIN32) || defined(_WIN64)
#include <process.h>
#endif

static acl::atomic_long __counter = 0;

http_servlet::http_servlet(acl::socket_stream* stream, acl::session* session)
: HttpServlet(stream, session)
, uploading_(false)
, req_(NULL)
, res_(NULL)
, content_length_(0)
, read_length_(0)
, mime_(NULL)
, fsize1_(-1)
, fsize2_(-1)
, fsize3_(-1)
{
	handlers_["/upload"] = &http_servlet::onUpload;
}

http_servlet::~http_servlet(void)
{
}

bool http_servlet::doError(request_t&, response_t& res)
{
	res.setStatus(400);
	res.setContentType("text/xml; charset=utf-8");
	// 发送 http 响应头
	if (!res.sendHeader()) {
		return false;
	}

	// 发送 http 响应体
	acl::string buf;
	buf.format("<root error='some error happened!' />\r\n");
	(void) res.getOutputStream().write(buf);
	return false;
}

bool http_servlet::doOther(request_t&, response_t& res, const char* method)
{
	res.setStatus(400);
	res.setContentType("text/xml; charset=utf-8");
	// 发送 http 响应头
	if (!res.sendHeader()) {
		return false;
	}
	// 发送 http 响应体
	acl::string buf;
	buf.format("<root error='unkown request method %s' />\r\n", method);
	(void) res.getOutputStream().write(buf);
	return false;
}

// 业务逻辑入口
bool http_servlet::run(void)
{
	// 如果正在读取客户端上传的数据，则进入上传处理过程
	if (uploading_) {
		if (req_ == NULL) {
			logger_error("req_ null");
			return false;
		}
		if (res_ == NULL) {
			logger_error("res_ null");
			return false;
		}

		return doUpload(*req_, *res_);
	}
	// 否则，走正常的HTTP处理流程，doGet/doPost 将会被调用
	else {
		return doRun();
	}
}

bool http_servlet::doGet(request_t& req, response_t& res)
{
	return doPost(req, res);
}

bool http_servlet::doPost(request_t& req, response_t& res)
{
	const char* path = req.getPathInfo();
	handler_t handler = path && *path ? handlers_[path] : NULL;
	return handler ? (this->*handler)(req, res) : onPage(req, res);
}

// 缺省 HTTP 请求，将 upload.html 页面返回给 HTTP 客户端
bool http_servlet::onPage(request_t& req, response_t& res)
{
	res.setContentType("text/html; charset=utf-8")	// 设置响应字符集
		.setKeepAlive(req.isKeepAlive())	// 设置是否保持长连接
		.setContentEncoding(true)		// 自动支持压缩传输
		.setChunkedTransferEncoding(true);	// 采用 chunk 传输方式

	acl::string buf;
	if (!acl::ifstream::load(var_cfg_page_html, &buf)) {
		buf.format("load %s error %s",
			var_cfg_page_html, acl::last_serror());
	}

	res.setContentLength(buf.size());
	return res.write(buf) && res.write(NULL, 0);
}

bool http_servlet::onUpload(request_t& req, response_t& res)
{
	res.setContentType("text/xml; charset=utf-8")	// 设置响应字符集
		.setKeepAlive(req.isKeepAlive())	// 设置是否保持长连接
		.setContentEncoding(true)		// 自动支持压缩传输
		.setChunkedTransferEncoding(true);	// 采用 chunk 传输方式

	// 获得 HTTP 请求的数据类型，正常的参数类型，即 name&value 方式
	// 还是 MIME 数据类型，还是数据流类型
	acl::http_request_t request_type = req.getRequestType();
	if (request_type != acl::HTTP_REQUEST_MULTIPART_FORM) {
		logger_warn("should be acl::HTTP_REQUEST_MULTIPART_FORM");
		return onPage(req, res);
	}

	// 先获得 Content-Type 对应的 http_ctype 对象
	mime_ = req.getHttpMime();
	if (mime_ == NULL) {
		logger_error("http_mime null");
		return onPage(req, res);
	}

	// 获得数据体的长度
	content_length_ = req.getContentLength();
	if (content_length_ <= 0) {
		logger_error("body empty");
		return onPage(req, res);
	}

	acl::string path;
	long long n = ++__counter;
	long long i = __counter.value();
	printf("i=%lld, n=%lld\r\n", i, n);
#if defined(_WIN32) || defined(_WIN64)
	path.format("%s\\mime_file.%u.%lld",
		var_cfg_var_path, (unsigned) _getpid(), n);
#else
	path.format("%s/mime_file.%u.%lld",
		var_cfg_var_path, (unsigned) getpid(), n);
#endif

	acl::meter_time(__FUNCTION__, __LINE__, "begin");

	if (fp_.open_write(path)) {
		// 设置原始文件存入路径
		mime_->set_saved_path(path);

		req_       = &req;
		res_       = &res;
		uploading_ = true;

		// 直接返回，从而触发异步读 HTTP 数据体过程
		return true;
	}

	logger_error("open %s error %s", path.c_str(), acl::last_serror());
	return doReply(req, res, "open file error");
}

void http_servlet::reset(void)
{
	uploading_      = false;
	req_            = NULL;
	res_            = NULL;
	content_length_ = 0;
	read_length_    = 0;
	mime_           = NULL;
	fp_.close();

	param1_.clear();
	param2_.clear();
	param3_.clear();
	file1_.clear();
	file2_.clear();
	file3_.clear();
	fsize1_ = -1;
	fsize2_ = -1;
	fsize3_ = -1;
}

bool http_servlet::doUpload(request_t& req, response_t& res)
{
	// 当未读完数据体时，需要异步读 HTTP 请求数据体
	if (content_length_ > read_length_) {
		if (!upload(req, res)) {
			return false;
		}
	}

	// 还没有读完上传的数据，需要返回，异步等待可读
	if (content_length_ > read_length_) {
		return true;
	}

	// 当已经读完 HTTP 请求数据体时，则开始分析上传的数据
	bool ret = parse(req, res);

	// 处理完毕，需重置 HTTP 会话状态，以便于处理下一个 HTTP 请求
	reset();
	acl::meter_time(__FUNCTION__, __LINE__, "end");
	return ret;
}

#if 0

bool http_servlet::upload(request_t& req, response_t& res)
{
	// 获得输入流
	acl::istream& in = req.getInputStream();
	acl::string buf(8193);
	bool finish = false;

	//logger(">>>>>>>>>>read: %lld, total: %lld<<<<<",
	//	read_length_, content_length_);

	// 读取 HTTP 客户端请求数据
	while (content_length_ > read_length_) {
		if (!in.read_peek(buf, true)) {
			break;
		}

#if 0
		printf(">>>size: %ld, space: %ld\r\n",
			(long) buf.size(), (long) buf.capacity());
#endif

		if (fp_.write(buf) == -1) {
			logger_error("write error %s", acl::last_serror());
			(void) doReply(req, res, "write error");
			return false;
		}

		read_length_ += buf.size();

		// 将读得到的数据输入至解析器进行解析
		// 如果再读到多余数据，可以直接丢掉，不必再放入 mime 解析器中
		if (!finish && mime_->update(buf, buf.size())) {
			finish = true;
		}
	}

	if (in.eof()) {
		logger_error("read error from http client");
		return false;
	}

	return true;
}

#else

bool http_servlet::upload(request_t& req, response_t& res)
{
	// 获得输入流
	acl::istream& in = req.getInputStream();
	char buf[8192];
	bool finish = false;

	//logger(">>>>>>>>>>read: %lld, total: %lld<<<<<",
	//	read_length_, content_length_);

	// 读取 HTTP 客户端请求数据
	while (content_length_ > read_length_) {
		int ret = in.read_peek(buf, sizeof(buf));

		if (ret <= 0) {
			break;
		}

		//printf(">>>size: %d\r\n", ret);
		if (fp_.write(buf, ret) == -1) {
			logger_error("write error %s", acl::last_serror());
			(void) doReply(req, res, "write error");
			return false;
		}

		read_length_ += (size_t) ret;

		// 将读得到的数据输入至解析器进行解析
		// 如果再读到多余数据，可以直接丢掉，不必再放入 mime 解析器中
		if (!finish && mime_->update(buf, (size_t) ret)) {
			finish = true;
		}
	}

	if (in.eof()) {
		logger_error("read error from http client");
		return false;
	}

	return true;
}

#endif

bool http_servlet::parse(request_t& req, response_t& res)
{
	const char* ptr = req.getParameter("name1");
	if (ptr) {
		param1_ = ptr;
	}
	ptr = req.getParameter("name2");
	if (ptr) {
		param2_ = ptr;
	}
	ptr = req.getParameter("name3");
	if (ptr) {
		param3_ = ptr;
	}

	acl::string path;

	// 遍历所有的 MIME 结点，找出其中为文件结点的部分进行转储
	const std::list<acl::http_mime_node*>& nodes = mime_->get_nodes();
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

			// 有的浏览器（如IE）上传文件时会带着文件路径，所以
			// 需要先将路径去掉
			filename = acl_safe_basename(filename);
#if defined(_WIN32) || defined(_WIN64)
			path.format("%s\\%s", var_cfg_var_path, filename);
#else
			path.format("%s/%s", var_cfg_var_path, filename);
#endif
			(void) (*cit)->save(path.c_str());

			if (strcmp(name, "file1") == 0) {
				file1_ = filename;
				fsize1_ = get_fsize(var_cfg_var_path, filename);
			} else if (strcmp(name, "file2") == 0) {
				file2_ = filename;
				fsize2_ = get_fsize(var_cfg_var_path, filename);
			} else if (strcmp(name, "file3") == 0) {
				file3_ = filename;
				fsize3_ = get_fsize(var_cfg_var_path, filename);
			}
		}
	}

	// 查找上载的某个文件并转储
	const acl::http_mime_node* node = mime_->get_node("file1");
	if (node && node->get_mime_type() == acl::HTTP_MIME_FILE) {
		ptr = node->get_filename();
		if (ptr) {
			// 有的浏览器（如IE）上传文件时会带着文件路径，所以
			// 需要先将路径去掉
			ptr = acl_safe_basename(ptr);
#if defined(_WIN32) || defined(_WIN64)
			path.format("%s\\1_%s", var_cfg_var_path, ptr);
#else
			path.format("%s/1_%s", var_cfg_var_path, ptr);
#endif
			(void) node->save(path.c_str());
		}
	}

	return doReply(req, res, "OK");
}

bool http_servlet::doReply(request_t& req, response_t& res, const char* info)
{
	// 创建 xml 格式的数据体
	acl::xml1 body;

	body.get_root().add_child("root", true)
		.add_child("content_type", true)
			.add_attr("type", (int) req.getRequestType())
			.get_parent()
		.add_child("info", true)
			.set_text(info)
			.get_parent()
		.add_child("params", true)
			.add_child("param", true)
				.add_attr("name1", param1_)
			.get_parent()
			.add_child("param", true)
				.add_attr("name2", param2_)
			.get_parent()
			.add_child("param", true)
				.add_attr("name3", param3_)
			.get_parent()
		.add_child("files", true)
			.add_child("file", true)
				.add_attr("filename", file1_)
				.add_attr("fsize", fsize1_)
				.get_parent()
			.add_child("file", true)
				.add_attr("filename", file2_)
				.add_attr("fsize", fsize2_)
				.get_parent()
			.add_child("file", true)
				.add_attr("filename", file3_)
				.add_attr("fsize", fsize3_);
	acl::string buf;
	body.build_xml(buf);

	logger(">>%s<<", buf.c_str());
	return res.write(buf) && res.write(NULL, 0);
}

long long http_servlet::get_fsize(const char* dir, const char* filename)
{
	acl::string path;
#if defined(_WIN32) || defined(_WIN64)
	path.format("%s\\%s", dir, filename);
#else
	path.format("%s/%s", dir, filename);
#endif
	acl::ifstream in;
	if (in.open_read(path) == false) {
		logger_error("open %s error %s", path.c_str(), acl::last_serror());
		return -1;
	}
	return in.fsize();
}
