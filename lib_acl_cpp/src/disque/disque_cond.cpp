#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/disque/disque_cond.hpp"
#endif

namespace acl
{

disque_cond::disque_cond()
	: replicate_(0)
	, delay_(-1)
	, retry_(0)
	, ttl_(0)
	, maxlen_(0)
	, async_(false)
{

}

disque_cond::~disque_cond()
{

}

disque_cond& disque_cond::set_replicate(int n)
{
	replicate_ = n;
	return *this;
}

disque_cond& disque_cond::set_delay(int n)
{
	delay_ = n;
	return *this;
}

disque_cond& disque_cond::set_retry(int n)
{
	retry_ = n;
	return *this;
}

disque_cond& disque_cond::set_ttl(int n)
{
	ttl_ = n;
	return *this;
}

disque_cond& disque_cond::set_maxlen(int n)
{
	maxlen_ = n;
	return *this;
}

disque_cond& disque_cond::set_async(bool on)
{
	async_ = on;
	return *this;
}

} // namespace acl
