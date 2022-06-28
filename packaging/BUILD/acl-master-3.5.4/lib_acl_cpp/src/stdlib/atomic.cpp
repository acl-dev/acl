#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/atomic.hpp"
#endif

namespace acl
{

void* atomic_new(void)
{
	return acl_atomic_new();
}

void  atomic_free(void* atomic)
{
	acl_atomic_free((ACL_ATOMIC*) atomic);
}

void  atomic_set(void* atomic, void* value)
{
	acl_atomic_set((ACL_ATOMIC*) atomic, value);
}

void* atomic_cas(void* atomic, void* cmp, void* value)
{
	return acl_atomic_cas((ACL_ATOMIC*) atomic, cmp, value);
}

void* atomic_xchg(void* atomic, void* value)
{
	return acl_atomic_xchg((ACL_ATOMIC*) atomic, value);
}

/////////////////////////////////////////////////////////////////////////////

atomic_long::atomic_long(long long n)
: atomic<long long>(&n_)
{
	n_ = n;
	acl_atomic_int64_set((ACL_ATOMIC*) atomic_, n_);
}

atomic_long::atomic_long(const atomic_long& n)
: atomic<long long>(&n_)
{
	n_ = n.value();
	acl_atomic_int64_set((ACL_ATOMIC*) atomic_, n_);
}

void atomic_long::set(long long n)
{
	n_ = n;
	acl_atomic_int64_set((ACL_ATOMIC*) atomic_, n);
}

long long atomic_long::cas(long long cmp, long long n)
{
	return acl_atomic_int64_cas((ACL_ATOMIC*) atomic_, cmp, n);
}

long long atomic_long::fetch_add(long long n)
{
	return acl_atomic_int64_fetch_add((ACL_ATOMIC*) atomic_, n);
}

long long atomic_long::add_fetch(long long n)
{
	return acl_atomic_int64_add_fetch((ACL_ATOMIC*) atomic_, n);
}

} // namespace acl
