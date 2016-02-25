#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/util.hpp"
#endif

namespace acl
{

int last_error(void)
{
	return acl_last_error();
}

void set_error(int errnum)
{
	acl_set_error(errnum);
}

const char* last_serror(void)
{
	return acl_last_serror();
}

const char* last_serror(char* buf, size_t size)
{
	return acl_last_strerror(buf, (int) size);
}

const char* string_error(int errnum, char* buf, size_t size)
{
	return acl_strerror((unsigned int) errnum, buf, (int) size);
}

int strncasecmp_(const char *s1, const char *s2, size_t n)
{
	return ::strncasecmp(s1, s2, n);
}

void assert_(bool n)
{
	if (n == false)
		abort();
}

void meter_time(const char *filename, int line, const char *info)
{
	acl_meter_time(filename, line, info);
}

} // namespace acl
