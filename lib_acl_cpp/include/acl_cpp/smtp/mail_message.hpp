#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include <vector>

namespace acl {

class dbuf_pool;
struct rfc822_addr;
class mail_attach;
class mail_body;
class ofstream;

class ACL_CPP_API mail_message
{
public:
	mail_message(const char* charset = "utf-8");
	~mail_message();

	mail_message& set_auth(const char* user, const char* pass);
	mail_message& set_from(const char* from, const char* name = NULL);
	mail_message& set_sender(const char* sender, const char* name = NULL);
	mail_message& set_reply_to(const char* reply_to, const char* name = NULL);
	mail_message& set_return_path(const char* return_path);
	mail_message& set_delivered_to(const char* delivered_to);
	mail_message& add_recipients(const char* recipients);
	mail_message& add_to(const char* to);
	mail_message& add_cc(const char* cc);
	mail_message& add_bcc(const char* bcc);
	mail_message& set_subject(const char* subject);
	mail_message& add_header(const char* name, const char* value);
	mail_message& set_body(const mail_body* body);
	mail_message& add_attachment(const char* filepath,
		const char* content_type);

	bool save_to(const char* filepath);

	bool build_header(string& out);

	const char* get_email() const
	{
		return filepath_;
	}

	const char* get_auth_user() const
	{
		return auth_user_;
	}

	const char* get_auth_pass() const
	{
		return auth_pass_;
	}

	const rfc822_addr* get_from() const
	{
		return from_;
	}

	const rfc822_addr* get_sender() const
	{
		return sender_;
	}

	const rfc822_addr* get_reply_to() const
	{
		return reply_to_;
	}

	const rfc822_addr* get_return_path() const
	{
		return return_path_;
	}

	const rfc822_addr* get_delivered_to() const
	{
		return delivered_to_;
	}

	const std::vector<rfc822_addr*>& get_to() const
	{
		return to_list_;
	}

	const std::vector<rfc822_addr*>& get_cc() const
	{
		return cc_list_;
	}

	const std::vector<rfc822_addr*>& get_bcc() const
	{
		return bcc_list_;
	}

	const std::vector<rfc822_addr*>& get_recipients() const
	{
		return recipients_;
	}

	const char* get_header_value(const char* name) const;

	const char* get_filepath() const
	{
		return filepath_;
	}

	static void create_boundary(const char* id, string& out);

private:
	dbuf_pool* dbuf_;
	char* charset_;
	char* transfer_encoding_;

	char* auth_user_;
	char* auth_pass_;
	rfc822_addr* from_;
	rfc822_addr* sender_;
	rfc822_addr* reply_to_;
	rfc822_addr* return_path_;
	rfc822_addr* delivered_to_;
	std::vector<rfc822_addr*> to_list_;
	std::vector<rfc822_addr*> cc_list_;
	std::vector<rfc822_addr*> bcc_list_;
	std::vector<rfc822_addr*> recipients_;
	char* subject_;
	std::vector<std::pair<char*, char*> > headers_;
	std::vector<mail_attach*> attachments_;
	const mail_body* body_;
	size_t body_len_;
	char* filepath_;

	void add_addrs(const char* in, std::vector<rfc822_addr*>& out);
	bool append_addr(const rfc822_addr& addr, string& out);
	bool append_addr(const char* name, const rfc822_addr& addr,
		string& out);
	bool append_addrs(const char* name,
		const std::vector<rfc822_addr*>& addrs, string& out);
	bool append_message_id(string& out);
	bool append_subject(const char* subject, string& out);
	bool append_date(string& out);
	bool append_header(ofstream& fp);
	bool append_multipart(ofstream& fp);
};

} // namespace acl
