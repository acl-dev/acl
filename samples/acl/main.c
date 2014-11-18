#include "lib_acl.h"
#include <stdio.h>

int main(void)
{
	printf("current acl version: %s\r\n", acl_version());
	printf("ACL_VSTREAM's size: %d\r\n", (int) sizeof(ACL_VSTREAM));
	return (0);
}
