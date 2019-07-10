#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/diff_manager.hpp"
#include "acl_cpp/stdlib/diff_string.hpp"
#endif

namespace acl
{

diff_string::diff_string(diff_manager& manager, const char* key, const char* val)
: diff_object(manager)
, range_(-1)
{
	dbuf_guard& dbuf = manager.get_dbuf();
	key_ = dbuf.dbuf_strdup(key);
	val_ = dbuf.dbuf_strdup(val);
}

diff_string::~diff_string(void)
{
}

const char* diff_string::get_val(void) const
{
	return val_;
}

const char* diff_string::get_key(void) const
{
	return key_;
}

bool diff_string::operator== (const diff_object& obj) const
{
	const diff_string& o = (const diff_string&) obj;

	return strcmp(o.val_, val_) == 0 ? true : false;
}

bool diff_string::check_range(long long range_from, long long range_to) const
{
	if(range_from == -1 || range_to == -1) {
		return true;
	}

	if (range_ < 0) {
		return true;
	}

	return (range_ >= range_from && range_ <= range_to);
}

void diff_string::set_range(long long range)
{
	range_ = range;
}

} // namespace acl
