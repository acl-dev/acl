#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/atomic.hpp"
#endif

namespace acl
{

atomic_long::atomic_long(long long n)
: atomic<long long>(&n_)
{
	acl_atomic_int64_set(atomic_, n);
}

void atomic_long::set(long long n)
{
	acl_atomic_int64_set(atomic_, n);
}

long long atomic_long::fetch_add(long long n)
{
	return acl_atomic_int64_fetch_add(atomic_, n);
}

long long atomic_long::add_fetch(long long n)
{
	return acl_atomic_int64_add_fetch(atomic_, n);
}

} // namespace acl
