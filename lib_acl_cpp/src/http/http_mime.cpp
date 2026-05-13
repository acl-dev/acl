#include "acl_stdafx.hpp"
#include "../mime/internal/mime_state.hpp"
#include "../mime/internal/header_opts.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stdlib/charset_conv.hpp"
#include "acl_cpp/stream/ifstream.hpp"
#include "acl_cpp/http/http_header.hpp"
#include "acl_cpp/http/http_mime.hpp"
#endif

#if !defined(ACL_MIME_DISABLE)

namespace acl {

//////////////////////////////////////////////////////////////////////////

http_mime_node::http_mime_node(const char* path, const MIME_NODE* node,
	bool decodeIt /* = true */, const char* toCharset /* =  */,
	off_t off /* = 0 */)
: mime_attach(path, node, decodeIt, toCharset, off)
{
	if (get_filename() == NULL) {
		mime_type_ = HTTP_MIME_PARAM;
		const char* name = get_name();
		if (path && name) {
			load_param(path);
		}
	} else {
		mime_type_ = HTTP_MIME_FILE;
	}
}

http_mime_node::~http_mime_node() {}

http_mime_t http_mime_node::get_mime_type() const {
	return mime_type_;
}

void http_mime_node::load_param(const char* path) {
	ifstream in;
	if (!in.open_read(path)) {
		logger_error("open file %s error(%s)", path, last_serror());
		return;
	}
	off_t begin = get_bodyBegin();
	off_t end = get_bodyEnd();
	if (begin < 0 || end < 0) {
		logger_error("invalid file offset, begin: %d, end: %d",
			(int) begin, (int) end);
		return;
	}

	// 如果 begin >= end 则说明该节点没有数据 -- zsx, 2019.7.9
	if (begin >= end) {
		return;
	}

	if (in.fseek(begin, SEEK_SET) == -1) {
		logger_error("fseek file %s error(%s), begin: %d",
			path, last_serror(), (int) begin);
		return;
	}

	size_t len = (size_t) (end - begin);
	char* buf = (char*) acl_mymalloc(len + 1);
	if (in.read(buf, len) == -1) {
		acl_myfree(buf);
		logger_error("read file %s error(%s)", path, last_serror());
		return;
	}
	buf[len] = 0;
	char* ptr = buf + len - 1;
	while (ptr >= buf) {
		if (*ptr == '\r' || *ptr == '\n' || *ptr == ' ' || *ptr == '\t') {
			*ptr-- = 0;
			continue;
		}

		break;
	}
	if (*buf == 0) {
		acl_myfree(buf);
		return;
	}

	char* value = acl_url_decode(buf, NULL);
	if (value == NULL) {
		value = buf;
	} else {
		acl_myfree(buf);
	}

	const char* fromCharset = get_charset();
	const char* toCharset   = get_toCharset();
	if (fromCharset && *fromCharset && toCharset && *toCharset
		  && strcasecmp(fromCharset, toCharset) != 0) {

		charset_conv conv;
		string tmp;
		if (conv.convert(fromCharset, toCharset, value, strlen(value), &tmp)) {

			param_value_ = tmp.c_str();
			acl_myfree(value);
		} else {
			param_value_ = value;
		}
	} else {
		param_value_ = value;
	}
}

const char* http_mime_node::get_value() const {
	return param_value_.empty() ? NULL : param_value_.c_str();
}

//////////////////////////////////////////////////////////////////////////

http_mime::http_mime(const char* boundary,
	const char* local_charset /* = "gb2312" */) {
	assert(boundary && *boundary);
	boundary_ = boundary;

	if (local_charset && *local_charset) {
		safe_snprintf(local_charset_, sizeof(local_charset_),
			"%s", local_charset);
	} else {
		local_charset_[0] = 0;
	}

	decode_on_ = true;

	save_path_.clear();
	mime_state_ = mime_state_alloc();

	static const char ctype_pre[] =
		"Content-Type: multipart/form-data; boundary=";

	// 为了使用邮件的 mime 解析器，需要模拟出一个头部字段
	mime_state_update(mime_state_, ctype_pre, sizeof(ctype_pre) - 1);
	size_t len = strlen(boundary);
	mime_state_update(mime_state_, boundary, (int) len);
	mime_state_update(mime_state_, "\r\n\r\n", 4);

	// 因为该头作为解析器的主头是额外加进去的，所以会造成实际的偏移量，
	// 通过 off_ 来进行偏移量补偿
	off_ = 0 - ((off_t) sizeof(ctype_pre) - 1 + (off_t) len + 4);

	parsed_ = false;
}

http_mime::~http_mime() {
	if (mime_state_) {
		mime_state_free(mime_state_);
	}
	for (std::list<http_mime_node*>::iterator it = mime_nodes_.begin();
		  it != mime_nodes_.end(); ++it) {
		delete *it;
	}
}

void http_mime::set_saved_path(const char* path) {
	if (path && *path) {
		save_path_ = path;
	}
}

bool http_mime::update(const char* data, size_t len) {
	return mime_state_update(mime_state_, data, (int) len) == 1;
}

const std::list<http_mime_node*>& http_mime::get_nodes() const {
	if (parsed_) {
		return mime_nodes_;
	}

	// 如果还没有分析完整就调用本函数，则直接返回空的集合
	if (mime_state_->curr_status != MIME_S_TERM) {
		return mime_nodes_;
	}

	const_cast<http_mime*>(this)->parsed_ = true;

	ACL_ITER iter;
	int  i = 0;
	acl_foreach(iter, mime_state_) {
		// 每一个节点是主头结点，所以跳过
		if (i++ == 0) {
			continue;
		}
		MIME_NODE *node = (MIME_NODE *) iter.data;
		const_cast<http_mime*>(this)->mime_nodes_.push_back(
			NEW http_mime_node(save_path_, node,
					decode_on_, local_charset_, off_));
	}

	return mime_nodes_;
}

const http_mime_node* http_mime::get_node(const char* name) const {
	get_nodes();

	for (std::list<http_mime_node*>::const_iterator cit = mime_nodes_.begin();
		  cit != mime_nodes_.end(); ++cit) {
		const char *ptr = (*cit)->get_name();
		if (ptr && strcmp(ptr, name) == 0) {
			return *cit;
		}
	}

	return NULL;
}

////////////////////////////////////////////////////////////////////////////////

http_mime& http_mime::add_param(const char* name, const char* value) {
	if (name && *name && value) {
		params_[name] = value;
	}
	return *this;
}

http_mime& http_mime::add_file(const char* name, const char* filepath) {
	if (name == NULL ||  *name == 0 || filepath == NULL) {
		return *this;
	}

	const char* filename = acl_safe_basename(filepath);
	files_[name] = std::make_pair(filename, filepath);
	return *this;
}

bool http_mime::save_to(const char *file_path) {
	fstream fp;
	if (!fp.open_trunc(file_path)) {
		logger_error("open %s failed(%s)", file_path, last_serror());
		return false;
	}
	return save_to(fp);
}

bool http_mime::save_to(ostream &out) {
	std::string bound("--");
	bound += boundary_;

	for (std::map<std::string, pair_value>::const_iterator it = files_.begin();
		  it != files_.end(); ++it) {
		if (!save_file(out, bound, *it)) {
			return false;
		}
	}

	for (std::map<std::string, std::string>::const_iterator it = params_.begin();
		  it != params_.end(); ++it) {
		if (!save_param(out, bound, *it)) {
			return false;
		}
	}

	bound += "--\r\n";
	if (out.write(bound.c_str(), bound.size()) != (int) bound.size()) {
		logger_error("write failed(%s)", last_serror());
		return false;
	}
	return true;
}

bool http_mime::save_param(ostream &out, const std::string& bound,
	  const std::pair<std::string, std::string>& param) {
	string buf;
	buf.copy(bound.c_str(), bound.size());
	buf.append("\r\n");

	const char disposition[] = "Content-Disposition: form-data; name=\"";
	buf.append(disposition, sizeof(disposition) - 1);
	if (!param.first.empty()) {
		buf.append(param.first.c_str(), param.first.size());
	}
	buf.append("\"\r\n\r\n");

	if (!param.second.empty()) {
		buf.append(param.second.c_str(), param.second.size());
	}
	buf.append("\r\n");

	if (out.write(buf) == false) {
		logger_error("write to file error: %s", last_serror());
		return false;
	}
	return true;
}

bool http_mime::save_file(ostream &out, const std::string& bound,
		const std::pair<std::string, pair_value>& file) {
	string buf;
	buf.copy(bound.c_str(), bound.size());
	buf.append("\r\n");

	const char disposition[] = "Content-Disposition: form-data; name=\"";
	buf.append(disposition, sizeof(disposition) - 1);
	if (!file.first.empty()) {
		buf.append(file.first.c_str(), file.first.size());
	}
	buf.append("\"; filename=\"");
	if (!file.second.first.empty()) {
		buf.append(file.second.first.c_str(), file.second.first.size());
	}
	buf.append("\"\r\n");

	buf.append("Content-Type: ");
	std::string ctype;
	get_ctype(file.second.first.c_str(), ctype);
	buf.append(ctype.c_str(), ctype.size());
	buf.append("\r\n\r\n");

	if (file.second.second.empty()) {
		buf.append("\r\n");
		if (out.write(buf) == false) {
			logger_error("write to file error: %s", last_serror());
			return false;
		}
		return true;
	}

	if (out.write(buf) == false) {
		logger_error("write to file error: %s", last_serror());
		return false;
	}

	const char* filepath = file.second.second.c_str();
	ifstream in;
	if (!in.open_read(filepath)) {
		logger_error("open %s failed(%s)", filepath, last_serror());
		return false;
	}

	char buffer[8192];
	while (!in.eof()) {
		const int ret = in.read(buffer, sizeof(buffer), false);
		if (ret == -1) {
			break;
		}
		if (out.write(buffer, ret) == -1) {
			logger_error("write to file error: %s", last_serror());
			return false;
		}
	}

	const char crln[] = "\r\n";
	if (out.write(crln, sizeof(crln) - 1) == -1) {
		logger_error("write to file error: %s", last_serror());
		return false;
	}
	return true;
}

void http_mime::get_ctype(const char *filename, std::string& ctype) {
	ctype = "application/octet-stream";
	const char* ext = strrchr(filename, '.');
	if (ext == NULL || *++ext == 0) {
		return;
	}

#define EQ !strcasecmp

	if (EQ(ext, "txt") || EQ(ext, "js") || EQ(ext, "css")) {
		ctype = "text/plain";
	} else if (EQ(ext, "html") || EQ(ext, "htm")) {
		ctype = "text/html";
	} else if (EQ(ext, "json")) {
		ctype = "application/json";
	} else if (EQ(ext, "png")) {
		ctype = "image/png";
	} else if (EQ(ext, "gif")) {
		ctype = "image/gif";
	} else if (EQ(ext, "jpg") || EQ(ext, "jpeg")) {
		ctype = "image/jpeg";
	}
}

} // namespace acl

#endif // !defined(ACL_MIME_DISABLE)
