#include "acl_stdafx.hpp"

#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/string.hpp"
#endif

#include "acl_cpp/acl_cpp_init.hpp"

namespace acl
{

void acl_cpp_init(void)
{
	acl_lib_init();
}

const char* acl_cpp_verbose(void)
{
    static string buf;
    buf.clear();

#ifdef HAS_MBEDTLS
    buf += "HAS_MBEDTLS";
# ifdef HAS_MBEDTLS_DLL
    buf += ", HAS_MBEDTLS_DLL";
# endif
#endif

#ifdef HAS_POLARSSL
    buf += ", HAS_POLARSSL";
# ifdef HAS_POLARSSL_DLL
    buf += ", HAS_POLARSSL_DLL";
# endif
#endif

#ifdef ACL_CLIENT_ONLY
    buf += ", ACL_CLIENT_ONLY";
#endif

#ifdef ACL_HOOK_NEW
    buf += ", ACL_HOOK_NEW";
#endif

    return buf.c_str();
}

#ifdef ACL_WINDOWS

static FILE* dos_fp_ = NULL;

void open_dos(void)
{
	if (dos_fp_)
		return;

	// ´ò¿ª DOS ´°¿Ú
	AllocConsole();

#if _MSC_VER >= 1500
	if (freopen_s(&dos_fp_,"CONOUT$", "w+t", stdout) != 0)
		dos_fp_ = NULL;
#else
	dos_fp_ = freopen("CONOUT$", "w+t", stdout);
#endif
	if (dos_fp_ == NULL)
	{
		printf("open DOS error %s\r\n", last_serror());
		FreeConsole();
	}
}

void close_dos(void)
{
	if (dos_fp_)
	{
		fclose(dos_fp_);
		FreeConsole();
		dos_fp_ = NULL;
	}
}

#endif

}  // namespace acl
