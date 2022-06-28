#ifndef	__TEST_MAIN_INCLUDE_H__
#define	__TEST_MAIN_INCLUDE_H__

#include "lib_acl.h"

#include "stdlib/test_stdlib.h"
#include "unit_test/unit_test.h"
#include "net/test_net.h"

typedef struct {
	const char *label;
	void (*register_fn)(void);
} TEST_ENTRY;

static TEST_ENTRY __test_entry_tab[] = {
	{ "test_stdlib",	test_stdlib_register	},
	{ "test_unit",		test_unit_register	},
	{ "test_net",		test_net_register	},
	{ NULL, NULL },
};

#endif

