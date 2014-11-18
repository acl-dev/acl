#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_msg.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_mystring.h"
#include "stdlib/acl_array.h"
#include "unit_test/acl_unit_test.h"

#endif

/*---------------------------- 分析外部命令配置行 --------------------------*/

AUT_LINE *aut_add_outer_cmd(const ACL_CFG_LINE *cfg_line)
{
	const char *myname = "aut_add_outer_cmd";
	AUT_LINE *test_line;

	if (cfg_line->ncount < 3)
		aut_log_fatal("%s: cmd_name=%s, ncount=%d, input error, "
				"please check configure file",
				myname, cfg_line->value[0], cfg_line->ncount);

	test_line = (AUT_LINE *) acl_mycalloc(1, sizeof(*test_line));

	if (test_line == NULL) {
		char tbuf[256];
		aut_log_fatal("%s: can't malloc AUT_LINE, err_msg=%s",
			myname, acl_last_strerror(tbuf, sizeof(tbuf)));
	}

	snprintf(test_line->cmd_name, sizeof(test_line->cmd_name),
			"%s", cfg_line->value[0]);
	test_line->line_number = cfg_line->line_number;
	test_line->result      = atoi(cfg_line->value[1]);
	test_line->argc        = atoi(cfg_line->value[2]);

	if (cfg_line->ncount >= 4) {
		test_line->args_str = acl_mystrdup(cfg_line->value[3]);
		if (test_line->args_str == NULL) {
			char tbuf[256];
			aut_log_fatal("%s: cmd_name=%s, strdup for "
					"args_str, err_msg=%s",
					myname,
					cfg_line->value[0],
					acl_last_strerror(tbuf, sizeof(tbuf)));
		}
		test_line->argv = aut_parse_args_list(cfg_line->value[3]);
		if (test_line->argv == NULL)
			aut_log_fatal("%s: cmd_name=%s, aut_parse_args_list error",
					myname, cfg_line->value[0]);
	} else {
		test_line->args_str = NULL;
		test_line->argv = NULL;
	}

	if (acl_array_append(var_aut_line_array, (void *) test_line) < 0) {
		char tbuf[256];
		aut_log_fatal("%s: cmd_name=%s, acl_array_append error, err_msg=%s",
				myname, acl_last_strerror(tbuf, sizeof(tbuf)));
	}

	test_line->valid_line_idx = var_aut_valid_line_idx++;

	return (test_line);
}
