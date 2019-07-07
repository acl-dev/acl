#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stream/aio_delay_free.hpp"
#endif

namespace acl
{

aio_delay_free::aio_delay_free(void)
{
	locked_ = false;
	locked_fixed_ = false;
}

aio_delay_free::~aio_delay_free(void)
{
}

bool aio_delay_free::locked(void) const
{
	return locked_ || locked_fixed_;
}

void aio_delay_free::set_locked(void)
{
	locked_fixed_ = true;
}

void aio_delay_free::unset_locked(void)
{
	locked_fixed_ = false;
}

} // namespace acl
