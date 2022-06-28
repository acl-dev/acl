#ifndef	__UNIT_TAB_TEST_INCLUDE_H__
#define	__UNIT_TAB_TEST_INCLUDE_H__

#include "lib_acl.h"
#ifdef	__cplusplus
extern "C" {
#endif

#include "unit_test.h"

static AUT_FN_ITEM __test_fn_tab[] = {
	/* 命令字名称		函数提示名		回调函数名称		回调参数 是否是内部命令 */
	/* in test_unit_fn.c */
	{ "test_param",		"fn_test_param",	test_unit_param,	NULL, 0 },
	{ "test_loop",		"fn_test_loop",		test_unit_loop,		NULL, 0 },

	{ NULL, NULL, NULL, NULL, 0},
};

#ifdef	__cplusplus
}
#endif

#endif
