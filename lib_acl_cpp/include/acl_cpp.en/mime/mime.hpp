#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"
#include <list>
#include <string>
#include "mime_head.hpp"

#if !defined(ACL_MIME_DISABLE)

struct MIME_STATE;

namespace acl {

class string;
class mime_node;
class mime_attach;
class mime_body;
class mime_image;
class ifstream;
class fstream;

class ACL_CPP_API mime : public noncopyable {
public:
	mime();
	~mime();

	/////////////////////////////////////////////////////////////////////
	// Functions related to mail parsing and saving

	/**
	 * When reusing the same MIME object to parse multiple emails, you need to call this function to clear previously
	 * parsed data in memory. Otherwise, calling this function multiple times will have no effect. To avoid affecting
	 * performance, it is recommended to call this function before parsing each email.
	 */
	mime& reset();

	/**
	 * When users can determine mail header completion, they can call this function to mark primary mail header completion.
	 */
	void primary_head_finish();

	/**
	 * When streaming parsing, call this function to determine whether mail header is complete.
	 * @return {bool} Whether mail header is complete.
	 */
	bool primary_head_ok() const;

	/**
	 * Initialize streaming parsing process. This function internally automatically calls reset() to reset object
	 * state.
	 * @param path {const char*} Mail file path. When this parameter is empty, it means
	 *  mail parsing is not from file, and you cannot call save_xxx related interfaces.
	 */
	void update_begin(const char* path);

	/**
	 * Call this function to parse mail data in streaming format. This function parses mail header first, then uses this
	 * interface to parse mail body until update_end() interface ends. If you need to parse multiple emails,
	 * you need to call this function continuously until it returns true, indicating multipart format
	 * mail parsing is complete. For non-multipart format mail, this function may return true,
	 * so you need to determine mail completion position yourself.
	 * @param data {const char*} Mail data (can include mail header or mail body, but
	 *  data must be continuous).
	 * @param len {size_t} Data length of data.
	 * @return {bool} For multipart mail, returning true indicates this mail parsing is complete;
	 *  For non-multipart mail, this return value is always false, with no special meaning. You need to
	 *  determine mail completion position yourself.
	 * Note: After calling this function, you must call update_end to notify parsing completion.
	 */
	bool update(const char* data, size_t len);

	/**
	 * Call this function when streaming parsing is complete.
	 */
	void update_end();

	/**
	 * Call this function to parse an email from file.
	 * @param file_path {const char*} Mail file path.
	 * @return {bool} Returns false to indicate source mail file cannot be opened.
	 */
	bool parse(const char* file_path);

	/**
	 * Save mail content to an output stream.
	 * @param out {ostream&} Target output stream.
	 * @return {bool} Whether successful.
	 */
	bool save_as(ostream& out);

	/**
	 * Save mail content to a file.
	 * @param file_path {const char*} Target file path.
	 * @return {bool} Whether successful.
	 */
	bool save_as(const char* file_path);

	/**
	 * After mail parsing is complete, save in client display format to HTML file process,
	 * users can use browser to open this HTML page.
	 * @param path {const char*} Page save path.
	 * @param filename {const char*} Target file name.
	 * @param enableDecode {bool} Whether to automatically decode during conversion.
	 * @param toCharset {const char*} Target character set.
	 * @param off {off_t} When parsing attachments, offset from start to attachment data.
	 * @return {bool} Whether successful.
	 */
	bool save_mail(const char* path, const char* filename,
		bool enableDecode = true, const char* toCharset = "gb2312",
		off_t off = 0);

	/**
	 * Get mail body node.
	 * @param htmlFirst {bool} Prefer HTML format text body. If not found, then
	 *  plain text body. If only HTML text exists, convert HTML text to plain text.
	 * @param enableDecode {bool} Whether to decode original encoding during conversion.
	 * @param toCharset {const char*} Target character set.
	 * @param off {off_t} When parsing attachments, offset from start to attachment data.
	 * @return {mime_body*} Returns NULL if not found or error.
	 */
	mime_body* get_body_node(bool htmlFirst, bool enableDecode = true,
                const char* toCharset = "gb2312", off_t off = 0);

	/**
	 * Get text/plain format body node.
	 * @param enableDecode {bool} Whether to decode original encoding during conversion.
	 * @param toCharset {const char*} Target character set.
	 * @param off {off_t} When parsing attachments, offset from start to attachment data.
	 * @return {mime_body*} Returns NULL if plain format body is not found or error.
	 */
	mime_body* get_plain_body(bool enableDecode = true,
		const char* toCharset = "gb2312", off_t off = 0);

	/**
	 * Get text/html format body node.
	 * @param enableDecode {bool} Whether to decode original encoding during conversion.
	 * @param toCharset {const char*} Target character set.
	 * @param off {off_t} When parsing attachments, offset from start to attachment data.
	 * @return {mime_body*} Returns NULL if html format body is not found or error.
	 */
	mime_body* get_html_body(bool enableDecode = true,
		const char* toCharset = "gb2312", off_t off = 0);

