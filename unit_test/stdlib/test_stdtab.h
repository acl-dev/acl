#ifndef	__TEST_STDTAB_INCLUDE_H__
#define	__TEST_STDTAB_INCLUDE_H__

#include "lib_acl.h"
#ifdef	__cplusplus
extern "C" {
#endif

#include "test_stdlib.h"

static AUT_FN_ITEM __test_fn_tab[] = {
	/* 命令字名称		函数提示名		回调函数名称		回调参数 是否是内部命令 */
	/* test_mylock.c */
	{ "test_mylock_unlock",	"test_mylock_unlock",	test_mylock_unlock,	NULL, 0 },
	{ "test_mylock_excl",	"test_mylock_excl",	test_mylock_excl,	NULL, 0 },
	{ "test_mylock_nowait",	"test_mylock_nowait",	test_mylock_nowait,	NULL, 0 },
	{ "test_mylock_shared",	"test_mylock_shared",	test_mylock_shared,	NULL, 0 },

	/* test_ring.c */
	{ "test_ring",		"test_ring",		test_ring,		NULL, 0	},

	/* test_htable.c */
	{ "htable_create",      "htable_create",        test_htable_create,     NULL, 0 },
	{ "htable_find",	"htable_find",		test_htable_find,	NULL, 0 },
	{ "htable_stat",	"htable_stat",		test_htable_stat,	NULL, 0 },
	{ "htable_free",	"htable_free",		test_htable_free,	NULL, 0 },
	{ "htable_rwlock",	"htable_rwlock",	test_htable_rwlock,	NULL, 0 },

	/* test_malloc.c */
	{ "fatal_malloc",	"fatal_malloc",		test_fatal_malloc,	NULL, 0 },
	{ "fatal_free",		"fatal_free",		test_fatal_free,	NULL, 0 },

	/* test_avl.c */
	{ "avl_create",		"avl_create",		test_avl_create,	NULL, 0 },
	{ "avl_add_bat",	"avl_add_bat",		test_avl_add_bat,	NULL, 0 },
	{ "avl_walk",		"avl_walk",		test_avl_walk,		NULL, 0 },
	{ "avl_find",		"avl_find",		test_avl_find,		NULL, 0 },
	{ "avl_add",		"avl_add",		test_avl_add,		NULL, 0 },
	{ "avl_delete",		"avl_delete",		test_avl_delete,	NULL, 0 },

	/* test_stack.c */
	{ "stack_bat_add",	"stack_bat_add",	test_stack_bat_add,	NULL, 0 },
	{ "stack_walk",		"stack_walk",		test_stack_walk,	NULL, 0 },
	{ "stack_pop",		"stack_pop",		test_stack_pop,	NULL, 0 },

	/* test_vstream.c */
	{ "file_vstream",	"file_vstream",		test_file_vstream,	NULL, 0 },

	/* test_string.c */
	{ "strrncasecmp",	"strrncasecmp",		test_strrncasecmp,	NULL, 0 },
	{ "strrncmp",		"strrncmp",		test_strrncmp,		NULL, 0 },
	{ "i64toa",		"i64toa",		test_x64toa,		NULL, 0 },
	{ "strcasestr",		"strcasestr",		test_strcasestr,	NULL, 0 },

	{ NULL, NULL, NULL, NULL, 0},
};

#ifdef	__cplusplus
}
#endif

#endif

