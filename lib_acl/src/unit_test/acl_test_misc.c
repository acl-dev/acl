#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <string.h>
#include <stdio.h>
#include <string.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_msg.h"
#include "stdlib/acl_array.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_mystring.h"
#include "unit_test/acl_unit_test.h"

#endif

AUT_LINE *aut_line_new(const ACL_CFG_LINE *cfg_line)
{
	AUT_LINE *test_line = NULL;

	test_line = (AUT_LINE *) acl_mycalloc(1, sizeof(AUT_LINE));
	test_line->line_number = cfg_line->line_number;
	snprintf(test_line->cmd_name, sizeof(test_line->cmd_name),
		"%s", cfg_line->value[0]);
	test_line->obj_type  = AUT_OBJ_OUTER;

	return (test_line);
}

void aut_line_free(void *ctx)
{
	AUT_LINE *test_line = (AUT_LINE*) ctx;

	if (test_line->args_str)
		acl_myfree(test_line->args_str);
	if (test_line->argv)
		aut_free_args_list(test_line->argv);
	if (test_line->arg_inner && test_line->free_arg_inner)
		test_line->free_arg_inner(test_line->arg_inner);

	acl_myfree(test_line);
}

const ACL_ARRAY *aut_args_get(const char *cmd_name)
{
	const char *myname = "aut_args_get";
	AUT_LINE *test_line;
	int   i, n;

	if (var_aut_line_array == NULL)
		aut_log_fatal("%s: var_aut_line_array=NULL", myname);

	n = acl_array_size(var_aut_line_array);
	for (i = 0; i < n; i++) {
		test_line = (AUT_LINE *) acl_array_index(var_aut_line_array, i);
		if (test_line == NULL)
			break;
		if (strcasecmp(cmd_name, test_line->cmd_name) == 0)
			return (test_line->argv);
	}

	return (NULL);
}

int aut_size(void)
{
	const char *myname = "aust_size";
	int   n;

	if (var_aut_line_array == NULL)
		aut_log_fatal("%s: var_aut_line_array=NULL", myname);

	n = acl_array_size(var_aut_line_array);
	return (n);
}

AUT_LINE *aut_index(int idx)
{
	const char *myname = "aut_cfg_index";
	AUT_LINE *test_line;
	int   n;

	if (var_aut_line_array == NULL)
		aut_log_fatal("%s: var_aut_line_array=NULL", myname);

	n = acl_array_size(var_aut_line_array);

	if (idx < 0 || idx > n) {
		printf("%s: idx=%d, acl_array_size=%d, err_msg=invalid idx\n",
			myname, idx, n);
		return (NULL);
	}

	test_line = (AUT_LINE *) acl_array_index(var_aut_line_array, idx);
	return (test_line);
}

/* 比较所给的命令字与配置行中的命令字是否相等 */
int aut_line_cmdcmp(const AUT_LINE *test_line, const char *cmd_name)
{
	const char *myname = "aut_line_cmdcmp";

	if (test_line == NULL || cmd_name == NULL) {
		printf("%s: input error\n", myname);
		return (-1);
	}

	return (strcasecmp(test_line->cmd_name, cmd_name));
}

/* 比较程序执行结果与配置行中的期望结果值 */
int aut_line_resultcmp(const AUT_LINE *test_line, int value)
{
	const char *myname = "aut_line_resultcmp";

	if (test_line == NULL) {
		printf("%s: input error\n", myname);
		return (-1);
	}

	if (test_line->result == value)
		return (0);
	return (1);
}

/* 取得该配置行在配置文件中的行号 */
int aut_line_number(const AUT_LINE *test_line)
{
	const char *myname = "aut_line_number";

	if (test_line == NULL) {
		printf("%s: input error\n", myname);
		return (-1);
	}
	return (test_line->line_number);
}

int aut_line_valid_linenum(const AUT_LINE *test_line)
{
	if (test_line == NULL)
		return (-1);
	return (test_line->valid_line_idx);
}

/* 取得该配置行的命令字 */
const char *aut_line_cmdname(const AUT_LINE *test_line)
{
	const char *myname = "aut_line_cmdname";

	if (test_line == NULL) {
		printf("%s: input error\n", myname);
		return (NULL);
	}

	return (test_line->cmd_name);
}

