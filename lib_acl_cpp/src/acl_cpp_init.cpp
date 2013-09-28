#include "acl_stdafx.hpp"
#include "acl_cpp/acl_cpp_init.hpp"

namespace acl
{

void acl_cpp_init(void)
{
	acl_init();
}

#ifdef WIN32

static FILE* dos_fp_ = NULL;

void open_dos(void)
{
	if (dos_fp_)
		return;
	// ´ò¿ª DOS ´°¿Ú
	AllocConsole();
	dos_fp_ = freopen("CONOUT$","w+t",stdout);
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
