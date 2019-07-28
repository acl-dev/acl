#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/mime/mime_image.hpp"
#endif

#if !defined(ACL_MIME_DISABLE)

namespace acl {

const char* mime_image::get_location(void) const
{
	return header_value("Content-Location");
}

} // namespace acl

#endif // !defined(ACL_MIME_DISABLE)
