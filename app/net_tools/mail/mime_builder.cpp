#include "StdAfx.h"
#include "global/global.h"
#include "mime_builder.hpp"

#ifdef WIN32
#include <process.h>
#define getpid _getpid
#endif

mime_builder::mime_builder()
: body_text_(NULL)
, body_html_(NULL)
{

}

mime_builder::~mime_builder()
{
	std::vector<char*>::iterator it = attachs_.begin();
	for (; it != attachs_.end(); ++it)
		acl_myfree(*it);
	if (body_text_)
		acl_myfree(body_text_);
	if (body_html_)
		acl_myfree(body_html_);
}

mime_builder& mime_builder::set_body_text(const char* src, size_t len)
{
	body_text_ = (char*) acl_mymemdup(src, len + 1);
	body_text_[len] = 0;
	return *this;
}

mime_builder& mime_builder::set_body_html(const char* src, size_t len)
{
	body_text_ = (char*) acl_mymemdup(src, len + 1);
	body_text_[len] = 0;
	return *this;
}

mime_builder& mime_builder::add_file(const char* filepath)
{
	attachs_.push_back(acl_mystrdup(filepath));
	return *this;
}

bool mime_builder::save_as(const char* to)
{
	acl::ofstream fp;
	if (fp.open_write(to) == false)
	{
		logger_error("open file %s failed %s", to, acl::last_serror());
		return false;
	}
	else if (save_as(fp) == false)
	{
		fp.close();
		_unlink(to);
		return false;
	}
	else
		return true;
}

bool mime_builder::save_as(acl::ofstream& fp)
{
	acl::string buf;
	if (attachs_.empty())
	{
		//header_.set_type(MIME_CTYPE_TEXT, body_html_ ?
		//	MIME_STYPE_HTML : MIME_STYPE_PLAIN);
		header_.set_type("text", body_html_ ? "html" : "plain");
		header_.add_header("Content-Transfer-Encoding", "base64");
	}
	else
	{
		//header_.set_type(MIME_CTYPE_MULTIPART,
		//	MIME_STYPE_MIXED);
		header_.set_type("multipart", "mixed");
		delimeter_.format("------=_Part_%d_%ld.%ld", getpid(),
			acl_pthread_self(), time(NULL));
		header_.set_boundary(delimeter_.c_str());
	}
	header_.build_head(buf, false);
	if (fp.write(buf) == -1)
	{
		logger_error("write head to file %s error %s",
			fp.file_path(), acl::last_serror());
		return false;
	}

	if (!delimeter_.empty())
	{
		if (add_boundary(fp) == false)
			return false;
	}
	if (add_body(fp) == false)
		return false;
	if (delimeter_.empty())
		return true;

	std::vector<char*>::const_iterator cit = attachs_.begin();
	for (; cit != attachs_.end(); ++cit)
	{
		if (add_boundary(fp) == false)
			return false;
		if (add_attach(fp, *cit) == false)
			return false;
	}

	// 添加最后一个分隔符
	return add_boundary(fp, true);
}

bool mime_builder::add_body(acl::ofstream& fp)
{
	acl::string buf;
	const char* ptr;
	size_t len;
	if (body_html_)
		ptr = body_html_;
	else
		ptr = body_text_;
	if (ptr == NULL)
	{
		logger_error("no body!");
		return false;
	}

	int ret = fp.format("Content-Type: text/%s; charset=\"utf-8\"\r\n"
		"Content-Transfer-Encoding: base64\r\n\r\n",
		body_html_ ? "html" : "plain");
	if (ret == -1)
	{
		logger_error("write body header to %s error %s",
			fp.file_path(), acl::last_serror());
		return false;
	}

	len = strlen(ptr);
	acl::mime_base64::encode(ptr, (int) len, &buf);
	if (fp.write(buf) == -1)
	{
		logger_error("write body to %s error %s",
			fp.file_path(), acl::last_serror());
		return false;
	}
	else if (fp.format("\r\n") == -1)
	{
		logger_error("write body endline to %s error %s",
			fp.file_path(), acl::last_serror());
		return false;
	}
	else
		return true;
}

bool mime_builder::add_boundary(acl::ofstream& fp, bool end /* = false */)
{
	if (fp.format("--%s", delimeter_.c_str()) == -1)
	{
		logger_error("write boundary error(%s) to %s",
			acl::last_serror(), fp.file_path());
		return false;
	}
	else if (end)
	{
		if (fp.format("--\r\n\r\n") == -1)
		{
			logger_error("write boundary endline error(%s) to %s",
				acl::last_serror(), fp.file_path());
			return false;
		}
		else
			return true;
	}
	else if (fp.format("\r\n") == -1)
	{
		logger_error("write boundary end error(%s) to %s",
			acl::last_serror(), fp.file_path());
		return false;
	}
	else
		return true;
}

bool mime_builder::add_attach(acl::ofstream& fp, const char* filepath)
{
	//////////////////////////////////////////////////////////////////////////
	// 创建头部分信息并写入邮件中

	acl::string filebuf;
	global::get_filename(filepath, filebuf);
	acl::string filename;
	acl::rfc2047::encode(filebuf.c_str(), (int) filebuf.length(),
		&filename, "utf-8", 'B', false);
	acl::string header;
	header.format("Content-Type: application/octet-stream;\r\n"
		"\tname=\"%s\r\n"
		"Content-Disposition: attachment;\r\n"
		"\tfilename=\"%s\"\r\n"
		"Content-Transfer-Encoding: base64\r\n\r\n",
		filename.c_str(), filename.c_str());
	if (fp.write(header) == -1)
	{
		logger_error("write header to file %s error %s",
			fp.file_path(), acl::last_serror());
		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	// 读取附件数据并写入邮件中

	acl::ifstream in;
	if (in.open_read(filepath) == false)
	{
		logger_error("open file %s error %s",
			filepath, acl::last_serror());
		return false;
	}

	acl::mime_base64 coder;

	char inbuf[8192];
	acl::string outbuf;
	int  ret;
	while (!in.eof())
	{
		ret = in.read(inbuf, sizeof(inbuf) - 1, false);
		if (ret == -1)
			break;
		coder.encode_update(inbuf, ret, &outbuf);
		if (!outbuf.empty() && fp.write(outbuf) == -1)
		{
			logger_error("write to %s error %s", fp.file_path(),
				acl::last_serror());
			return false;
		}
		outbuf.clear();
	}
	coder.encode_finish(&outbuf);
	if (outbuf.empty() == false && fp.write(outbuf) == -1)
	{
		logger_error("write to %s error %s", fp.file_path(),
			acl::last_serror());
		return false;
	}

	if (fp.format("\r\n") == -1)
	{
		logger_error("write endline to %s error %s", fp.file_path(),
			acl::last_serror());
		return false;
	}
	else
		return true;
}
