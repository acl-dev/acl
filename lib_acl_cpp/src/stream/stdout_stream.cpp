#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stream/stdout_stream.hpp"
#endif

namespace acl
{

stdout_stream::stdout_stream(void)
{
	stream_ = ACL_VSTREAM_OUT;
	eof_    = false;
	opened_ = true;
}

stdout_stream::~stdout_stream(void)
{
	stream_ = NULL;
}

} // namespace acl