/* 取得该配置行中配置参数的个数 */
int aut_line_argc(const AUT_LINE *test_line)
{
	const char *myname = "aut_line_argc";

	if (test_line == NULL) {
		printf("%s: input error\n", myname);
		return (-1);
	}

	return (test_line->argc);
}

const char *aut_line_getvalue(const AUT_LINE *test_line, const char *name)
{
	const char *myname = "aut_line_getvalue";
	AUT_ARG_ITEM *arg = NULL;
	int   i, n;

	if (test_line == NULL || name == NULL || *name == 0) {
		printf("%s: input error\n", myname);
		return (NULL);
	}

	n = acl_array_size(test_line->argv);

	for (i = 0; i < n; i++) {
		arg = (AUT_ARG_ITEM *) acl_array_index(test_line->argv, i);
		if (arg == NULL || arg->name == NULL)
			break;
		if (strcasecmp(name, arg->name) == 0)
			return (arg->value);
	}

	return (NULL);
}

/* 取得该配置行中参数内容  */
const char *aut_line_argstr(const AUT_LINE *test_line)
{
	const char *myname = "aut_line_argstr";

	if (test_line == NULL) {
		printf("%s: input error\n", myname);
		return (NULL);
	}

	return (test_line->args_str);
}

/* 取得该配置行中的期望结果值 */
int aut_line_result(const AUT_LINE *test_line)
{
	const char *myname = "aut_line_result";

	if (test_line == NULL) {
		printf("%s: input error\n", myname);
		return (-1);
	}

	return (test_line->result);
}

/* 判断是否停止继续执行的配置行 */
int aut_line_stop(const AUT_LINE *test_line)
{
	const char *myname = "aut_line_stop";

	if (test_line == NULL)
		aut_log_fatal("%s: input error", myname);

	if (strcasecmp(test_line->cmd_name, VAR_AUT_STOP) == 0)
		return (1);
	return (0);
}

/* 判断是否是保留配置行 */
int aut_line_reserved(AUT_LINE *test_line)
{
	const char *myname = "aut_line_reserved";

	if (test_line == NULL)
		aut_log_fatal("%s: input error", myname);

	if (strcasecmp(test_line->cmd_name, VAR_AUT_LOG) == 0)
		return (1);
	return (0);
}

/* 将用户自己的参数项存储在 test_line 内的 arg_inner 中 */
int aut_line_add_arg(AUT_LINE *test_line, void *arg)
{
	const char *myname = "aut_line_add_arg";

	if (test_line == NULL)
		aut_log_fatal("%s: input error", myname);

	test_line->arg_outer = arg;
	return (0);
}

void aut_line_del_arg(AUT_LINE *test_line, void (*free_fn) (void *))
{
	if (test_line == NULL)
		return;
	if (free_fn != NULL)
		free_fn(test_line->arg_outer);
	test_line->arg_outer = NULL;
}

void *aut_line_get_arg(const AUT_LINE *test_line)
{
	if (test_line == NULL)
		return (NULL);
	return (test_line->arg_outer);
}

int aut_end_linenum(int start_linenum)
{
	const char *myname = "aut_end_linenum";
	const AUT_LINE *test_line_start;
	const AUT_LINE *test_line_end = NULL;

	if (var_aut_line_array == NULL) {
		printf("%s: var_aut_line_array=NULL\n", myname);
		return (-1);
	}

	test_line_start = aut_index(start_linenum);
	if (test_line_start == NULL)
		return (-1);

	if (strcasecmp(test_line_start->cmd_name, VAR_AUT_LOOP_BEGIN) == 0)
		test_line_end = aut_loop_end(test_line_start);

	if (test_line_end == NULL)
		return (-1);

	return (aut_line_number(test_line_end));
}

const AUT_LINE *aut_lookup_from_line(const AUT_LINE *test_line, int flag)
{
	AUT_CMD_TOKEN *token;
	const AUT_LINE *dst_line;
	int   i, n;

	if (test_line == NULL)
		return (NULL);

	n = aut_size();
	for (i = aut_line_valid_linenum(test_line) + 1; i < n; i++) {
		dst_line = aut_index(i);
		if (dst_line == NULL)
			break;
		if (dst_line->arg_inner == NULL)
			continue;
		token = (AUT_CMD_TOKEN *) dst_line->arg_inner;
		if (token->flag == flag)
			return (dst_line);
	}

	return (NULL);
}

