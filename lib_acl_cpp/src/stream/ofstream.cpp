#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stream/ofstream.hpp"
#endif

namespace acl {

ofstream::ofstream(void)
{
}

ofstream::~ofstream(void)
{
}

bool ofstream::open_write(const char* path, bool otrunc /* = true */)
{
	if (otrunc) {
		return open(path, O_WRONLY | O_TRUNC | O_CREAT, 0600);
	} else {
		return open(path, O_WRONLY | O_CREAT, 0600);
	}
}

bool ofstream::open_append(const char* path)
{
	return open(path, O_WRONLY | O_APPEND | O_CREAT, 0600);
}

} // namespace acl
