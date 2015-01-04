#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef ACL_UNIX
#include <unistd.h>
#endif
#include <string.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

/* Util */
#include "stdlib/acl_msg.h"
#include "stdlib/acl_sys_patch.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_array.h"

#include "unit_test/acl_unit_test.h"

#endif

int var_aut_verbose = 0;

static ACL_ARRAY *__all_callback_fn = NULL;

/* forward declare */
static int __fn_callback(int from, int to, int *stop);

/*--------------------------------------------------------------------------*/
static int __test_stop(AUT_LINE *test_line, void *arg acl_unused)
{
	const char *myname = "__test_stop";

	aut_log_info("%s(%d): Stop here, line=%d, cmd=%s",
			myname, __LINE__,
			aut_line_number(test_line),
			aut_line_cmdname(test_line));

	return (0);
}

/* 内部睡眠命令函数 */
static int __test_sleep(AUT_LINE *test_line, void *arg acl_unused)
{
	const char *myname = "__test_sleep";
	const char *ptr;
	int   sleep_second;

	ptr = aut_line_getvalue(test_line, VAR_AUT_ITEM_COUNT);
	if (ptr == NULL) {
		aut_log_error("%s: can't get name=%s' value",
				myname, VAR_AUT_ITEM_COUNT);
		return (-1);
	}

	sleep_second = atoi(ptr);
	aut_log_info("%s: sleep %s seconds now!", myname, sleep_second);
	if (sleep_second > 0)
		sleep(sleep_second);

	return (0);
}

/* 内部暂停函数 */
static int __test_pause(AUT_LINE *test_line acl_unused, void *arg acl_unused)
{
	const char *myname = "__test_pause";
	int   i = 0;

	aut_log_info("%s: PAUSE, exit when type Ctrl + c", myname);
	while (1) {
		sleep(1000);
		if (++i >= 1000000)
			break;
	}

	return (0);
}

static int __test_loop_begin(AUT_LINE *test_line, void *arg acl_unused)
{
	const char *myname = "__test_loop_begin";
	int   count, i, from, to, ret, stop;

	count = aut_loop_count(test_line);
	if (count < 0) {
		aut_log_error("%s(%d): no loop count", myname, __LINE__);
		return (-1);
	}

	from = aut_loop_from(test_line);
	to   = aut_loop_to(test_line);

	if (from > to) {
		aut_log_error("%s(%d): from(%d) > to(%d)",
				myname, __LINE__, from, to);
		return (-1);
	}
	if (from == to)
		return (1);

	for (i = 0; i < count; i++) {
		stop = 0;
		ret = __fn_callback(from, to, &stop);
		if (ret < 0)
			return (-1);
		if (stop)
			break;
	}

	return (to - from + 1);
}

static int __test_loop_break(AUT_LINE *test_line, void *arg acl_unused)
{
	const char *myname = "__test_loop_break";
	const AUT_LINE *loop_end_line;
	int   num_break, num_end; 

	loop_end_line = aut_lookup_from_line(test_line, AUT_FLAG_LOOP_END);
	if (loop_end_line == NULL) {
		aut_log_error("%s(%d): failed, no loop end, line=%d, cmd=%d",
				myname, __LINE__,
				aut_line_number(test_line),
				aut_line_cmdname(test_line));
		return (-1);
	}

	num_break = aut_line_valid_linenum(test_line);
	num_end   = aut_line_valid_linenum(loop_end_line);
	if (num_break >= num_end) {
		aut_log_error("%s(%d): failed, num_break(%d) >= num_end(%d), "
				"line=%d, cmd=%s",
				myname, __LINE__,
				num_break, num_end,
				aut_line_number(test_line),
				aut_line_cmdname(test_line));
		return (-1);
	}

	return (num_end - num_break + 1);
}

static int __test_loop_end(AUT_LINE *test_line acl_unused, void *arg acl_unused)
{
	return (1);
}

/* not complete yet */
static int __test_if(AUT_LINE *test_line acl_unused, void *arg acl_unused)
{
	return (1);
}

/* not complete yet */
static int __test_else(AUT_LINE *test_line acl_unused, void *arg acl_unused)
{
	return (1);
}

/* not complete yet */
static int __test_endif(AUT_LINE *test_line acl_unused, void *arg acl_unused)
{
	return (1);
}

/* not complete yet */
static int __test_goto(AUT_LINE *test_line acl_unused, void *arg acl_unused)
{
	return (1);
}

/* 内部用动作命令表 */
static AUT_FN_ITEM __inner_fn_tab[] = {
	/* 用来停止执行下一步测试操作 */
	{ VAR_AUT_STOP, "__test_stop", __test_stop, NULL, 1 },
	{ VAR_AUT_SLEEP, "__test_sleep", __test_sleep, NULL, 1 },
	{ VAR_AUT_PAUSE, "__test_pause", __test_pause, NULL, 1 },

	{ VAR_AUT_LOOP_BEGIN, "__test_loop_begin", __test_loop_begin, NULL, 1 },
	{ VAR_AUT_LOOP_BREAK, "__test_loop_break", __test_loop_break, NULL, 1 },
	{ VAR_AUT_LOOP_END, "__test_loop_end", __test_loop_end, NULL, 1 },

	{ VAR_AUT_IF, "__test_if", __test_if, NULL, 1 },
	{ VAR_AUT_ELSE, "__test_else", __test_else, NULL, 1 },
	{ VAR_AUT_ENDIF, "__testt_endif", __test_endif, NULL, 1 },

	{ VAR_AUT_GOTO, "__test_goto", __test_goto, NULL, 1 },

	{ NULL, NULL, NULL, NULL, 0 },
};
/*--------------------------------------------------------------------------*/

