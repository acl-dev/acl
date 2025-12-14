#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include "../stdlib/string.hpp"
#include "../stdlib/noncopyable.hpp"
#include "../http/http_ctype.hpp"

#if !defined(ACL_MIME_DISABLE)

namespace acl {

class mime_code;
class mail_attach;

/**
 * Email body construction class
 */
class ACL_CPP_API mail_body : public noncopyable
{
public:
	/**
	 * Constructor
	 * @param charset {const char*} Character set of body
	 * @param encoding {const char*} Encoding format of body
	 */
	mail_body(const char* charset = "utf-8",
		const char* encoding = "base64");
	~mail_body();

	/**
	 * Get content type of body
	 * @return {const string&}
	 */
	const string& get_content_type() const
	{
		return content_type_;
	}

	/**
	 * Get content type object of body
	 * @return {const http_ctype&}
	 */
	const http_ctype& get_ctype() const
	{
		return ctype_;
	}

	/**
	 * Set email body to TEXT/HTML format
	 * @param html {const char*} HTML data
	 * @param len {size_t} html data length (although html is string format, providing
	 *  data length facilitates more flexible and efficient calls, internally no longer recalculates length through strlen)
	 * @return {mail_body&}
	 */
	mail_body& set_html(const char* html, size_t len);

	/**
	 * Set email body to TEXT/PLAIN format
	 * @param plain {const char*} TEXT data
	 * @param len {size_t} plain data length (although plain is text format, providing
	 *  data length facilitates more flexible and efficient calls, internally no longer recalculates length through strlen)
	 * @return {mail_body&}
	 */
	mail_body& set_plain(const char* plain, size_t len);

	/**
	 * When email content is multipart/alternative format, call this function to set corresponding type's
	 * body content
	 * @param html {const char*} HTML data in body (non-empty)
	 * @param hlen {size_t} html data length (>0)
	 * @param plain {const char*} TEXT data in body (non-empty)
	 * @param plen {size_t} plain data length (>0)
	 * @return {mail_body&}
	 */
	mail_body& set_alternative(const char* html, size_t hlen,
		const char* plain, size_t plen);

	/**
	 * When email body content is multipart/relative format, call this function to set body content
	 * @param html {const char*} HTML data in body (non-empty)
	 * @param hlen {size_t} html data length (>0)
	 * @param plain {const char*} plain data in body (non-empty)
	 * @param plen {size_t} plain data length (>0)
	 * @param attachments {const std::vector<mail_attach*>&} Store
	 *  image and other attachment objects related to cid in html
	 * @return {mail_body&}
	 */
	mail_body& set_relative(const char* html, size_t hlen,
		const char* plain, size_t plen,
		const std::vector<mail_attach*>& attachments);

	/**
	 * Get html/plain data set by set_html function
	 * @param len {size_t} Store data length result
	 * @return {const char*}
	 */
	const char* get_html(size_t& len) const
	{
		len = hlen_;
		return html_;
	}

	/**
	 * Get plain/plain data set by set_plain function
	 * @param len {size_t} Store data length result
	 * @return {const char*}
	 */
	const char* get_plain(size_t& len) const
	{
		len = plen_;
		return plain_;
	}

	/**
	 * Get attachment collection set by set_attachments function
	 * @return {const std::vector<mail_attach*>*}
	 */
	const std::vector<mail_attach*>* get_attachments() const
	{
		return attachments_;
	}

	/**
	 * Construct email body and append result to given output stream
	 * @param out {ostream&} Output stream object
	 * @return {bool} Whether operation was successful
	 */
	bool save_to(ostream& out) const;

	/**
	 * Construct email body and append result to given buffer
	 * @param out {string&} Store result
	 * @return {bool} Whether operation was successful
	 */
	bool save_to(string& out) const;

	/**
	 * Email body construction process for text/html format, and append result to given buffer
	 * @param in {const char*} Input html format data
	 * @param len {size_t} Data length of in
	 * @param out {string&} Store result in append mode
	 * @return {bool} Whether operation was successful
	 */
	bool save_html(const char* in, size_t len, string& out) const;

	/**
	 * Email body construction process for text/plain format, and append result to given buffer
	 * @param in {const char*} Input plain format data
	 * @param len {size_t} Data length of in
	 * @param out {string&} Store result in append mode
	 * @return {bool} Whether operation was successful
	 */
	bool save_plain(const char* in, size_t len, string& out) const;

	/**
	 * Email body construction process for multipart/relative format, and append result to given buffer
	 * @param html {const char*} Input html format data
	 * @param hlen {size_t} Data length of html
	 * @param plain {const char*} TEXT data in body (non-empty)
	 * @param plen {size_t} plain data length (>0)
	 * @param attachments {const std::vector<mail_attach*>&} Store
	 *  image and other attachment objects related to cid in html
	 * @param out {string&} Store result in append mode
	 * @return {bool} Whether operation was successful
	 */
	bool save_relative(const char* html, size_t hlen,
		const char* plain, size_t plen,
		const std::vector<mail_attach*>& attachments,
		string& out) const;

	/**
	 * Email body construction process for multipart/alternative format, and append result to given buffer
	 * @param html {const char*} Input html format data
	 * @param hlen {size_t} Data length of html
	 * @param plain {const char*} TEXT data in body (non-empty)
	 * @param plen {size_t} plain data length (>0)
	 * @param out {string&} Store result in append mode
	 * @return {bool} Whether operation was successful
	 */
	bool save_alternative(const char* html, size_t hlen,
		const char* plain, size_t plen, string& out) const;

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
	const char* plain_;
	size_t plen_;
	const std::vector<mail_attach*>* attachments_;

	bool build(const char* in, size_t len, const char* content_type,
		const char* charset, mime_code& coder, string& out) const;
	bool build_html(const char* in, size_t len,
		const char* charset, string& out) const;
	bool build_plain(const char* in, size_t len,
		const char* charset, string& out) const;

	void set_content_type(const char* content_type);
};

} // namespace acl

#endif // !defined(ACL_MIME_DISABLE)

