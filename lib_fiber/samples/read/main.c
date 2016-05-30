#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lib_fiber.h"

int main(void)
{
	int   ret;
	char  buf[8192];

	//acl_sys_hook();

	ret = read(0, buf, sizeof(buf) - 1);

	if (ret <= 0) {
		printf("read error\r\n");
		return 1;
	}

	buf[ret] = 0;
	printf("read: [%s]\r\n", buf);

	write(1, buf, strlen(buf));

	return 0;
}
