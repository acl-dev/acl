#include "stdafx.h"
#include "libws.h"

int main(int argc, char *argv[])
{
	test_websocket_main(argc, argv);
#ifdef WIN32
	printf("enter any key to exit ...\r\n");
	getchar();
#endif

	return 0;
}

