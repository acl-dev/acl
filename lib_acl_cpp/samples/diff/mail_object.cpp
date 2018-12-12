#include "stdafx.h"
#include "mail_object.h"

mail_object::mail_object(acl::diff_manager& manager, const char* key, const char* val)
: diff_object(manager)
, ctime_(-1)
{
	acl::dbuf_guard& dbuf = manager.get_dbuf();
	key_ = dbuf.dbuf_strdup(key);
	val_ = dbuf.dbuf_strdup(val);
}

mail_object::~mail_object()
{
}

const char* mail_object::get_val() const
{
	return val_;
}

const char* mail_object::get_key() const
{
	return key_;
}

bool mail_object::operator== (const acl::diff_object& obj) const
{
	const mail_object& mo = (const mail_object&) obj;

	return strcmp(mo.val_, val_) == 0 ? true : false;
}

bool mail_object::check_range(long long from, long long to) const
{
	if (from == -1 && to == -1)
		return true;

	if (ctime_ < 0)
		return true;

	return (ctime_ >= from && ctime_ <= to);
}

void mail_object::set_ctime(long long n)
{
	ctime_ = n;
}
