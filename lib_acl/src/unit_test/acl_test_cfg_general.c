#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <string.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_loadcfg.h"
#include "stdlib/acl_array.h"
#include "stdlib/acl_mystring.h"
#include "unit_test/acl_unit_test.h"

#endif

/*------------------------ 通常的配置文件选项处理 --------------------------*/

static int __put_log_level(const ACL_CFG_LINE *cfg_line)
{
	const char *myname = "__put_log_level";

	if (cfg_line->ncount < 2) {
		aut_log_error("%s: ncount=%d", myname, cfg_line->ncount);
		return (-1);
	}

	var_aut_log_level = 0;
	if (strcasecmp(cfg_line->value[1], "print") == 0)
		var_aut_log_level |= VAR_AUT_LOG_PRINT;
	if (strcasecmp(cfg_line->value[1], "fprint") == 0)
		var_aut_log_level |= VAR_AUT_LOG_FPRINT;

	if (strcasecmp(cfg_line->value[1], "print:fprint") == 0)
		var_aut_log_level |= VAR_AUT_LOG_PRINT | VAR_AUT_LOG_FPRINT;
	return (0);
}

/*----------------------------------------------------------------------------*/
static ACL_CFG_FN __general_cfg_tab[] = {
	{ VAR_AUT_LOG, __put_log_level },
	{ NULL, NULL },
};

int aut_cfg_add_general_line(const ACL_CFG_LINE *line)
{
	const char *myname = "aut_cfg_add_general_line";
	int   i;

	if (line->ncount < 1)
		aut_log_fatal("%s: ncount=%d", myname, line->ncount);

	for (i = 0; __general_cfg_tab[i].name != NULL; i++) {
		if (strcasecmp(line->value[0], __general_cfg_tab[i].name) == 0) {
			(void) __general_cfg_tab[i].func(line);

			return (0);
		}
	}

	return (-1);
}

