
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "lib_acl.h"
#include "test_net.h"
#include "test_nettab.h"

void test_net_register(void)
{
	aut_register(__test_fn_tab);
}
