#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_dll.h"

#ifdef ACL_UNIX
#include <dlfcn.h>
#endif

#endif

ACL_DLL_HANDLE acl_dlopen(const char *dlname)
{
	const char *myname = "acl_dlopen";
	ACL_DLL_HANDLE handle;

#ifdef ACL_UNIX
	if (1)
		handle = dlopen(dlname, RTLD_LOCAL | RTLD_LAZY);
	else
		handle = dlopen(dlname, RTLD_GLOBAL | RTLD_NOW);
#elif defined(WIN32)
	handle = LoadLibrary(dlname);
#endif
	if (handle == NULL)
		acl_msg_error("%s(%d): open(%s) error(%s)",
			myname, __LINE__, dlname, acl_last_serror());
	return (handle);
}

void acl_dlclose(ACL_DLL_HANDLE handle)
{
#ifdef ACL_UNIX
	dlclose(handle);
#elif defined(WIN32)
	FreeLibrary(handle);
#endif
}

ACL_DLL_FARPROC acl_dlsym(void *handle, const char *name)
{
#ifdef ACL_UNIX
	return (dlsym(handle, name));
#elif defined(WIN32)
	return (GetProcAddress(handle, name));
#endif
}

