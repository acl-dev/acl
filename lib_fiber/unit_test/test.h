#pragma once

#include "tbox/test_tbox.h"
#include "file/test_file.h"
#include "io/test_io.h"

typedef struct {
	const char *label;
	void (*register_fn)(void);
} TEST_ENTRY;

static TEST_ENTRY __test_entry_tab[] = {
	{ "test_tbox",		tbox_register		},
	{ "test_file",		file_register		},
	{ "test_io",		io_register		},

	{ NULL,			NULL			},
};
