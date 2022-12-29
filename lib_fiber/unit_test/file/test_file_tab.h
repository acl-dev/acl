#pragma once

static AUT_FN_ITEM __test_fn_tab[] = {
	/* 命令字名称		函数提示名		回调函数名称		回调参数	是否是内部命令 */

	/* In file_read.cpp */
	{ "file_load",		"file_load",		file_load,		NULL,		0		},

	{ NULL, NULL, NULL, NULL, 0 },
};