static AUT_FN_ITEM *__lookup_fn_item(const AUT_LINE *test_line)
{
	const char *myname = "__lookup_fn_item";
	int   i, n;
	AUT_FN_ITEM *item = NULL, *tmp;

	n = acl_array_size(__all_callback_fn);
	for (i = 0; i < n; i++) {
		tmp = (AUT_FN_ITEM *) acl_array_index(__all_callback_fn, i);
		if (tmp == NULL)
			aut_log_fatal("%s(%d): idx=%d, null rebuild from array",
					myname, __LINE__, i);
		if (aut_line_cmdcmp(test_line, tmp->cmd_name) == 0) {
			item = tmp;
			break;
		}
	}

	return (item);
}

static int __fn_callback(int from, int to, int *stop)
{
	const char *myname = "__fn_callback";
	AUT_FN_ITEM *item;
	AUT_LINE *test_line;
	int   ret;

	if (from > to)
		aut_log_fatal("%s(%d): from(%d) > to(%d)",
				myname, __LINE__, from, to);

	for (; from < to;) {
		test_line = aut_index(from);
		if (test_line == NULL)
			break;
		item = __lookup_fn_item(test_line);
		if (item == NULL) {
			aut_log_error("%s(%d): invalid cmd(%s), line=%d",
					myname, __LINE__,
					aut_line_cmdname(test_line),
					aut_line_number(test_line));
			return (-1);
		}

		if (item->inner) {
			ret = item->fn_callback(test_line, item->arg);
			if (ret < 0) {
#ifdef	AUT_DEBUG
				aut_log_error("%s(%d): failed, line=%d, cmd=%s",
						myname, __LINE__,
						aut_line_number(test_line),
						item->cmd_name);
#endif
				return (-1);
			}

			from += ret;
			if (from > to) {
				if (stop)
					*stop = 1;
				break;
			}

			if (item->fn_callback == __test_stop)
				break;
			continue;
		}
		
		ret = item->fn_callback(test_line, item->arg);
		if (aut_line_resultcmp(test_line, ret) != 0) {
			aut_log_error("%s(%d): failed, line=%d, cmd=%s",
					myname, __LINE__,
					aut_line_number(test_line),
					item->cmd_name);
			if (stop)
				*stop = 1;
			return (-1);
		}
		aut_log_info("%s(%d): success, function=%s, cmd=%s",
				myname, __LINE__,
				item->fn_name, item->cmd_name);
		from++;
	}

	return (0);
}

int aut_start(void)
{
	const char *myname = "aut_start";
	int   max;

	if (__all_callback_fn == NULL) {
		printf("%s: please call aut_register first\n", myname);
		return (-1);
	}

	/* 取得所有有效配置行的总和 */
	max = aut_size();
	if (max <= 0) {
		aut_log_error("%s(%d): configure's size is null",
				myname, __LINE__);
		return (-1);
	}

	return (__fn_callback(0, max, NULL));
}

static void free_fn_item(void *arg)
{
	AUT_FN_ITEM *item = (AUT_FN_ITEM*) arg;

	acl_myfree(item);
}

void aut_stop(void)
{
	if (var_aut_line_array) {
		acl_array_free(var_aut_line_array, aut_line_free);
		var_aut_line_array = NULL;
	}
	if (__all_callback_fn) {
		acl_array_free(__all_callback_fn, free_fn_item);
		__all_callback_fn = NULL;
	}
}

static void __add_fn_item(ACL_ARRAY *fn_tab, const AUT_FN_ITEM *fn_item, int inner)
{
	const char *myname = "__add_fn_item";
	AUT_FN_ITEM *item;

	item = (AUT_FN_ITEM *) acl_mycalloc(1, sizeof(AUT_FN_ITEM));
	item->cmd_name    = fn_item->cmd_name;
	item->fn_name     = fn_item->fn_name;
	item->fn_callback = fn_item->fn_callback;
	item->arg         = fn_item->arg;
	item->inner       = inner;

	if (acl_array_append(fn_tab, item) < 0) {
		char tbuf[256];
		aut_log_fatal("%s(%d): array_append error(%s)",
			myname, __LINE__, acl_last_strerror(tbuf, sizeof(tbuf)));
	}
}

void aut_register(const AUT_FN_ITEM test_fn_tab[])
{
	int   i;

	if (__all_callback_fn == NULL)
		__all_callback_fn = acl_array_create(10);

	/* 先增加注册内部用命令 */
	for (i = 0; __inner_fn_tab[i].cmd_name != NULL; i++)
		__add_fn_item(__all_callback_fn, &__inner_fn_tab[i], 1);

	/* 再注册外部传来的命令任务 */
	for (i = 0; test_fn_tab[i].cmd_name != NULL; i++)
		__add_fn_item(__all_callback_fn, &test_fn_tab[i], 0);
}
