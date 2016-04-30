#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE
#include "stdlib/acl_mymalloc.h"
#endif
#include "fiber/filber.h"

FIBER *acl_fiber_create(void)
{
	FIBER *fiber = (FIBER*) acl_mycalloc(1, sizeof(FIBER));
	return fiber;
}
