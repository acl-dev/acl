#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <vector>
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/http/http_ctype.hpp"

namespace acl {

class mime_code;
class mail_attach;

class ACL_CPP_API mail_body
{
public:
	mail_body(const char* charset = "utf-8",
		const char* encoding = "base64");
	~mail_body();

	const string& get_content_type() const
	{
		return content_type_;
	}
	const http_ctype& get_ctype() const
	{
		return ctype_;
	}

	mail_body& set_html(const char* html, size_t len);
	mail_body& set_text(const char* text, size_t len);
	mail_body& set_alternative(const char* html, size_t hlen,
		const char* text, size_t tlen);
	mail_body& set_relative(const char* html, size_t hlen,
		const char* text, size_t tlen,
		const std::vector<mail_attach*>& attachments);

	const char* get_html(size_t& len) const
	{
		len = hlen_;
		return html_;
	}
	const char* get_text(size_t& len) const
	{
		len = tlen_;
		return text_;
	}
	const std::vector<mail_attach*>* get_attachments() const
	{
		return attachments_;
	}

	bool save_to(ostream& out) const;
	bool save_to(string& out) const;
	bool save_html(const char* in, size_t len, string& out) const;
	bool save_text(const char* in, size_t len, string& out) const;
	bool save_relative(const char* html, size_t hlen,
		const char* text, size_t tlen,
		const std::vector<mail_attach*>& attachments,
		string& out) const;
	bool save_alternative(const char* html, size_t hlen,
		const char* text, size_t tlen, string& out) const;

private:
	string  charset_;
	string  content_type_;
	string  transfer_encoding_;
	mime_code* coder_;
	string  boundary_;
	http_ctype ctype_;
	int     mime_stype_;

	const char* html_;
	size_t hlen_;
	const char* text_;
	size_t tlen_;
	const std::vector<mail_attach*>* attachments_;

	bool build(const char* in, size_t len, string& out) const;
	void set_content_type(const char* content_type);
};

} // namespace acl
