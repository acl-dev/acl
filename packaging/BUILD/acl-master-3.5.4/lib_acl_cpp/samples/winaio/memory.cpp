#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include "malloc.hpp"
#include "memory.h"

void* __new(size_t n, const char* filename,
	const char* funcname, int lineno)
{
	printf(">>>%s(%d), %s: new size: %d\n",
		filename, lineno, funcname, (int) n);

	return (acl::acl_new(n, filename, funcname, lineno));
}

//void* __new[](size_t n, const char* filename,
//	const char* funcname, int lineno)
//{
//
//}

void __delete(void* p, const char* filename,
	const char* funcname, int lineno)
{
	printf(">>>%s(%d), %s: delete\n",
		filename, lineno, funcname);
	if (p)
		acl::acl_delete(p, filename, funcname, lineno);
}

//void __delete[](void* p, const char* filename,
//	const char* funcname, int lineno)
//{
//
//}
