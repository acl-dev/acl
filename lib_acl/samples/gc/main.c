#include "lib_acl.h"
#include <stdio.h>
#include <string.h>
#include "mem_gc.h"
#include "mem_test.h"

static void init(void)
{
	mem_gc_hook();
}

int main(int argc acl_unused, char *argv[] acl_unused)
{
	init();
	mem_test();
	return (0);
}

