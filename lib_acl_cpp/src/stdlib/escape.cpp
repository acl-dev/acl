#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/escape.hpp"
#endif

namespace acl
{

enum {
	escape_prefix = 0x01,
	noescape_min = 0x10,
	escape_shift = 0x40,
};

void escape(const char* in, size_t len, acl::string& out)
{
	char  ch;

	while (len > 0)
	{
		ch = *in;
		if (ch >= noescape_min)
			out << ch;
		else
		{
			out << (char) escape_prefix;
			out << (char) (ch + escape_shift);
		}
		in++;
		len--;
	}
}

bool unescape(const char* in, size_t len, acl::string& out)
{
	char  ch;

	while (len > 0)
	{
		ch = *in;
		if (ch != escape_prefix)
			out << ch;
		else if (*++in == 0)
			return (false);
		else
		{
			ch = *in;
			if (ch < escape_shift)
				return (false);
			ch -= escape_shift;
			out << ch;
		}
		in++;
		len--;
	}

	return (true);
}

} // namespace acl
