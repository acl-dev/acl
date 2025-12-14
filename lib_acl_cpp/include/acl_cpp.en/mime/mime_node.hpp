#pragma once
#include "../acl_cpp_define.hpp"
#include <stdlib.h>
#include <map>
#include "../stdlib/noncopyable.hpp"
#include "../stdlib/string.hpp"

#if !defined(ACL_MIME_DISABLE)

struct MIME_NODE;

namespace acl {

class pipe_manager;
class ostream;
class ifstream;

class ACL_CPP_API mime_node : public noncopyable {
public:
	/**
	 * Constructor
	 * @param emailFile {const char*} Source file name storing email data. If
	 *  empty, when calling save_body function later, the source file must be specified.
	 * @param node {const MIME_NODE*} A certain MIME node in the email.
	 * @param enableDecode {bool} Whether to automatically decode when email content is in base64/qp encoding format.
	 * @param toCharset {const char*} Default target character set. If target
	 *  character set is different from source character set, character set conversion will be performed.
	 * @param off {off_t} Starting position offset in the email body data stream, with additional
	 *  offset added, so that users can add their own private data before the email body.
	 */
	mime_node(const char* emailFile, const MIME_NODE* node,
		bool enableDecode = true, const char* toCharset = "gb2312",
		off_t off = 0);
	virtual ~mime_node();

	/**
	 * Get the name field value in the MIME node's Content-Type value.
	 * @return {const char*} Returns NULL if there is no such field value.
	 */
	const char* get_name() const {
		if (m_name.empty()) {
			return NULL;
		}
		return m_name.c_str();
	}

	/**
	 * Get the main type in Content-Type, e.g.: Content-Type: image/jpeg, then
	 * returns MIME_CTYPE_IMAGE (defined in mime_define.hpp)
	 * @return {int} Returns MIME_CTYPE_XXX defined in mime_define.hpp
	 */
	int get_ctype() const {
		return m_ctype;
	}

	/**
	 * Get the sub type in Content-Type, e.g.: Content-Type: image/jpeg, then
	 * returns MIME_STYPE_JPEG (defined in mime_define.hpp)
	 * @return {int} Returns MIME_STYPE_XXX defined in mime_define.hpp
	 */
	int get_stype() const {
		return m_stype;
	}

	/**
	 * Get the main type in Content-Type, represented as a string format.
	 * @return {const char*} Returns "" if unknown.
	 */
	const char* get_ctype_s() const;

	/**
	 * Get the sub type in Content-Type, represented as a string format.
	 * @return {const char*} Returns "" if unknown.
	 */
	const char* get_stype_s() const;

	/**
	 * Get encoding method (corresponding to Content-Transfer-Encoding)
	 * @return {int} Returns MIME_ENC_XXX defined in mime_define.hpp
	 */
	int get_encoding() const {
		return m_encoding;
	}

	/**
	 * Get character set string (corresponding to charset field in Content-Type)
	 * @return {const char*} Returns NULL if there is no such field.
	 */
	const char* get_charset() const {
		return m_charset;
	}

	/**
	 * Get target character set. If not set in constructor.
	 * @return {const char*} Returns NULL if user has not set it.
	 */
	const char* get_toCharset() const {
		if (m_toCharset[0]) {
			return m_toCharset;
		} else {
			return NULL;
		}
	}

	/**
	 * Get the starting offset of this node in the email.
	 * @return {off_t}
	 */
	off_t get_bodyBegin() const {
		return m_bodyBegin;
	}

	/**
	 * Get the ending offset of this node in the email.
	 * @return {off_t}
	 */
	off_t get_bodyEnd() const {
		return m_bodyEnd;
	}

	/**
	 * Get the value of a certain header field in this node.
	 * @param name {const char*} Field name, e.g.: Content-Type
	 * @return {const char*} Returns NULL if not found.
	 */
	const char* header_value(const char* name) const;

	/**
	 * Get the header field collection of this node.
	 * @return {const std::map<string, string>&}
	 */
	const std::map<string, string>& get_headers() const;

	/**
	 * Convert this node's body data to the specified pipe stream.
	 * @param out {pipe_manager&}
	 * @return {bool} Whether successful
	 */
	bool save(pipe_manager& out) const;

