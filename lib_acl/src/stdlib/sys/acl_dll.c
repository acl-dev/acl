#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_dll.h"
#include "stdlib/acl_mymalloc.h"

#ifdef ACL_UNIX
#include <dlfcn.h>
#endif

#endif

ACL_DLL_HANDLE acl_dlopen(const char *dlname)
{
	const char *myname = "acl_dlopen";
	ACL_DLL_HANDLE handle;

#ifdef ACL_UNIX
# ifdef MINGW
	handle = dlopen(dlname, RTLD_LAZY);
# else
#  if 1
    handle = dlopen(dlname, RTLD_LOCAL | RTLD_LAZY);
#  else
    handle = dlopen(dlname, RTLD_GLOBAL | RTLD_NOW);
#  endif
# endif
	if (handle != NULL)
		dlerror();  /* clear any existing error */
#elif defined(ACL_WINDOWS)
	{
		int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
			dlname, -1, NULL, 0);
		if (n > 0) {
			wchar_t *wname = (wchar_t *) acl_mymalloc(sizeof(wchar_t) * n);
			if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
				dlname, -1, wname, n) > 0)
			{
				handle = LoadLibraryW(wname);
			} else {
				handle = NULL;
			}
			acl_myfree(wname);
		} else {
			handle = LoadLibrary(dlname);
		}
	}
#endif
	if (handle == NULL)
		acl_msg_error("%s(%d): open(%s) error(%s)",
			myname, __LINE__, dlname, acl_last_serror());
	return handle;
}

void acl_dlclose(ACL_DLL_HANDLE handle)
{
#ifdef ACL_UNIX
	dlclose(handle);
#elif defined(ACL_WINDOWS)
	FreeLibrary(handle);
#endif
}

ACL_DLL_FARPROC acl_dlsym(void *handle, const char *name)
{
#ifdef ACL_UNIX
	return dlsym(handle, name);
#elif defined(ACL_WINDOWS)
	return GetProcAddress(handle, name);
#endif
}

const char *acl_dlerror(void)
{
#ifdef ACL_UNIX
	const char *ptr = dlerror();
	return ptr == NULL ? "" : ptr;
#elif defined(ACL_WINDOWS)
	return acl_last_serror();
#endif
}
