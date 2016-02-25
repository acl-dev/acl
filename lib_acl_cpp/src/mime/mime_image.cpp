#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/mime/mime_image.hpp"
#endif

namespace acl {

const char* mime_image::get_location() const
{
	return (header_value("Content-Location"));
}

} // namespace acl
