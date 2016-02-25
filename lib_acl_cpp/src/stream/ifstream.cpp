#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stream/ifstream.hpp"
#endif

namespace acl {

bool ifstream::open_read(const char* path)
{
	return open(path, O_RDONLY, 0200);
}

bool ifstream::load(acl::string* s)
{
	if (s == NULL)
		return false;
	if (stream_ == NULL)
		return false;
	if (fseek(0, SEEK_SET) == -1)
		return false;

	char buf[4096];
	int   ret;
	while (1) {
		if ((ret = read(buf, sizeof(buf), false)) < 0)
			break;

		s->append(buf, ret);
	}

	return true;
}

bool ifstream::load(const char* path, acl::string* s)
{
	acl::ifstream fp;
	if (fp.open_read(path) == false)
		return false;
	return fp.load(s);
}

} // namespace acl
