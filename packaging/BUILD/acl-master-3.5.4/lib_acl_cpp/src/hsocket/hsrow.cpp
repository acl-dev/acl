#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include <vector>
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/escape.hpp"
#include "acl_cpp/hsocket/hsrow.hpp"
#endif

#ifndef ACL_CLIENT_ONLY

namespace acl
{

hsrow::hsrow(int ncolum)
: ncolum_(ncolum)
, icolum_(0)
{
	colums_ = new string[ncolum_];
}

hsrow::~hsrow(void)
{
	delete[] colums_;
}

void hsrow::reset(int ncolum)
{
	icolum_ = 0;
	row_.clear();
	if (ncolum <= ncolum_) {
		return;
	}
	delete[] colums_;
	ncolum_ = ncolum;
	colums_ = new string[ncolum_];
}

void hsrow::push_back(const char* value, size_t dlen)
{
	if (icolum_ >= ncolum_) {
		logger_error("icolum_(%d) >= ncolum_(%d)",
			icolum_, ncolum_);
		return;
	}

	static const char* dummy_ = "";
	if (*value == 0) {
		row_.push_back(dummy_);
		icolum_++;
		return;
	}

	string* buf = &colums_[icolum_];
	buf->clear();
	unescape(value, dlen, *buf);
	row_.push_back(buf->c_str());
	icolum_++;
}

const std::vector<const char*>& hsrow::get_row(void) const
{
	return row_;
}

}  // namespace acl

#endif // ACL_CLIENT_ONLY
