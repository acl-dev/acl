
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "lib_acl.h"
#include "test_stdlib.h"

#include "test_stdtab.h"

void test_stdlib_register()
{
	aut_register(__test_fn_tab);
}
