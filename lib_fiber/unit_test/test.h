#pragma once

#include "tbox/test_tbox.h"

typedef struct {
	const char *label;
	void (*register_fn)(void);
} TEST_ENTRY;

static TEST_ENTRY __test_entry_tab[] = {
	{ "test_tbox",		tbox_register		},

	{ NULL,			NULL			},
};
