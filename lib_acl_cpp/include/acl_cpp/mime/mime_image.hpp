#pragma once
#include "../acl_cpp_define.hpp"
#include "mime_node.hpp"

#if !defined(ACL_MIME_DISABLE)

namespace acl {

class ACL_CPP_API mime_image : public mime_node
{
public:
	mime_image(const char* emailFile, const MIME_NODE* node,
		bool enableDecode = true, const char* toCharset = "gb2312",
		off_t off = 0)
		: mime_node(emailFile, node, enableDecode, toCharset, off)
	{
	}

	~mime_image(void) {}

	const char* get_location(void) const;
};

} // namespace acl

#endif // !defined(ACL_MIME_DISABLE)
