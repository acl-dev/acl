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

static int __if_nested_count = 0;
static int __if_count        = 0;
/* static int _else_count = 0; */
/* static int _elif_count = 0; */
static int __endif_count     = 0;

/*----------------------------------------------------------------------------*/
static AUT_LINE *__mkcmd_if(const ACL_CFG_LINE *line)
{
	AUT_LINE *test_line;
	AUT_CMD_TOKEN *if_token = NULL;

	__if_nested_count++;

	if_token = (AUT_CMD_TOKEN *) acl_mycalloc(1, sizeof(*if_token));
	if_token->flag         = AUT_FLAG_IF;
	if_token->match_number = ++__if_count;
	if_token->peer         = NULL;

	test_line = aut_line_new(line);

	test_line->arg_inner = (void *) if_token;
	test_line->free_arg_inner = acl_myfree_fn;

	return (test_line);
}

static AUT_LINE *__mkcmd_endif(const ACL_CFG_LINE *line)
{
	const char *myname = "__mkcmd_endif";
	AUT_LINE *test_line, *test_line_peer;
	AUT_CMD_TOKEN *if_token = NULL, *if_token_peer;
	int   n, i;

	__if_nested_count--;
	if_token = (AUT_CMD_TOKEN *) acl_mycalloc(1, sizeof(*if_token));
	if_token->flag         = AUT_FLAG_ENDIF;
	if_token->match_number = ++__endif_count;
	if_token->peer         = NULL;

	test_line = aut_line_new(line);

	test_line->arg_inner = (void *) if_token;
	test_line->free_arg_inner = acl_myfree_fn;

	n = acl_array_size(var_aut_line_array);
	for (i = 0; i < n; i++) {
		test_line_peer = (AUT_LINE *) acl_array_index(var_aut_line_array, i);
		if (test_line_peer->arg_inner == NULL)
			continue;
		if_token_peer = (AUT_CMD_TOKEN *) test_line_peer->arg_inner;
		if (if_token_peer->flag != AUT_FLAG_IF)
			continue;

		if (if_token_peer->match_number != if_token->match_number)
			continue;

		/* 找到匹配的循环开始对等结点 */
		if_token_peer->peer = test_line;
		if_token->peer      = test_line_peer;
	}

	if (if_token->peer == NULL) {
		aut_log_fatal("%s: line_number=%d, cmd=%s, "
				"if_nested=%d, if_count=%d, "
				"endif_count=%d, please check configure, "
				"err_msg=not found peer loop begin",
				myname, test_line->line_number,
				test_line->cmd_name, __if_nested_count,
				__if_count, __endif_count);
	}

	return (test_line);
}

static AUT_LINE *__mkcmd_stop(const ACL_CFG_LINE *line)
{
	AUT_LINE *test_line;
	AUT_CMD_TOKEN *inner_token = NULL;

	test_line = aut_line_new(line);

	snprintf(test_line->cmd_name, sizeof(test_line->cmd_name),
			"%s", line->value[0]);
	inner_token = (AUT_CMD_TOKEN *) acl_mycalloc(1, sizeof(*inner_token));
	inner_token->flag = AUT_FLAG_STOP;

	test_line->arg_inner = (void *) inner_token;
	test_line->free_arg_inner = acl_myfree_fn;

	return (test_line);
}

/*---------------------------- 分析与内部命令相匹配的配置行 ---------------- */
/* 内部保留的命令项数据结构定义 */
typedef struct {
	const char *cmd_name;
	AUT_LINE *(*match_fn)(const ACL_CFG_LINE *line);
} __MATCH_CMD;

/* 内部保留的命令对象表 */
static __MATCH_CMD __inner_cmd_tab[] = {
	{ VAR_AUT_LOOP_BEGIN, aut_loop_make_begin },
	{ VAR_AUT_LOOP_BREAK, aut_loop_make_break },
	{ VAR_AUT_LOOP_END, aut_loop_make_end },
	{ VAR_AUT_IF, __mkcmd_if },
	{ VAR_AUT_ENDIF, __mkcmd_endif },
	{ VAR_AUT_STOP, __mkcmd_stop },
	{ NULL, NULL },
};

/* 从内部命令表中取得相应的对象 */

AUT_LINE *aut_add_inner_cmd(const ACL_CFG_LINE *line)
{
	const char *myname = "aut_add_inner_cmd";
	AUT_LINE *test_line = NULL;
	__MATCH_CMD *pmatch_cmd;
	AUT_CMD_TOKEN *inner_token;
	int   i;

	if (line->ncount < 1) {
		aut_log_error("%s: ncount=%d", myname, line->ncount);
		return (NULL);
	}

	for (i = 0; __inner_cmd_tab[i].cmd_name != NULL; i++) {
		pmatch_cmd = &__inner_cmd_tab[i];
		if (strcasecmp(line->value[0], pmatch_cmd->cmd_name) == 0) {
			/* 由内部命令保留函数动态分配一个相对应的
			 * AUT_LINE 对象
			 */
			test_line = pmatch_cmd->match_fn(line);
			break;
		}
	}

	if (test_line == NULL)
		return (NULL);

	if (acl_array_append(var_aut_line_array, (void *) test_line) < 0) {
		char  tbuf[256];
		aut_log_fatal("%s: cmd_name=%s, "
			"acl_array_append error, err_msg=%s",
			myname, test_line->cmd_name, acl_last_strerror(tbuf, sizeof(tbuf)));
	}

	/* 设置有效行号 */
	inner_token = (AUT_CMD_TOKEN *) test_line->arg_inner;
	if (inner_token == NULL)
		return (test_line);

	test_line->valid_line_idx = var_aut_valid_line_idx++;

	/* 调整有效行号 */
	inner_token->valid_line_idx = test_line->valid_line_idx;

	/* 只对循环命令起作用, 设置相对命令位移 */
	inner_token->offset_valid_line_idx = inner_token->valid_line_idx;

	return (test_line);
}
