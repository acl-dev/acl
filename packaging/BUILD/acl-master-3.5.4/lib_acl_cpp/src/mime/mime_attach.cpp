#include "acl_stdafx.hpp"
#include "internal/mime_state.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/mime/rfc2047.hpp"
#include "acl_cpp/mime/mime_attach.hpp"
#endif

#if !defined(ACL_MIME_DISABLE)

namespace acl {

mime_attach::mime_attach(const char* emailFile, const MIME_NODE* node,
	bool enableDecode /* = true */,
	const char* toCharset /* = "gdb2312" */,
	off_t off /* = 0 */)
: mime_node(emailFile, node, enableDecode, toCharset, off)
, m_filename(128)
{
	if (node->header_filename) {
		if (toCharset) {
			rfc2047 rfc;
			rfc.decode_update(node->header_filename,
					(int) strlen(node->header_filename));
			rfc.decode_finish(toCharset, &m_filename);
		} else {
			m_filename = node->header_filename;
		}
	}
}

mime_attach::~mime_attach(void)
{
}

const char* mime_attach::get_filename(void) const
{
	if (m_filename.empty()) {
		return NULL;
	}
	return m_filename.c_str();
}

} // namespace acl

#endif // !defined(ACL_MIME_DISABLE)
