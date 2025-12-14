#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"
#include "../stdlib/noncopyable.hpp"
#include <vector>

#if !defined(ACL_MIME_DISABLE)

namespace acl {

class dbuf_pool;
struct rfc822_addr;
class mail_attach;
class mail_body;
class ofstream;

/**
 * Email data construction class. This class can generate a complete email, and is also used to build email envelope information
 * for SMTP sending process
 */
class ACL_CPP_API mail_message : public noncopyable
{
public:
	/**
	 * Constructor
	 * @param charset {const char*} Character set
	 */
	mail_message(const char* charset = "utf-8");
	~mail_message();

	/**
	 * Set authentication information for SMTP sending process
	 * @param user {const char*} Email account
	 * @param pass {const char*} Email password
	 * @return {mail_message&}
	 */
	mail_message& set_auth(const char* user, const char* pass);

	/**
	 * Set sender's email address for email. This field can be used for MAIL FROM command in SMTP sending process,
	 * and can also be used as From field value in email header
	 * @param from {const char*} Sender's email address
	 * @param name {const char*} Sender's name
	 * @return {mail_message&}
	 */
	mail_message& set_from(const char* from, const char* name = NULL);

	mail_message& set_sender(const char* sender, const char* name = NULL);

	/**
	 * Set Reply-To field value in email header
	 * @param reply_to {const char*} Reply-To email field value
	 * @param name {const char*} Person name corresponding to Reply-To
	 * @return {mail_message&}
	 */
	mail_message& set_reply_to(const char* reply_to, const char* name = NULL);

	/**
	 * Set Return-Path field value in email header
	 * @param return_path {const char*} Return-Path email field value
	 * @return {mail_message&}
	 */
	mail_message& set_return_path(const char* return_path);

	/**
	 * Set Delivered-To field value in email header
	 * @param delivered_to {const char*} Delivered-To email field value
	 * @return {mail_message&}
	 */
	mail_message& set_delivered_to(const char* delivered_to);

	/**
	 * Add recipient address. This address only appears in envelope, not in email header
	 * @param recipients {const char*} Recipient collection, complies with RFC822 format
	 * @return {mail_message&}
	 */
	mail_message& add_recipients(const char* recipients);

	/**
	 * Set To field value in email header. This recipient address collection is also used in envelope as recipients
	 * @param to {const char*} Recipient email address collection, complies with RFC822 format
	 * @return {mail_message&}
	 */
	mail_message& add_to(const char* to);

	/**
	 * Set Cc field value in email header. This recipient address collection is also used in envelope as recipients
	 * @param cc {const char*} Recipient email address collection, complies with RFC822 format
	 * @return {mail_message&}
	 */
	mail_message& add_cc(const char* cc);

	/**
	 * Set blind carbon copy address collection for email sending. This address collection will not appear in email header
	 * @param bcc {const char*} Blind carbon copy email address collection, complies with RFC822 format
	 * @return {mail_message&}
	 */
	mail_message& add_bcc(const char* bcc);

	/**
	 * Set subject in email header. This subject will use rfc2047 encoding and use character set set by class constructor
	 * @param subject {const char*} Email header subject field value
	 * @return {mail_message&}
	 */
	mail_message& set_subject(const char* subject);

	/**
	 * Users can call this function to add header extension field values in email header
	 * @param name {const char*} Field name
	 * @param value {const char*} Field value
	 * @return {mail_message&}
	 */
	mail_message& add_header(const char* name, const char* value);

	/**
	 * Set email body object
	 * @param body {const mail_body&} Email body object
	 * @return {mail_message&}
	 */
	mail_message& set_body(const mail_body& body);

	/**
	 * Add an attachment to an email
	 * @param filepath {const char*} Full path of attachment (non-empty)
	 * @param content_type {const char*} Attachment type (non-empty)
	 * @return {mail_message&}
	 */
	mail_message& add_attachment(const char* filepath,
		const char* content_type);

	/**
	 * Construct a complete email and store email content in given disk file. If this file
	 * exists, will clear it first, otherwise will create new file
	 * @param filepath {const char*} Target file
	 * @return {bool} Whether operation was successful
	 */
	bool save_to(const char* filepath);

	/**
	 * Can call this function separately to generate email header data
	 * @param out {string&} Created email header data will be appended to this buffer
	 * @return {bool} Whether operation was successful
	 */
	bool build_header(string& out);

	/**
	 * Get full path of created email on disk. This function must be called after successfully calling save_to
	 * @return {const char*}
	 */
	const char* get_email() const
	{
		return filepath_;
	}

	/**
	 * Get email account for SMTP authentication
	 * @return {const char*}
	 */
	const char* get_auth_user() const
	{
		return auth_user_;
	}

	/**
	 * Get email account password for SMTP authentication
	 * @return {const char*}
	 */
	const char* get_auth_pass() const
	{
		return auth_pass_;
	}

	/**
	 * Get email address object set by set_from
	 * @return {const rfc822_addr*}
	 */
	const rfc822_addr* get_from() const
	{
		return from_;
	}

	/**
	 * Get email address object set by set_sender
	 * @return {const rfc822_addr*}
	 */
	const rfc822_addr* get_sender() const
	{
		return sender_;
	}

	/**
	 * Get email address object set by set_reply_to
	 * @return {const rfc822_addr*}
	 */
	const rfc822_addr* get_reply_to() const
	{
		return reply_to_;
	}

	/**
	 * Get email address object set by set_return_path
	 * @return {const rfc822_addr*}
	 */
	const rfc822_addr* get_return_path() const
	{
		return return_path_;
	}

	/**
	 * Get email address object set by set_delivered_to
	 * @return {const rfc822_addr*}
	 */
	const rfc822_addr* get_delivered_to() const
	{
		return delivered_to_;
	}

	const std::vector<rfc822_addr*>& get_to() const
	{
		return to_list_;
	}

	/**
	 * Get email address object collection set by set_cc
	 * @return {const std::vector<rfc822_addr*>&}
	 */
	const std::vector<rfc822_addr*>& get_cc() const
	{
		return cc_list_;
	}

	/**
	 * Get email address object collection set by set_bcc
	 * @return {const std::vector<rfc822_addr*>&}
	 */
	const std::vector<rfc822_addr*>& get_bcc() const
	{
		return bcc_list_;
	}

	/**
	 * Get all email recipient address collection
	 * @return {const std::vector<rfc822_addr*>&}
	 */
	const std::vector<rfc822_addr*>& get_recipients() const
	{
		return recipients_;
	}

	/**
	 * Get email header extension field value set by user
	 * @param name {const char*} Field name
	 * @return {const char*}
	 */
	const char* get_header_value(const char* name) const;

	/**
	 * Create unique delimiter for MIME data
	 * @param id {const char*} ID identifier filled by caller
	 * @param out {string&} Store result
	 */
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

#endif // !defined(ACL_MIME_DISABLE)