	/**
	 * Get all mime node list.
	 * @param enableDecode {bool} Whether to automatically decode during conversion.
	 * @param toCharset {const char*} Target character set.
	 * @param off {off_t} When parsing attachments, offset from start to attachment data.
	 * @return {const std::list<mime_node*>&}
	 */
	const std::list<mime_node*>& get_mime_nodes(bool enableDecode = true,
		const char* toCharset = "gb2312", off_t off = 0);

	/**
	 * Get attachment list.
	 * @param enableDecode {bool} Whether to automatically decode during conversion.
	 * @param toCharset {const char*} Target character set.
	 * @param off {off_t} When parsing attachments, offset from start to attachment data.
	 * @param all {bool} Get all nodes including message/application/image type nodes.
	 * @return {const std::list<mime_attach*>&}
	 */
	const std::list<mime_attach*>& get_attachments(bool enableDecode = true,
		const char* toCharset = "gb2312", off_t off = 0, bool all = true);

	/**
	 * Get image list.
	 * @param enableDecode {bool} Whether to automatically decode during conversion.
	 * @param toCharset {const char*} Target character set.
	 * @param off {off_t} When parsing attachments, offset from start to attachment data.
	 * @return {const std::list<mime_image*>&}
	 */
	const std::list<mime_image*>& get_images(bool enableDecode = true,
		const char* toCharset = "gb2312", off_t off = 0);
	mime_image* get_image(const char* cld, bool enableDecode = true,
		const char* toCharset = "gb2312", off_t off = 0);

	/**
	 * Debug MIME object.
	 * @param save_path {const char*} Path to store MIME debug information.
	 * @param decode {bool} Whether to decode original encoding.
	 */
	void mime_debug(const char* save_path, bool decode = true);

	/////////////////////////////////////////////////////////////////////
        // Functions related to mail header

	/**
	 * Set sender address.
	 * @param addr {const char*} Mail address.
	 * @return {mime&}
	 */
	mime& set_sender(const char* addr) {
		m_primaryHeader.set_returnpath(addr);
		return *this;
	}

	/**
	 * Set sender address: From: zhengshuxin@51iker.com
	 * @param addr {const char*} Mail address.
	 * @return {mime&}
	 */
	mime& set_from(const char* addr) {
		m_primaryHeader.set_from(addr);
		return *this;
	}

	/**
	 * Set mail reply address: Reply-To: zhengshuxin@51iker.com
	 * @param addr {const char*} Mail address.
	 * @return {mime&}
	 */
	mime& set_replyto(const char* addr) {
		m_primaryHeader.set_replyto(addr);
		return *this;
	}

	/**
	 * Set mail return address Return-Path: <zhengshuxin@51iker.com>
	 * @param addr {const char*} Mail address.
	 * @return {mime&}
	 */
	mime& set_returnpath(const char* addr) {
		m_primaryHeader.set_returnpath(addr);
		return *this;
	}

	/**
	 * Set mail subject: Subject: test
	 * @param s {const char*} Mail subject.
	 * @return {mime&}
	 */
	mime& set_subject(const char* s) {
		m_primaryHeader.set_subject(s);
		return *this;
	}

	/**
	 * Add mail recipient address: To: <zhengshuxin@51iker.com>
	 * @param addr {const char*} Mail address.
	 * @return {mime&}
	 */
	mime& add_to(const char* addr) {
		m_primaryHeader.add_to(addr);
		return *this;
	}

	/**
	 * Add mail CC address: CC: <zhengshuxin@51iker.com>
	 * @param addr {const char* addr} Mail address.
	 * @return {mime&}
	 */
	mime& add_cc(const char* addr) {
		m_primaryHeader.add_cc(addr);
		return *this;
	}

	/**
	 * Add mail BCC address: BCC: <zhengshuxin@51iker.com>
	 * @param addr {const char* addr} Mail address.
	 * @return {mime&}
	 */
	mime& add_bcc(const char* addr) {
		m_primaryHeader.add_bcc(addr);
		return *this;
	}

	/**
	 * Add mail recipient address: CC: <zhengshuxin@51iker.com>
	 * @param addr {const char* addr} Mail address.
	 * @return {mime&}
	 */
	mime& add_rcpt(const char* addr) {
		m_primaryHeader.add_rcpt(addr);
		return *this;
	}

	/**
	 * Add mail header field.
	 * @param name {const char*} Field name.
	 * @param value {const char*} Field value.
	 * @return {mime&}
	 */
	mime& add_header(const char* name, const char* value) {
		m_primaryHeader.add_header(name, value);
		return *this;
	}

