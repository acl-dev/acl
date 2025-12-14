#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"
#include "mime_node.hpp"

#if !defined(ACL_MIME_DISABLE)

namespace acl {

class ACL_CPP_API mime_attach : public mime_node {
public:
	mime_attach(const char* emailFile, const MIME_NODE* node,
		bool enableDecode = true, const char* toCharset = "gb2312",
		off_t off = 0);

	virtual ~mime_attach();

	/**
	 * Get the attachment filename
	 * @return {const char*} Returns NULL if filename was not found
	 */
	const char* get_filename() const;

private:
	string m_filename;
};

} // namespace acl

#endif // !defined(ACL_MIME_DISABLE)

