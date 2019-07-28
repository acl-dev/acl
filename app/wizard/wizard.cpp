// wizard.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <stdio.h>
#include "master_creator.h"
#include "http_creator.h"

static void create_db()
{

}

int main(int argc, char* argv[])
{
	(void) argc, (void) argv;

	acl::acl_cpp_init();

	while (true)
	{
		printf("select one below:\r\n");
		printf("m: master_service; d: db; h: http; q: exit\r\n");
		printf(">"); fflush(stdout);

		char buf[256];
		int n = acl_vstream_gets_nonl(ACL_VSTREAM_IN, buf, sizeof(buf));
		if (n == ACL_VSTREAM_EOF)
			break;
		else if (strcasecmp(buf, "m") == 0)
			master_creator();
		else if (strcasecmp(buf, "d") == 0)
			create_db();
		else if (strcasecmp(buf, "h") == 0)
			http_creator();
		else if (strcasecmp(buf, "q") == 0)
			break;
		else
			printf("unknown %s\r\n", buf);
	}

	printf("Bye!\r\n");
	return 0;
}
