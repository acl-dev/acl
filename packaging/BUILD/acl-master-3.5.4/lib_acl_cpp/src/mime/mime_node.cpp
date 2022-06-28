#include "acl_stdafx.hpp"
#include "internal/header_opts.hpp"
#include "internal/mime_state.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stdlib/pipe_stream.hpp"
#include "acl_cpp/mime/mime_define.hpp"
#include "acl_cpp/mime/rfc2047.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stream/ifstream.hpp"
#include "acl_cpp/stream/ofstream.hpp"
#include "acl_cpp/mime/mime_code.hpp"
#include "acl_cpp/mime/mime_node.hpp"
#endif

#if !defined(ACL_MIME_DISABLE)

#define	SCOPY(x, y)	ACL_SAFE_STRNCPY((x), (y), sizeof((x)))

namespace acl {

mime_node::mime_node(const char* emailFile, const MIME_NODE* node,
	bool enableDecode /* = true */,
	const char* toCharset /* = "gb2312" */,
	off_t off /* = 0 */)
: m_name(128)
, m_headers_(NULL)
, m_pMimeNode(node)
, m_pParent(NULL)
{
	if (emailFile) {
		m_emailFile = emailFile;
	}
	m_enableDecode = enableDecode;
	if (toCharset) {
		SCOPY(m_toCharset, toCharset);
	} else {
		m_toCharset[0] = 0;
	}

	if (node->header_name) {
		if (toCharset) {
			rfc2047 rfc;
			rfc.decode_update(node->header_name,
				(int) strlen(node->header_name));
			rfc.decode_finish(toCharset, &m_name);
		} else {
			m_name = node->header_name;
		}
	}

	m_ctype = node->ctype;
	m_stype = node->stype;
	m_encoding = node->encoding;
	if (node->charset) {
		SCOPY(m_charset, node->charset);
	} else {
		m_charset[0] = 0;
	}
	m_bodyBegin = node->body_begin + off;
	m_bodyEnd = node->body_data_end + off;
}

mime_node::~mime_node(void)
{
	delete m_headers_;
	delete m_pParent;
}

const char* mime_node::get_ctype_s(void) const
{
	return m_pMimeNode->ctype_s ? m_pMimeNode->ctype_s : "";
}

const char* mime_node::get_stype_s(void) const
{
	return m_pMimeNode->stype_s ? m_pMimeNode->stype_s : "";
}

const char* mime_node::header_value(const char* name) const
{
	ACL_ITER iter;

	acl_foreach(iter, m_pMimeNode->header_list) {
		HEADER_NV* hdr = (HEADER_NV*) iter.data;
		if (strcasecmp(hdr->name, name) == 0 && *hdr->value) {
			return hdr->value;
		}
	}

	return NULL;
}

const std::map<string, string>& mime_node::get_headers(void) const
{
	if (m_headers_ != NULL) {
		return *m_headers_;
	}

	const_cast<mime_node*> (this)->m_headers_ =
		NEW std::map<string, string>;

	ACL_ITER iter;

	acl_foreach(iter, m_pMimeNode->header_list) {
		HEADER_NV* hdr = (HEADER_NV*) iter.data;
		if (*hdr->value == 0) {
			continue;
		}
		std::pair<string, string> entry(hdr->name, hdr->value);
		const_cast<mime_node*> (this)->m_headers_->insert(entry);
	}

	return *m_headers_;
}

bool mime_node::save(pipe_manager& out) const
{
	if (m_emailFile.empty()) {
		logger_error("m_emailFile empty!");
		return false;
	}

	ifstream in ;
	if (!in.open_read(m_emailFile)) {
		logger_error("open input file %s error(%s)",
			m_emailFile.c_str(), last_serror());
		return false;
	}

	if (m_bodyBegin < 0 || m_bodyEnd <= m_bodyBegin) {
		return true;
	}

	if (in.fseek(m_bodyBegin, SEEK_SET) < 0) {
		logger_error("fseek error(%s)", last_serror());
		return false;
	}

	// 当需要解码时查找匹配的解码器

	mime_code* mime_decoder;
	if (m_enableDecode) {
		mime_decoder = mime_code::create(m_encoding, false);
		if (mime_decoder) {
			mime_decoder->set_status(false);
			out.push_front(mime_decoder);
		}
	} else {
		mime_decoder = NULL;
	}

	char   buf[8192];
	size_t size;
	int    len = (int) (m_bodyEnd - m_bodyBegin);

	int   ret;
	while (len > 0) {
		size = sizeof(buf) > (size_t) len
			? (size_t) len : sizeof(buf);
		ret = in.read(buf, size, true);
		if (ret < 0) {
			logger_error("read error(%s), ret: %d",
				last_serror(), ret);
			delete mime_decoder;
			return false;
		}

		if (!out.update(buf, ret)) {
			delete mime_decoder;
			return false;
		}
		len -= ret;
	}

	bool result = out.update_end();

	delete mime_decoder;
	return result;
}

bool mime_node::save(pipe_manager& out, const char* src, int len) const
{
	if (src == NULL || len <= 0) {
		return save(out);
	}

	if (m_bodyBegin < 0 || m_bodyEnd <= m_bodyBegin) {
		return true;
	} else if (len < m_bodyEnd) {
		return true;
	} 

	// 当需要解码时查找匹配的解码器

	mime_code* mime_decoder;
	if (m_enableDecode) {
		mime_decoder = mime_code::create(m_encoding, false);
		if (mime_decoder) {
			mime_decoder->set_status(false);
			out.push_front(mime_decoder);
		}
	} else {
		mime_decoder = NULL;
	}

	size_t n = (size_t) (m_bodyEnd - m_bodyBegin);
	if (!out.update(src + m_bodyBegin, n)) {
		delete mime_decoder;
		return false;
	}

	bool result = out.update_end();

	delete mime_decoder;
	return result;
}

bool mime_node::save(ostream& out, const char* src /* = NULL */,
	int len /* = 0 */) const
{
	pipe_manager manager;
	manager.push_front(&out);
	return save(manager, src, len);
}

bool mime_node::save(const char* outFile, const char* src /* = NULL */,
	int len /* = 0 */) const
{
	ofstream out;

	if (!out.open_trunc(outFile)) {
		logger_error("open %s error(%s)", outFile, last_serror());
		return false;
	}
	return save(out, src, len);
}

bool mime_node::save(string& out, const char* src /* = NULL */,
	int len /* = 0 */) const
{
	pipe_manager manager;
	pipe_string ps(out);
	manager.push_front(&ps);
	return save(manager, src, len);
}

mime_node* mime_node::get_parent(void) const
{
	if (m_pParent) {
		return m_pParent;
	}

	const MIME_NODE* node = m_pMimeNode->parent;
	if (node == NULL) {
		return NULL;
	}
	const_cast<mime_node*>(this)->m_pParent =
		NEW mime_node(m_emailFile.c_str(),
			node, m_enableDecode, m_toCharset);
	return m_pParent;
}

bool mime_node::has_parent(void) const
{
	return m_pMimeNode->parent == NULL ? false : true;
}

int mime_node::parent_ctype(void) const
{
	if (m_pMimeNode->parent == NULL) {
		return MIME_CTYPE_OTHER;
	}
	return m_pMimeNode->parent->ctype;
}

int mime_node::parent_stype(void) const
{
	if (m_pMimeNode->parent == NULL) {
		return MIME_STYPE_OTHER;
	}
	return m_pMimeNode->parent->stype;
}

const char* mime_node::parent_ctype_s(void) const
{
	if (m_pMimeNode->parent == NULL) {
		return "";
	}
	const char* ptr = m_pMimeNode->parent->ctype_s;
	return ptr ? ptr : "";
}

const char* mime_node::parent_stype_s(void) const
{
	if (m_pMimeNode->parent == NULL) {
		return "";
	}
	const char* ptr = m_pMimeNode->parent->stype_s;
	return ptr ? ptr : "";
}

int mime_node::parent_encoding() const
{
	if (m_pMimeNode->parent == NULL) {
		return MIME_ENC_OTHER;
	}
	return m_pMimeNode->parent->encoding;
}

char* mime_node::parent_charset(void) const
{
	if (m_pMimeNode->parent == NULL) {
		return NULL;
	}
	return m_pMimeNode->parent->charset;
}

off_t mime_node::parent_bodyBegin(void) const
{
	if (m_pMimeNode->parent == NULL) {
		return -1;
	}
	return m_pMimeNode->parent->body_begin;
}

off_t mime_node::parent_bodyEnd(void) const
{
	if (m_pMimeNode->parent == NULL) {
		return -1;
	}
	return m_pMimeNode->parent->body_end;
}

const char* mime_node::parent_header_value(const char* name) const
{
	if (m_pMimeNode->parent == NULL) {
		return NULL;
	}

	ACL_ITER iter;

	acl_foreach(iter, m_pMimeNode->parent->header_list) {
		HEADER_NV* hdr = (HEADER_NV*) iter.data;
		if (strcasecmp(hdr->name, name) == 0 && *hdr->value) {
			return hdr->value;
		}
	}

	return NULL;
}

} // namespace acl

#endif // !defined(ACL_MIME_DISABLE)
