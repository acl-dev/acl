#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include "stdlib/acl_sys_patch.h"

#endif

#ifdef	ACL_WINDOWS
void sleep(int sec)
{
	Sleep((sec * 1000));
}
#endif
