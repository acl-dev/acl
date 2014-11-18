#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "stdlib/acl_msg.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_mystring.h"
#include "stdlib/acl_xinetd_cfg.h"

#endif

static void __init_conf_int_vars(ACL_CFG_INT_TABLE cit[])
{
	int   i;

	for (i = 0; cit[i].name != 0; i++)
		*(cit[i].target) = cit[i].defval;
}

static void __init_conf_int64_vars(ACL_CFG_INT64_TABLE cit[])
{
	int   i;

	for (i = 0; cit[i].name != 0; i++)
		*(cit[i].target) = cit[i].defval;
}

static void __init_conf_str_vars(ACL_CFG_STR_TABLE cst[])
{
	int   i;

	for (i = 0; cst[i].name != 0; i++)
		*(cst[i].target) = acl_mystrdup(cst[i].defval);
}

static void __init_conf_bool_vars(ACL_CFG_BOOL_TABLE cbt[])
{
	int   i;

	for (i = 0; cbt[i].name != 0; i++) {
		if (cbt[i].defval == 0)
			*(cbt[i].target) = 0;
		else
			*(cbt[i].target) = 1;
	}
}

static void __update_conf_int_vars(ACL_CFG_INT_TABLE cit[],
					const char *name,
					const char *value)
{
	int   i;

	for (i = 0; cit[i].name != 0; i++) {
		if (strcasecmp(cit[i].name, name) == 0)
			*(cit[i].target) = atoi(value);
	}
}

static void __update_conf_int64_vars(ACL_CFG_INT64_TABLE cit[],
					const char *name,
					const char *value)
{
	int   i;

	for (i = 0; cit[i].name != 0; i++) {
		if (strcasecmp(cit[i].name, name) == 0)
			*(cit[i].target) = acl_atoi64(value);
	}
}

static void __update_conf_str_vars(ACL_CFG_STR_TABLE cst[],
					const char *name,
					const char *value)
{
	int   i;

	for (i = 0; cst[i].name != 0; i++) {
		if (strcasecmp(cst[i].name, name) == 0) {
			acl_myfree(*(cst[i].target));
			*(cst[i].target) = acl_mystrdup(value);
		}
	}
}

static void __update_conf_bool_vars(ACL_CFG_BOOL_TABLE cbt[],
					const char *name,
					const char *value)
{
	int   i, n;

	for (i = 0; cbt[i].name != 0; i++) {
		if (strcasecmp(cbt[i].name, name) == 0) {
			n = atoi(value);
			if (n != 0)
				*(cbt[i].target) = 1;
			else
				*(cbt[i].target) = 0;
		}
	}
}

/*--------------------------------------------------------------------------*/

void acl_xinetd_params_int_table(ACL_XINETD_CFG_PARSER *cfg,
	ACL_CFG_INT_TABLE *table)
{
	int   i, n, ret;
	char *name, *value;

	if (table == NULL)
		return;

	__init_conf_int_vars(table);

	if (cfg == NULL)
		return;

	n = acl_xinetd_cfg_size(cfg);

	for (i = 0; i < n; i++) {
		ret = acl_xinetd_cfg_index(cfg, i, &name, &value);
		if (ret == 0)
			__update_conf_int_vars(table, name, value);
	}
}

void acl_xinetd_params_int64_table(ACL_XINETD_CFG_PARSER *cfg,
	ACL_CFG_INT64_TABLE *table)
{
	int   i, n, ret;
	char *name, *value;

	if (table == NULL)
		return;

	__init_conf_int64_vars(table);

	if (cfg == NULL)
		return;

	n = acl_xinetd_cfg_size(cfg);

	for (i = 0; i < n; i++) {
		ret = acl_xinetd_cfg_index(cfg, i, &name, &value);
		if (ret == 0)
			__update_conf_int64_vars(table, name, value);
	}
}

void  acl_xinetd_params_str_table(ACL_XINETD_CFG_PARSER *cfg,
	ACL_CFG_STR_TABLE *table)
{
	int   i, n, ret;
	char *name, *value;

	if (table == NULL)
		return;

	__init_conf_str_vars(table);

	if (cfg == NULL)
		return;

	n = acl_xinetd_cfg_size(cfg);

	for (i = 0; i < n; i++) {
		ret = acl_xinetd_cfg_index(cfg, i, &name, &value);
		if (ret == 0) {
			__update_conf_str_vars(table, name, value);
		}
	}
}

void  acl_xinetd_params_bool_table(ACL_XINETD_CFG_PARSER *cfg,
	ACL_CFG_BOOL_TABLE *table)
{
	int   i, n, ret;
	char *name, *value;

	if (table == NULL)
		return;

	__init_conf_bool_vars(table);

	if (cfg == NULL)
		return;

	n = acl_xinetd_cfg_size(cfg);

	for (i = 0; i < n; i++) {
		ret = acl_xinetd_cfg_index(cfg, i, &name, &value);
		if (ret == 0)
			__update_conf_bool_vars(table, name, value);
	}
}
