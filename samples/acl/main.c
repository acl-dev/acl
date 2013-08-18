#include "lib_acl.h"
#include <stdio.h>

int main(void)
{
	printf("current acl version: %s\n", acl_version());
	return (0);
}
