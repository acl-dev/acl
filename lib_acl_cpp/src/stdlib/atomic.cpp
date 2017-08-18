#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/atomic.hpp"
#endif

namespace acl
{

template<typename T>
atomic<T>::atomic(T* t)
{
	atomic_ = acl_atomic_new();
	acl_atomic_set(atomic_, t);
}

template<typename T>
atomic<T>::~atomic(void)
{
	acl_atomic_free(atomic_);
}

template<typename T>
T* atomic<T>::cas(T* cmp, T* val)
{
	return (T*) acl_atomic_cas(atomic_, cmp, val);
}

template<typename T>
T* atomic<T>::xchg(T* val)
{
	return (T*) acl_atomic_xchg(atomic_, val);
}

//////////////////////////////////////////////////////////////////////////////

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
