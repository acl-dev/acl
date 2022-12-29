#pragma once

static AUT_FN_ITEM __test_fn_tab[] = {
	/* 命令字名称		函数提示名		回调函数名称		回调参数	是否是内部命令 */

	/* In tbox_wait.cpp */
	{ "tbox_thread_wait",	"tbox_thread_wait",	tbox_thread_wait,	NULL,		0		},
	{ "tbox_fiber_wait",	"tbox_fiber_wait",	tbox_fiber_wait,	NULL,		0		},

	/* In tbox_fiber.cpp */
	{ "tbox_fiber_consume",	"tbox_fiber_consume",	tbox_fiber_consume,	NULL,		0		},

	/* In tbox_mixed.cpp */
	{ "tbox_mixed_consume",	"tbox_mixed_consume",	tbox_mixed_consume,	NULL,		0		},

	{ NULL, NULL, NULL, NULL, 0 },
};
