#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/util.hpp"
#endif

#include "acl_cpp/stdlib/trigger.hpp"

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
	if (n == false) {
		abort();
	}
}

void meter_time(const char *filename, int line, const char *info)
{
	acl_meter_time(filename, line, info);
}

long long get_curr_stamp(void)
{
	struct timeval now;
	gettimeofday(&now, NULL);

	return (long long) now.tv_sec * 1000 + (long long) now.tv_usec / 1000;
}

double stamp_sub(const struct timeval& from, const struct timeval& sub)
{
	struct timeval res;

	memcpy(&res, &from, sizeof(struct timeval));

	res.tv_usec -= sub.tv_usec;
	if (res.tv_usec < 0) {
		--res.tv_sec;
		res.tv_usec += 1000000;
	}

	res.tv_sec -= sub.tv_sec;
	return res.tv_sec * 1000.0 + res.tv_usec / 1000.0;
}

} // namespace acl
