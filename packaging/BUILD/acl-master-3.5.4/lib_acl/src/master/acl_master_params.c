#include "StdAfx.h"

#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_xinetd_cfg.h"
#include "stdlib/acl_mystring.h"

#endif

#include "master/acl_master_conf.h"

#ifndef ACL_CLIENT_ONLY

/*-------------- global static inition and update functions ----------------*/

static void init_conf_int_vars(ACL_CONFIG_INT_TABLE cit[])
{
	int   i;

	for (i = 0; cit[i].name != 0; i++)
		*(cit[i].target) = cit[i].defval;
}

static void init_conf_int64_vars(ACL_CONFIG_INT64_TABLE cit[])
{
	int   i;

	for (i = 0; cit[i].name != 0; i++)
		*(cit[i].target) = cit[i].defval;
}

static void init_conf_str_vars(ACL_CONFIG_STR_TABLE cst[])
{
	int   i;

	for (i = 0; cst[i].name != 0; i++)
		*(cst[i].target) = acl_mystrdup(cst[i].defval);
}

static void init_conf_bool_vars(ACL_CONFIG_BOOL_TABLE cbt[])
{
	int   i;

	for (i = 0; cbt[i].name != 0; i++) {
		if (cbt[i].defval == 0)
			*(cbt[i].target) = 0;
		else
			*(cbt[i].target) = 1;
	}
}

static void update_conf_int_vars(ACL_CONFIG_INT_TABLE cit[],
	const char *name, const char *value)
{
	int   i;

	for (i = 0; cit[i].name != 0; i++) {
		if (strcasecmp(cit[i].name, name) == 0)
			*(cit[i].target) = atoi(value);
	}
}

static void update_conf_int64_vars(ACL_CONFIG_INT64_TABLE cit[],
	const char *name, const char *value)
{
	int   i;

	for (i = 0; cit[i].name != 0; i++) {
		if (strcasecmp(cit[i].name, name) == 0)
			*(cit[i].target) = acl_atoi64(value);
	}
}

static void update_conf_str_vars(ACL_CONFIG_STR_TABLE cst[],
	const char *name, const char *value)
{
	int   i;

	for (i = 0; cst[i].name != 0; i++) {
		if (strcasecmp(cst[i].name, name) == 0) {
			acl_myfree(*(cst[i].target));
			*(cst[i].target) = acl_mystrdup(value);
		}
	}
}

static void update_conf_bool_vars(ACL_CONFIG_BOOL_TABLE cbt[],
	const char *name, const char *value)
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

/* below configure operations will be called by the app server */

static ACL_XINETD_CFG_PARSER *__app_cfg = NULL;
static char *__app_conf_file = NULL;

void acl_app_conf_load(const char *pathname)
{
	const char *myname = "acl_app_conf_load";

	if (pathname == NULL || *pathname == 0)
		acl_msg_fatal("%s(%d), %s: input error",
			__FILE__, __LINE__, myname);

	if (__app_cfg != NULL)
		acl_xinetd_cfg_free(__app_cfg);

	__app_cfg = acl_xinetd_cfg_load(pathname);

	if (__app_cfg == NULL)
		acl_msg_fatal("%s(%d), %s: load file(%s) error(%s)",
			__FILE__, __LINE__, myname, pathname, strerror(errno));

	__app_conf_file = acl_mystrdup(pathname);
}

void acl_app_conf_unload(void)
{
	if (__app_cfg) {
		acl_xinetd_cfg_free(__app_cfg);
		__app_cfg = NULL;
	}

	if (__app_conf_file) {
		acl_myfree(__app_conf_file);
		__app_conf_file = NULL;
	}
}

void  acl_get_app_conf_int_table(ACL_CONFIG_INT_TABLE *table)
{
	int   i, n, ret;
	char *name, *value;

	if (table == NULL)
		return;

	init_conf_int_vars(table);

	if (__app_cfg == NULL)
		return;

	n = acl_xinetd_cfg_size(__app_cfg);

	for (i = 0; i < n; i++) {
		ret = acl_xinetd_cfg_index(__app_cfg, i, &name, &value);
		if (ret == 0)
			update_conf_int_vars(table, name, value);
	}
}

void  acl_get_app_conf_int64_table(ACL_CONFIG_INT64_TABLE *table)
{
	int   i, n, ret;
	char *name, *value;

	if (table == NULL)
		return;

	init_conf_int64_vars(table);

	if (__app_cfg == NULL)
		return;

	n = acl_xinetd_cfg_size(__app_cfg);

	for (i = 0; i < n; i++) {
		ret = acl_xinetd_cfg_index(__app_cfg, i, &name, &value);
		if (ret == 0)
			update_conf_int64_vars(table, name, value);
	}
}

void  acl_get_app_conf_str_table(ACL_CONFIG_STR_TABLE *table)
{
	int   i, n, ret;
	char *name, *value;

	if (table == NULL)
		return;

	init_conf_str_vars(table);

	if (__app_cfg == NULL)
		return;

	n = acl_xinetd_cfg_size(__app_cfg);

	for (i = 0; i < n; i++) {
		ret = acl_xinetd_cfg_index(__app_cfg, i, &name, &value);
		if (ret == 0)
			update_conf_str_vars(table, name, value);
	}
}

void  acl_free_app_conf_str_table(ACL_CONFIG_STR_TABLE *table)
{
	int   i;

	if (table == NULL)
		return;

	for (i = 0; table[i].name != 0; i++) {
		if (*(table[i].target) != 0)
			acl_myfree(*(table[i].target));
	}
}

void  acl_get_app_conf_bool_table(ACL_CONFIG_BOOL_TABLE *table)
{
	int   i, n, ret;
	char *name, *value;

	if (table == NULL)
		return;

	init_conf_bool_vars(table);

	if (__app_cfg == NULL)
		return;

	n = acl_xinetd_cfg_size(__app_cfg);

	for (i = 0; i < n; i++) {
		ret = acl_xinetd_cfg_index(__app_cfg, i, &name, &value);
		if (ret == 0)
			update_conf_bool_vars(table, name, value);
	}
}

#endif /* ACL_CLIENT_ONLY */