	/**
	 * Convert this node's body data to the specified pipe stream.
	 * @param out {pipe_manager&}
	 * @param src {const char*} Starting address of email data. If NULL pointer,
	 *  then use the emailFile file provided by the constructor to read email data.
	 * @param len {int} Length of email data content. If 0, then use the constructor's
	 *  provided emailFile file to read email data.
	 * @return {bool} Whether successful
	 */
	bool save(pipe_manager& out, const char* src, int len) const;

	/**
	 * Convert this node's body data to the specified output stream.
	 * @param out {ostream&} Output stream
	 * @param src {const char*} Starting address of email data. If NULL pointer,
	 *  then use the emailFile file provided by the constructor to read email data.
	 * @param len {int} Length of email data content. If 0, then use the constructor's
	 *  provided emailFile file to read email data.
	 * @return {bool} Whether successful
	 */
	bool save(ostream& out, const char* src = NULL, int len = 0) const;

	/**
	 * Convert this node's body data to the specified file.
	 * @param outFile {const char*} Target file name
	 * @param src {const char*} Starting address of email data. If NULL pointer,
	 *  then use the emailFile file provided by the constructor to read email data.
	 * @param len {int} Length of email data content. If 0, then use the constructor's
	 *  provided emailFile file to read email data.
	 * @return {bool} Whether successful
	 */
	bool save(const char* outFile, const char* src = NULL, int len = 0) const;

	/**
	 * Convert this node's body data to a string buffer.
	 * @param out {string&} Output buffer
	 * @param src {const char*} Starting address of email data. If NULL pointer,
	 *  then use the emailFile file provided by the constructor to read email data.
	 * @param len {int} Length of email data content. If 0, then use the constructor's
	 *  provided emailFile file to read email data.
	 * @return {bool} Whether successful
	 */
	bool save(string& out, const char* src, int len) const;

	/**
	 * Get the parent node corresponding to this node.
	 * @return {mime_node*} Returns NULL if this node has no parent (i.e.,
	 *  this node is the email's root node); otherwise, returns the parent node. The caller needs to use
	 *  delete to release the corresponding memory.
	 */
	mime_node* get_parent() const;

	/**
	 * Determine whether this node has a parent.
	 * @return {bool} true means there is a parent, false means there is not.
	 */
	bool has_parent() const;

	/**
	 * Get parent node's main type (MIME_CTYPE_XXX). If return value is MIME_CTYPE_OTHER,
	 * it means this node does not exist or parent node's main type is unknown.
	 * @return {int} MIME_CTYPE_XXX
	 */
	int parent_ctype() const;
	const char* parent_ctype_s() const;

	/**
	 * Get parent node's sub type (MIME_STYPE_XXX). If return value is MIME_STYPE_OTHER,
	 * it means this node does not exist or parent node's sub type is unknown.
	 * @return {int} MIME_STYPE_XXX
	 */
	int parent_stype() const;
	const char* parent_stype_s() const;

	/**
	 * Get parent node's encoding method (MIME_ENC_XXX). If return value is MIME_ENC_OTHER,
	 * it means this node does not exist or parent node's encoding method is unknown.
	 * @return {int} MIME_ENC_XXX
	 */
	int parent_encoding() const;

	/**
	 * Get parent node's character set string. If return value is NULL, it means this node does not exist or parent
	 * node has no character set string.
	 * @return {const char*}
	 */
	char* parent_charset() const;

	/**
	 * Get parent node's body starting offset.
	 * @return {off_t} Return value of -1 means this node does not exist.
	 */
	off_t parent_bodyBegin() const;

	/**
	 * Get parent node's body ending offset.
	 * @return {off_t} Return value of -1 means this node does not exist.
	 */
	off_t parent_bodyEnd() const;

	/**
	 * Get the value corresponding to a certain header field name in parent node, e.g.: Content-Type
	 * @param name {const char*} Field name
	 * @return {const char*} Field value. Returns empty if this node does not exist
	 *  or parent node's header does not contain this field.
	 */
	const char* parent_header_value(const char* name) const;

protected:
	bool  m_enableDecode;
	string m_name;
	string m_emailFile;
	int   m_ctype;		// mime_define.hpp
	int   m_stype;		// mime_define.hpp
	int   m_encoding;	// mime_define.hpp
	char  m_charset[32];
	char  m_toCharset[32];
	off_t m_bodyBegin;
	off_t m_bodyEnd;
	std::map<string, string>* m_headers_;
	const MIME_NODE* m_pMimeNode;
	mime_node* m_pParent;
};

} // namespace acl

#endif // !defined(ACL_MIME_DISABLE)

