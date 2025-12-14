#pragma once
#include "../acl_cpp_define.hpp"
#include "mime_node.hpp"

#if !defined(ACL_MIME_DISABLE)

struct MIME_NODE;

namespace acl {

class pipe_manager;
class ostream;
class pipe_string;
class string;

class ACL_CPP_API mime_body : public mime_node {
public:
	/**
	 * Constructor
	 * @param emailFile {const char*} Source file storing email content. Can
	 * be empty, but when empty, cannot specify source file when calling save_body
	 * function
	 * @param node {const MIME_NODE*} A node object in email
	 * @param htmlFirst {bool} Whether to prioritize extracting HTML data when
	 * extracting content
	 * @param enableDecode {bool} When email content is base64/qp and other
	 * encoding formats,
	 *  whether to automatically decode
	 * @param toCharset {const char*} Default target character set. If target
	 * character set is different from source character set, performs character set
	 * conversion
	 * @param off {off_t} Relative offset added to starting position of email
	 * content in entire data,
	 *  so that users can add their own private data before email content
	 */
	mime_body(const char* emailFile, const MIME_NODE* node,
		bool htmlFirst = true, bool enableDecode = true,
		const char* toCharset = "gb2312", off_t off = 0)
		: mime_node(emailFile, node, enableDecode, toCharset, off)
		, m_htmlFirst(htmlFirst)
	{
	}

	~mime_body() {}

	/**
	 * Set whether to only extract HTML data. If true, prioritizes extracting HTML
	 * data.
	 * When HTML data does not exist, extracts plain text data. If false,
	 * prioritizes
	 * extracting plain text data. When only HTML data exists, extracts plain text
	 * data from this HTML data
	 * @param htmlFirst {bool}
	 */
	void set_status(bool htmlFirst) {
		m_htmlFirst = htmlFirst;
	}

	/**
	 * Dump email body content into pipe stream
	 * @param out {pipe_manager&} Pipe stream manager
	 * @param src {const char*} Starting address of email content. If it is NULL
	 * pointer,
	 *  extracts email content from emailFile file provided in constructor
	 * @param len {int} Data length of email content. If 0, extracts email content
	 * from
	 *  emailFile file provided in constructor
	 * @return {bool} Whether successful
	 */
	bool save_body(pipe_manager& out, const char* src = NULL, int len = 0);

	/**
	 * Dump email body content into output stream
	 * @param out {ostream&} Output stream
	 * @param src {const char*} Starting address of email content. If it is NULL
	 * pointer,
	 *  extracts email content from emailFile file provided in constructor
	 * @param len {int} Data length of email content. If 0, extracts email content
	 * from
	 *  emailFile file provided in constructor
	 * @return {bool} Whether successful
	 */
	bool save_body(ostream& out, const char* src = NULL, int len = 0);

	/**
	 * Dump email body content into target file
	 * @param file_path {const char*} Target filename
	 * @param src {const char*} Starting address of email content. If it is NULL
	 * pointer,
	 *  extracts email content from emailFile file provided in constructor
	 * @param len {int} Data length of email content. If 0, extracts email content
	 * from
	 *  emailFile file provided in constructor
	 * @return {bool} Whether successful
	 */
	bool save_body(const char* file_path, const char* src = NULL, int len = 0);

	/**
	 * Dump email body content into pipe buffer
	 * @param out {pipe_string&} Pipe buffer
	 * @param src {const char*} Starting address of email content. If it is NULL
	 * pointer,
	 *  extracts email content from emailFile file provided in constructor
	 * @param len {int} Data length of email content. If 0, extracts email content
	 * from
	 *  emailFile file provided in constructor
	 * @return {bool} Whether successful
	 */
	bool save_body(pipe_string& out, const char* src = NULL, int len = 0);

	/**
	 * Dump email body content into buffer
	 * @param out {string&} Buffer
	 * @param src {const char*} Starting address of email content. If it is NULL
	 * pointer,
	 *  extracts email content from emailFile file provided in constructor
	 * @param len {int} Data length of email content. If 0, extracts email content
	 * from
	 *  emailFile file provided in constructor
	 * @return {bool} Whether successful
	 */
	bool save_body(string& out, const char* src = NULL, int len = 0);

	/**
	 * Determine whether subtype in node header type is MIME_STYPE_HTML type
	 * @return {bool}
	 */
	bool html_stype() const;

private:
	bool m_htmlFirst;
};

} // namespace acl

#endif // !defined(ACL_MIME_DISABLE)

