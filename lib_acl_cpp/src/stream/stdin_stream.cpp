#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stream/stdin_stream.hpp"
#endif

namespace acl
{

stdin_stream::stdin_stream()
{
	stream_ = ACL_VSTREAM_IN;
	eof_ = false;
	opened_ = true;
}

stdin_stream::~stdin_stream()
{
	stream_ = NULL;
}

} // namespace acl
