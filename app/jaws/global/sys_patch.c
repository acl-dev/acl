#include "lib_acl.h"
#include <string.h>
#include "sys_patch.h"

#ifdef ACL_MS_WINDOWS

struct tm *localtime_r(const time_t *timep, struct tm *result)
{
	struct tm *tmp;
	tmp = localtime(timep);
	if (result)
		memcpy(result, tmp, sizeof(struct tm));
	return (tmp);
}

#endif

