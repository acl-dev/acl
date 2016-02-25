#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include <list>
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl_cpp/stream/ofstream.hpp"
#include "acl_cpp/stream/ifstream.hpp"
#include "acl_cpp/stdlib/thread.hpp"
#include "acl_cpp/mime/rfc822.hpp"
#include "acl_cpp/mime/rfc2047.hpp"
#include "acl_cpp/mime/mime_base64.hpp"
#include "acl_cpp/smtp/mail_attach.hpp"
#include "acl_cpp/smtp/mail_body.hpp"
#include "acl_cpp/smtp/mail_message.hpp"
#endif

namespace acl
{

mail_message::mail_message(const char* charset /* = "utf-8"*/)
{
	dbuf_ = new dbuf_pool;
	if (charset == NULL || *charset == 0)
		charset = "utf-8";
	charset_ = dbuf_->dbuf_strdup(charset);
	transfer_encoding_ = dbuf_->dbuf_strdup("base64");
	auth_user_ = NULL;
	auth_pass_ = NULL;
	from_ = NULL;
	sender_ = NULL;
	reply_to_ = NULL;
	return_path_ = NULL;
	delivered_to_ = NULL;
	subject_ = NULL;
	body_ = NULL;
	body_len_ = 0;
	filepath_ = NULL;
}

mail_message::~mail_message()
{
	std::vector<mail_attach*>::iterator it;
	for (it = attachments_.begin(); it != attachments_.end(); ++it)
		(*it)->~mail_attach();
	dbuf_->destroy();
}

mail_message& mail_message::set_auth(const char* user, const char* pass)
{
	if (user && *user && pass && *pass)
	{
		auth_user_ = dbuf_->dbuf_strdup(user);
		auth_pass_ = dbuf_->dbuf_strdup(pass);
	}
	return *this;
}

mail_message& mail_message::set_from(const char* from, const char* name)
{
	if (from == NULL || *from == 0)
		return *this;
	from_ = (rfc822_addr*) dbuf_->dbuf_alloc(sizeof(rfc822_addr));
	from_->addr = dbuf_->dbuf_strdup(from);
	if (name && *name)
		from_->comment = dbuf_->dbuf_strdup(name);
	else
		from_->comment = NULL;
	return *this;
}

mail_message& mail_message::set_sender(const char* sender, const char* name)
{
	if (sender == NULL || *sender == 0)
		return *this;
	sender_ = (rfc822_addr*) dbuf_->dbuf_alloc(sizeof(rfc822_addr));
	sender_->addr = dbuf_->dbuf_strdup(sender);
	if (name && *name)
		sender_->comment = dbuf_->dbuf_strdup(name);
	else
		sender_->comment = NULL;
	return *this;
}

mail_message& mail_message::set_reply_to(const char* replyto, const char* name)
{
	if (replyto == NULL || *replyto == 0)
		return *this;
	reply_to_ = (rfc822_addr*) dbuf_->dbuf_alloc(sizeof(rfc822_addr));
	reply_to_->addr = dbuf_->dbuf_strdup(replyto);
	if (name && *name)
		reply_to_->comment = dbuf_->dbuf_strdup(name);
	else
		reply_to_->comment = NULL;
	return *this;
}

mail_message& mail_message::set_return_path(const char* return_path)
{
	if (return_path == NULL || *return_path == 0)
		return *this;
	return_path_ = (rfc822_addr*) dbuf_->dbuf_alloc(sizeof(rfc822_addr));
	return_path_->addr = dbuf_->dbuf_strdup(return_path);
	return_path_->comment = NULL;
	return *this;
}

mail_message& mail_message::set_delivered_to(const char* delivered_to)
{
	if (delivered_to == NULL || *delivered_to == 0)
		return *this;
	delivered_to_ = (rfc822_addr*) dbuf_->dbuf_alloc(sizeof(rfc822_addr));
	delivered_to_->addr = dbuf_->dbuf_strdup(delivered_to);
	delivered_to_->comment = NULL;
	return *this;
}

void mail_message::add_addrs(const char* in, std::vector<rfc822_addr*>& out)
{
	rfc822 rfc;
	const std::list<rfc822_addr*>& addrs = rfc.parse_addrs(in, charset_);
	std::list<rfc822_addr*>::const_iterator cit = addrs.begin();
	for (; cit != addrs.end(); ++cit)
	{
		rfc822_addr* addr = (rfc822_addr* )
			dbuf_->dbuf_alloc(sizeof(rfc822_addr));
		if ((*cit)->addr == NULL)
			continue;
		addr->addr = dbuf_->dbuf_strdup((*cit)->addr);
		if ((*cit)->comment)
			addr->comment = dbuf_->dbuf_strdup((*cit)->comment);
		else
			addr->comment = NULL;
		out.push_back(addr);
	}
}

mail_message& mail_message::add_recipients(const char* recipients)
{
	string buf(recipients);
	std::list<string>& tokens = buf.split(" \t;,");
	std::list<string>::const_iterator cit;
	for (cit = tokens.begin(); cit != tokens.end(); ++cit)
		(void) add_to((*cit).c_str());
	return *this;
}

mail_message& mail_message::add_to(const char* to)
{
	if (to && *to)
	{
		add_addrs(to, to_list_);
		add_addrs(to, recipients_);
	}
	return *this;
}

mail_message& mail_message::add_cc(const char* cc)
{
	if (cc && *cc)
	{
		add_addrs(cc, to_list_);
		add_addrs(cc, recipients_);
	}
	return *this;
}

mail_message& mail_message::add_bcc(const char* bcc)
{
	if (bcc && *bcc)
	{
		add_addrs(bcc, to_list_);
		add_addrs(bcc, recipients_);
	}
	return *this;
}

mail_message& mail_message::set_subject(const char* subject)
{
	if (subject && *subject)
		subject_ = dbuf_->dbuf_strdup(subject);
	return *this;
}

mail_message& mail_message::add_header(const char* name, const char* value)
{
	if (name == NULL || *name == 0 || value == NULL || *value == 0)
		return *this;
	char* n = dbuf_->dbuf_strdup(name);
	char* v = dbuf_->dbuf_strdup(value);
	headers_.push_back(std::make_pair(n, v));
	return *this;
}

const char* mail_message::get_header_value(const char* name) const
{
	std::vector<std::pair<char*, char*> >::const_iterator cit;
	for (cit = headers_.begin(); cit != headers_.end(); ++cit)
	{
		if (strcasecmp((*cit).first, name) == 0)
			return (*cit).second;
	}
	return NULL;
}

mail_message& mail_message::set_body(const mail_body& body)
{
	body_ = &body;
	return *this;
}

mail_message& mail_message::add_attachment(const char* filepath,
	const char* content_type)
{
	if (filepath == NULL || content_type == NULL)
		return *this;

	char* buf = (char*) dbuf_->dbuf_alloc(sizeof(mail_attach));
	mail_attach* attach = new(buf) mail_attach(filepath,
		content_type, charset_);
	attachments_.push_back(attach);
	return *this;
}

#if defined(_WIN32) || defined(_WIN64)
#include <process.h>
#define PID	_getpid
#else
#include <unistd.h>
#define PID	getpid
#endif // defined(_WIN32) || defined(_WIN64)

bool mail_message::append_addr(const rfc822_addr& addr, string& out)
{
	if (addr.comment)
	{
		out.append("\"");
		if (rfc2047::encode(addr.comment, (int) strlen(addr.comment),
			&out, charset_, 'B', false) == false)
		{
			logger_error("rfc2047::encode(%s) error",
				addr.comment);
			return false;
		}
		out.append("\" ");
	}

	out.format_append("<%s>", addr.addr);
	return true;
}

bool mail_message::append_addr(const char* name, const rfc822_addr& addr,
	string& out)
{
	out.format_append("%s: ", name);

	if (append_addr(addr, out) == false)
		return false;

	out.append("\r\n");
	return true;
}

bool mail_message::append_addrs(const char* name,
	const std::vector<rfc822_addr*>& addrs, string& out)
{
	std::vector<rfc822_addr*>::const_iterator cit = addrs.begin();
	if (cit == addrs.end())
		return true;

	out.format_append("%s: ", name);
	if (append_addr(**cit, out) == false)
		return false;

	if (++cit == addrs.end())
	{
		out.append("\r\n");
		return true;
	}

	out.append(",\r\n");

	while (true)
	{
		out.append("\t");
		if (append_addr(**cit, out) == false)
			return false;

		if (++cit == addrs.end())
		{
			out.append("\r\n");
			break;
		}
		out.append(",\r\n");
	}

	return true;
}

bool mail_message::append_subject(const char* subject, string& out)
{
	out.append("Subject: ");
	if (rfc2047::encode(subject, (int) strlen(subject), &out,
		charset_, 'B', false) == false)
	{
		logger_error("rfc2047::encode error!");
		return false;
	}
	out.append("\r\n");
	return true;
}

bool mail_message::append_date(string& out)
{
	rfc822 rfc;
	char  buf[128];
	rfc.mkdate(time(NULL), buf, sizeof(buf), tzone_cst);
	out.format_append("Date: %s\r\n", buf);
	return true;
}

bool mail_message::append_message_id(string& out)
{
	out.format_append("Message-ID: <%lu.%lu.%lu.acl@localhost>\r\n",
		(unsigned long) PID(), thread::thread_self(),
		(unsigned long) time(NULL));
	return true;
}

bool mail_message::build_header(string& out)
{
	std::vector<std::pair<char*, char*> >::const_iterator cit;
	for (cit = headers_.begin(); cit != headers_.end(); ++cit)
		out.format_append("%s: %s\r\n", (*cit).first, (*cit).second);

	//if (reply_to_ && !append_addr(fp, "ReplyTo", *reply_to_))
	//	return false;

	if (return_path_ && !append_addr("Return-Path", *return_path_, out))
		return false;

	if (delivered_to_ && !append_addr("Delivered-To", *delivered_to_, out))
		return false;

	if (from_ && !append_addr("From", *from_, out))
		return false;

	if (!append_addrs("To", to_list_, out))
		return false;

	if (!append_addrs("Cc", cc_list_, out))
		return false;

	if (subject_ && !append_subject(subject_, out))
		return false;

	if (!append_date(out))
		return false;

	out.append("MIME-Version: 1.0\r\n");

	if (append_message_id(out) == false)
		return false;

	return true;
}

bool mail_message::append_header(ofstream& fp)
{
	string buf;

	if (build_header(buf) == false)
		return false;

	if (fp.write(buf) == -1)
	{
		logger_error("write mail header to %s error %s",
			fp.file_path(), last_serror());
		return false;
	}
	return true;
}

void mail_message::create_boundary(const char* id, string& out)
{
	out.format("====_%s_acl_part_%lu_%lu_%lu_====",
		id, (unsigned long) PID(), thread::thread_self(),
		(unsigned long) time(NULL));
}

bool mail_message::append_multipart(ofstream& fp)
{
	string boundary;

	// 创建 MIME 数据唯一分隔符
	create_boundary("0001", boundary);
	
	string buf(8192);

	// 向邮件头中添加 MIME 相关的信息头
	buf.format("Content-Type: multipart/mixed;\r\n"
		"\tcharset=\"%s\";\r\n"
		"\tboundary=\"%s\"\r\n\r\n",
		charset_, boundary.c_str());

	const char *prompt = "This is a multi-part message in MIME format.";
	buf.format_append("%s\r\n\r\n", prompt);

	// 添加数据体
	if (body_ != NULL)
	{
		buf.format_append("--%s\r\n", boundary.c_str());
		if (body_->save_to(buf) == false)
			return false;

		buf.append("\r\n");
	}

	if (fp.write(buf) == -1)
	{
		logger_error("write to %s error %s",
			fp.file_path(), last_serror());
		return false;
	}

	// 将所有附件内容进行 BASE64 编码后存入目标文件中

	mime_base64 base64(true, false);

	std::vector<mail_attach*>::const_iterator cit;
	for (cit = attachments_.begin(); cit != attachments_.end(); ++cit)
	{
		if (fp.format("--%s\r\n", boundary.c_str()) == -1)
		{
			logger_error("write boundary to %s error %s",
				fp.file_path(), last_serror());
			return false;
		}

		if ((*cit)->save_to(&base64, fp) == false)
		{
			logger_error("write attachment header to %s error %s",
				fp.file_path(), last_serror());
			return false;
		}
	}

	// 添加最后的分隔符至邮件尾部

	if (fp.format("--%s--\r\n", boundary.c_str()) == -1)
	{
		logger_error("write boundary end to %s error %s",
			fp.file_path(), last_serror());
		return false;
	}

	return true;
}

bool mail_message::save_to(const char* filepath)
{
	ofstream fp;
	if (fp.open_write(filepath) == false)
	{
		logger_error("open %s error: %s", filepath, last_serror());
		return false;
	}

	filepath_ = dbuf_->dbuf_strdup(filepath);

	// 先添加邮件头部分数据至文件流中

	if (append_header(fp) == false)
		return false;

	// 如果是 multipart 格式，则将 multipart 数据输出至文件流中

	if (!attachments_.empty())
		return append_multipart(fp);

	if (body_ == NULL)
	{
		logger_error("body null!");
		return false;
	}

	// 对非 multipart 格式，直接将正文数据输出至文件流中

	string buf(8192);
	if (body_->save_to(buf) == false)
		return false;

	if (fp.write(buf) == -1)
	{
		logger_error("write to %s error %s",
			fp.file_path(), last_serror());
		return false;
	}

	return true;
}

} // namespace acl
