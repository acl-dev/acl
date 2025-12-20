#pragma once

#include <memory>
#include "dbuf_pool.hpp"

namespace acl {

template <typename T>
class dbuf_allocator : public std::allocator<T> {
public:
	typedef size_t   size_type;
	typedef typename dbuf_allocator<T>::pointer         pointer;
	typedef typename dbuf_allocator<T>::value_type      value_type;
	typedef typename dbuf_allocator<T>::const_pointer   const_pointer;
	typedef typename dbuf_allocator<T>::reference       reference;
	typedef typename dbuf_allocator<T>::const_reference const_reference;

	pointer allocate(size_type _Count, const void* _Hint = NULL) {
		(void) _Hint;
		pointer p = (pointer) dbuf_.dbuf_alloc(_Count * sizeof(T));
		return p;
	}

	void deallocate(pointer _Ptr, size_type _Count) {
		(void) _Ptr;
		(void) _Count;
	}  

	template<class _Other>
	struct rebind {
		// convert this type to allocator<_Other>
		typedef dbuf_allocator<_Other> other;
	};  

	dbuf_allocator(void) throw() {}

	dbuf_allocator(const dbuf_allocator& __a) throw()
	: std::allocator<T> (__a) {
		(void) __a;
	}

	template<typename _Tp1>
	dbuf_allocator(const dbuf_allocator<_Tp1>&) throw() {}

	~dbuf_allocator(void) throw() {}

private:
	dbuf_guard dbuf_;
};
  
} // namespace acl
