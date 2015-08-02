#include "acl_stdafx.hpp"
#include "acl_cpp/stream/stdout_stream.hpp"

namespace acl
{

stdout_stream::stdout_stream()
{
	stream_ = ACL_VSTREAM_OUT;
	eof_ = false;
	opened_ = true;
}

stdout_stream::~stdout_stream()
{
	stream_ = NULL;
}

} // namespace acl