	/**
	 * Set mail header content type: Content-Type: text/plain
	 * @param ctype {const char*} Content type.
	 * @param stype {const char*} Subtype.
	 * @return {mime&}
	 */
	mime& set_type(const char* ctype, const char* stype) {
		m_primaryHeader.set_type(ctype, stype);
		return *this;
	}

	/**
	 * Set mail header boundary.
	 * @param s {const char*} Boundary.
	 * @return {mime&}
	 */
	mime& set_boundary(const char* s) {
		m_primaryHeader.set_boundary(s);
		return *this;
	}

	/**
	 * Get sender address.
	 * @return {const string&} Return value. When return value is empty
	 *  (i.e., string::empty()), it means no such field.
	 */
	const string& sender() const {
		return m_primaryHeader.sender();
	}

	/**
	 * Get sender address.
	 * @return {const string&} Return value. When return value is empty
	 *  (i.e., string::empty()), it means no such field.
	 */
	const string& from() const {
		return m_primaryHeader.from();
	}

	/**
	 * Get reply mail address.
	 * @return {const string&} Return value. When return value is empty
	 *  (i.e., string::empty()), it means no such field.
	 */
	const string& replyto() const {
		return m_primaryHeader.replyto();
	}

	/**
	 * Get reply mail address.
	 * @return {const string&} Return value. When return value is empty
	 *  (i.e., string::empty()), it means no such field.
	 */
	const string& returnpath() const {
		return m_primaryHeader.returnpath();
	}

	/**
	 * Get mail subject.
	 * @return {const string&} Return value. When return value is empty
	 *  (i.e., string::empty()), it means no such field.
	 */
	const string& subject() const {
		return m_primaryHeader.subject();
	}

	/**
	 * Get recipient list: To: xxx@xxx.com
	 * @return {const std::list<char*>&) Return value. When return value is empty
	 *  (i.e., std::list<char*>::empty()), it means no such field.
	 */
	const std::list<char*>& to_list() const {
		return m_primaryHeader.to_list();
	}

	/**
	 * Get CC recipient list: To: xxx@xxx.com
	 * @return {const std::list<char*>&) Return value. When return value is empty
	 *  (i.e., std::list<char*>::empty()), it means no such field.
	 */
	const std::list<char*>& cc_list() const {
		return m_primaryHeader.cc_list();
	}

	/**
	 * Get BCC recipient list: To: xxx@xxx.com
	 * @return {const std::list<char*>&) Return value. When return value is empty
	 *  (i.e., std::list<char*>::empty()), it means no such field.
	 */
	const std::list<char*>& bcc_list() const {
		return m_primaryHeader.bcc_list();
	}

	/**
	 * Get recipient list:
	 * To: xxx@xxx.xxx, CC: xxx@xxx.xxx, BCC: xxx@xxx.xxx
	 * @return {const std::list<char*>&) Return value. When return value is empty
	 *  (i.e., std::list<char*>::empty()), it means no such field.
	 */
	const std::list<char*>& rcpt_list() const {
		return m_primaryHeader.rcpt_list();
	}

	/**
	 * Get mail header additional field list.
	 * @return {const std::list<HEADER*>&)
	 */
	const std::list<HEADER*>& header_list() const {
		return m_primaryHeader.header_list();
	}	

	/**
	 * Query mail header corresponding field and get field value.
	 * @param name {const char*} Field name.
	 * @return {const char*} Field value. Returns empty when not found.
	 */
	const char* header_value(const char* name) const {
		return m_primaryHeader.header_value(name);
	}

	/**
	 * Query mail header corresponding field and get field value list.
	 * @param name {const char*} Field name.
	 * @param values {std::list<const char*>*} Store corresponding results.
	 * @return {int} Number of field value sets.
	 */
	int header_values(const char* name, std::list<const char*>* values) const {
		return m_primaryHeader.header_values(name, values);
	}

	/**
	 * Get text field in mail header Content-Type: text/html.
	 * @return {const char*} Always returns non-empty value.
	 */
	const char* get_ctype() const {
		return m_primaryHeader.get_ctype();
	}

	/**
	 * Get html field in mail header Content-Type: text/html.
	 * @return {const char*} Always returns non-empty value.
	 */
	const char* get_stype() const {
		return m_primaryHeader.get_stype();
	}

	/**
	 * Get mail header.
	 * @return {const mime_head&}
	 */
	const mime_head& primary_header() const {
		return m_primaryHeader;
	}

private:
	mime_head m_primaryHeader;

	MIME_STATE* m_pMimeState;
	bool m_bPrimaryHeadFinish;
	char* m_pFilePath;
	mime_body* m_pBody;
	std::list<mime_node*>* m_pNodes;
	std::list<mime_attach*>* m_pAttaches;
	std::list<mime_image*>* m_pImages;
};

} // namespace acl

#endif // !defined(ACL_MIME_DISABLE)

