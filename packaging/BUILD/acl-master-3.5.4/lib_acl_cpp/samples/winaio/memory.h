#pragma once

void* __new(size_t n, const char* filename,
	const char* funcname, int lineno);

//void* __new[](size_t n, const char* filename,
//	    const char* funcname, int lineno);

void __delete(void* p, const char* filename,
	const char* funcname, int lineno);

//void __delete[](void* p, const char* filename,
//	      const char* funcname, int lineno);

inline void* __cdecl operator new(size_t n)
{
	return ::__new(n, __FILE__, __FUNCTION__, __LINE__);
}

inline void __cdecl operator delete(void *p)
{
	::__delete(p, __FILE__, __FUNCTION__, __LINE__);
}

inline void* __cdecl operator new(size_t n, const char* filename, int lineno)
{
	return ::__new(n, filename, __FUNCTION__, lineno);
}

inline void __cdecl operator delete(void *p, const char* filename, int lineno)
{
	::__delete(p, filename, __FUNCTION__, lineno);
}

//#define NEW(n)	(::__new((n), __FILE__, __FUNCTION__, __LINE__))
//#define DELETE(p) (::__delete((p), __FILE__, __FUNCTION__, __LINE__))
