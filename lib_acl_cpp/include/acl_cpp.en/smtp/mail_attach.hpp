#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"

#if !defined(ACL_MIME_DISABLE)

namespace acl {

class mime_code;
class ostream;

/**
 * When composing emails, this class is used to create functionality related to email attachments
 */
class ACL_CPP_API mail_attach
{
public:
	/**
	 * Constructor when packaging a regular file into email
	 * @param filepath {const char*} Attachment file storage path (including filename)
	 * @param content_type {const char*} Attachment file type
	 * @param charset {const char*} If it is a plain file, this parameter indicates the character set of plain text
	 */
	mail_attach(const char* filepath, const char* content_type,
		const char* charset);
	~mail_attach();

	/**
	 * Set attachment filename. Internally will automatically encode filename in rfc2047 format
	 * @param name {const char*} Non-empty string
	 * @param charset {const char*} This parameter specifies character set. When not NULL, then internally
	 *  automatically uses rfc2047 format to encode filename, otherwise internally directly stores input name
	 * @return {mail_attach&}
	 */
	mail_attach& set_filename(const char* name, const char* charset = NULL);

	/**
	 * When email data body is multipart/relative type, call this function to set cid identifier
	 * in html body
	 * @param id {const char*} cid identifier
	 * @return {mail_attach&}
	 */
	mail_attach& set_content_id(const char* id);

	/**
	 * Get attachment file path passed in constructor
	 * @return {const char*}
	 */
	const char* get_filepath() const
	{
		return filepath_.c_str();
	}

	/**
	 * Get filename part of attachment after rfc2047 encoding
	 * @return {const char*}
	 */
	const char* get_filename() const
	{
		return filename_.c_str();
	}

	/**
	 * Get file type passed in constructor
	 * @return {const char*}
	 */
	const char* get_content_type() const
	{
		return ctype_.c_str();
	}

	/**
	 * Get cid identifier of this attachment set by set_content_id
	 * @return {const char*}
	 */
	const char* get_content_id() const
	{
		return cid_.c_str();
	}

	/**
	 * Encode attachment content using the passed encoder and store in memory buffer
	 * @param coder {mime_code*} Encoder (base64/qp, etc.)
	 * @param out {string&} Store result, using append method
	 * @return {bool} Whether encoding process was successful
	 */
	bool save_to(mime_code* coder, string& out);

	/**
	 * Encode attachment content using the passed encoder and store in output stream
	 * @param coder {mime_code*} Encoder (base64/qp, etc.)
	 * @param out {out&} Store result
	 * @return {bool} Whether encoding process was successful
	 */
	bool save_to(mime_code* coder, ostream& out);

	/**
	 * Create file header information for this attachment in MIME email
	 * @param transfer_encoding {const char*} Encoding method
	 * @param out {string&} Store result, using append method
	 */
	void build_header(const char* transfer_encoding, string& out);

private:
	string filepath_;
	string filename_;
	string ctype_;
	string cid_;
	string charset_;

	bool rfc2047_encode(const char* name, const char* charset, string& out);
};

} // namespace acl

#endif // !defined(ACL_MIME_DISABLE)

