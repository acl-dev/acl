#include "acl_stdafx.hpp"
#include "acl_cpp/mime/mime_image.hpp"

namespace acl {

const char* mime_image::get_location() const
{
	return (header_value("Content-Location"));
}

} // namespace acl
